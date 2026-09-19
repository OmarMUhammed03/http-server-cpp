# HTTP Server in C++ — CodeCrafters Challenge

A multithreaded HTTP/1.1 server built from scratch in C++23 using raw TCP primitives — no HTTP frameworks. Part of the [CodeCrafters "Build Your Own HTTP Server" challenge](https://app.codecrafters.io/courses/http-server/overview).

> HTTP is the protocol that powers the web. This project builds a HTTP server capable of handling simple GET/POST requests, serving files, and handling multiple concurrent connections using an event-driven architecture.

## Progress

| Stage | Status | Difficulty |
|-------|--------|------------|
| Bind to a port | ✅ Done | — |
| Respond with `200 OK` | ✅ Done | — |
| Extract URL path | ✅ Done | Easy |
| Respond with body | ✅ Done | Easy |
| Read header (`User-Agent`) | ✅ Done | Easy |
| Concurrent connections | ✅ Done | Easy |
| Return a file | ✅ Done | Medium |
| Read request body (POST) | ✅ Done | Medium |
| HTTP Compression (gzip) | ⏳ Coming soon | — |
| Persistent Connections (keep-alive) | ⏳ Coming soon | — |

Along the way: TCP sockets, HTTP request parsing, HTTP verbs/headers, thread pools, `epoll`, graceful shutdown.

## Features

- **TCP Server** — `AF_INET` + `SO_REUSEADDR`, non-blocking `accept4` with `SOCK_NONBLOCK | SOCK_CLOEXEC`
- **HTTP/1.1 Parsing** — request line (`METHOD PATH`), headers (`Key: Value`), `Content-Length` body framing, header-size guard (64 KiB)
- **Routing** — exact match (`/`, `/user-agent`) and wildcard prefix (`/echo/*` with body extraction)
- **Responses** — `200 OK`, `404 Not Found`, `503 Service Unavailable` with `Content-Type`, `Content-Length`, `Connection: close`
- **Concurrency** — `epoll` reactor (level-triggered, `MAX_EVENTS=64`) + fixed thread pool (8 workers, bounded queue 10k)
- **Backpressure** — bounded queue with `try_enqueue`; on overflow returns `503` + `Retry-After: 1`, high-water warning at 8k
- **Robustness** — per-connection buffering, `EAGAIN`/`EINTR` handling, `SIGPIPE` ignored, `SIGINT`/`SIGTERM` graceful shutdown

## Architecture

```
               ┌─────────────┐   EPOLLIN/EPOLLHUP   ┌────────────────┐
  Client ─────▶│  TcpServer  │─────────────────────▶│   EventLoop    │
               │  (epoll)    │◀─────────────────────│  (epoll_fd)    │
               └──────┬──────┘                      └────────────────┘
                      │  try_enqueue (bounded)
                      ▼
               ┌─────────────┐   route()   ┌────────┐
               │ ThreadPool  │────────────▶│ Router │
               │ 8 workers   │             └───┬────┘
               └──────┬──────┘                 │ handler(req) → HttpResponse
                      │                        ▼
                      │                 ┌──────────────┐
                      └────────────────▶│ HttpResponse │──send()──▶ Socket
                                        └──────────────┘
```

### Components

| File | Role |
|------|------|
| `src/server.{hpp,cpp}` | Core: `bind`/`listen`/`accept`, `epoll` loop, request reassembly (`is_request_complete`), `parse_request`, dispatch to pool |
| `src/event_loop.{hpp,cpp}` | Thin wrapper around `epoll_create1` / `epoll_ctl` / `epoll_wait` |
| `src/thread_pool.{hpp,cpp}` | Fixed workers, `mutex`+`cv`+`queue`, `try_enqueue` with `MAX_QUEUE_SIZE=10000` |
| `src/router.{hpp,cpp}` | `add_route(method, path, handler)`; `*` suffix = prefix/wildcard, extracts suffix into `req.body` |
| `src/request.{hpp,cpp}` | `HttpRequest { method, path, headers, body }` + `get_header_value()` |
| `src/response.{hpp,cpp}` | Factory `ok()`, `not_found()`, `service_unavailable()`; blocking `send()` loop |
| `src/socket.{hpp,cpp}` | RAII FD wrapper, move-only, `send`/`recv`, `set_nonblocking()` |
| `src/constants.hpp` | `PORT=4221`, `BACKLOG=128`, `THREAD_POOL_SIZE=8`, `BUFFER_SIZE=4096` |
| `src/main.cpp` | Wiring, signal handlers, route registration |

### Concurrency Model

1. Main thread runs `EventLoop::wait()` — level-triggered `epoll`.
2. Listening socket is non-blocking; on `EPOLLIN` drains all pending `accept4` calls.
3. Each client `fd` is registered `EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP`, data appended to `conn_buffers_[fd]`.
4. When `is_request_complete()` (headers `"\r\n\r\n"` + `Content-Length` body) — buffer sliced to one request, `fd` removed from epoll, task enqueued.
5. Worker calls `handle_queued_request()` → `parse_request()` → `router_.route()` → `response.send()` (socket closed on `Socket` destructor = `Connection: close`).
6. If queue full → `503` directly on reactor thread.

## Project Structure

```
.
├── src/
│   ├── main.cpp
│   ├── constants.hpp
│   ├── server.{hpp,cpp}
│   ├── event_loop.{hpp,cpp}
│   ├── thread_pool.{hpp,cpp}
│   ├── router.{hpp,cpp}
│   ├── request.{hpp,cpp}
│   ├── response.{hpp,cpp}
│   └── socket.{hpp,cpp}
├── CMakeLists.txt
├── vcpkg.json            # pthreads, zlib (for upcoming gzip)
├── your_program.sh       # build + run helper
├── .codecrafters/        # compile.sh / run.sh (remote)
└── README.md
```

## Getting Started

### Prerequisites

- CMake ≥ 3.13
- C++23 compiler (GCC/Clang)
- pthreads, zlib (`vcpkg` handles it)
- Linux (uses `epoll`, `accept4`)

### Build

```sh
# via helper (local)
./your_program.sh

# manual
cmake -B build -S .
cmake --build ./build
./build/http-server
```

Server listens on `0.0.0.0:4221`.

### Run & Test

```sh
# Terminal 1 — start server
./your_program.sh

# Terminal 2 — exercise endpoints
curl -i http://localhost:4221/
curl -i http://localhost:4221/echo/hello
curl -i -H "User-Agent: curl/8.0" http://localhost:4221/user-agent
curl -i http://localhost:4221/unknown   # → 404

# POST with body (Content-Length framing)
curl -i -X POST http://localhost:4221/echo/test --data "hello body"

# CodeCrafters tests (requires `codecrafters` CLI)
codecrafters submit
codecrafters test
```

## API

| Method | Path | Description | Response |
|--------|------|-------------|----------|
| `GET` | `/` | Health/root | `200` empty body |
| `GET` | `/echo/{text}` | Echo suffix | `200` `text/plain` `{text}` |
| `GET` | `/user-agent` | Reflect `User-Agent` header | `200` `text/plain` `<value>` |
| `*` | `*` | Unmatched | `404` |
| `*` | `*` | Overload (queue >10k) | `503` `Retry-After: 1` |

All responses include `Content-Length` and `Connection: close` (keep-alive planned).

## Implementation Notes

- **Parsing** — `parse_request()` splits on `\r\n`, extracts `method`/`path` from request line, iterates headers until empty line, slices body by `Content-Length` (truncates overflow).
- **Wildcard routes** — path ending `*` (e.g. `/echo/*`) matches prefix; suffix becomes `req.body` for handler (see `Router::match_route` in `src/router.cpp:7` and `src/server.cpp:125`).
- **Buffering** — `conn_buffers_` accumulates per-fd until complete request; guards against header overflow and handles pipelined remainder (currently discarded due to `close`).
- **Shutdown** — `SIGINT`/`SIGTERM` → `TcpServer::shutdown()` (atomic `running_` flag, `shutdown(SHUT_RDWR)` to wake `epoll_wait`, `ThreadPool::shutdown()`).

## Roadmap

- [ ] **HTTP Compression** — `Accept-Encoding: gzip` negotiation, `Content-Encoding: gzip` via zlib
- [ ] **Persistent Connections** — `Connection: keep-alive`, pipelining, timeout handling
- [ ] File serving (`GET /files/{name}` / `POST /files/{name}` with `--directory` flag)
- [ ] More methods/headers, chunked transfer

## Tech Stack

- **Language:** C++23
- **Build:** CMake, vcpkg
- **OS APIs:** POSIX sockets, `epoll`, `fcntl`, pthreads
- **Deps:** `Threads`, `ZLIB`

## Challenge Instructions (CodeCrafters)

```sh
codecrafters submit          # run remote tests
./your_program.sh            # local run (src/main.cpp)
```

Entry point is `src/main.cpp:18`. See [codecrafters.io](https://codecrafters.io) if viewing on GitHub.

## License

MIT — see original CodeCrafters template.
