# WebClient extension

Asynchronous **HTTP** and **WebSocket** client natives for SourceMod, backed by a
Rust core (tokio + reqwest + tokio-tungstenite over **rustls**, no OpenSSL). The
C++ half is a thin shim (native registration, `Handle` objects, main-thread
marshaling); all networking, TLS, and the async runtime live in the Rust crate
under `rust/` (`crate-type = ["staticlib"]`), reached through the C ABI in `ffi.h`.

## How it works

```
Pawn plugin ─native→ C++ shim ─FFI→ Rust (tokio runtime, reqwest/tungstenite, rustls)
                        ▲                        │  request runs on a worker thread
   IPluginFunction      │                        ▼
   ::Execute() on       │               Mutex<VecDeque<Completion>>   (Rust-side queue)
   the MAIN THREAD      │                        ▲
                        └── C++ game-frame hook ─┘  (drains via wc_poll_completion each frame)
```

Network I/O never touches the game thread. Worker tasks push completed responses /
WebSocket events onto an internal queue; the per-frame hook
(`ISourceMod::AddGameFrameHook`) drains it on the main thread and only there fires
the plugin callback. Rust never calls into the SourcePawn VM. This mirrors
SourceMod's threaded-DB path (`core/logic/Database.cpp`).

Object lifetimes use generational ids: a `Handle` wraps a heap-boxed `u64` slot-map
key, so a stale id resolves to "gone" instead of aliasing a reused object. Closing
the `Handle` (by the plugin, on unload, or after a callback) frees the backing Rust
object exactly once.

## Pawn API

Every methodmap, native, and callback is documented in
`plugins/include/webclient.inc`; `examples/webclient_example.sp` is a working
plugin. Quick taste:

```sourcepawn
HTTPRequest req = new HTTPRequest("https://example.com/");
req.SetHeader("User-Agent", "sm");
req.Get(OnResponse);   // also Post/Put/Patch/Delete/Head/Options, PostForm, Upload/DownloadFile
```

A few things the signatures don't make obvious:

- **Text vs binary.** `GetData`/`Send`/`Post`-without-`length` cross as
  NUL-terminated strings (right for JSON/text). For bytes with interior NULs use
  the explicit-length forms (`GetDataBinary`, `SendBinary`, the `length` arg).
- **Header/param values are format strings** (`SetHeader`, `AppendQueryParam`,
  `AppendFormParam`) — pass user data via `"%s"`.
- **A socket fires exactly one terminal callback** (disconnect **or** error, never
  both); `delete` the handle there.
- **File paths** (`UploadFile`/`DownloadFile`/`SaveToFile`) are confined to
  `addons/sourcemod/data/webclient`.

## Limits & safety defaults

Not visible in the include — applied by the Rust core so untrusted input or a
hostile endpoint can't take the server down:

- **SSRF guard (on by default):** private/loopback/link-local targets — incl. the
  cloud metadata IP `169.254.169.254` and IPv6 ranges that embed those (NAT64 /
  6to4) — are refused, re-checked on every redirect hop; WebSockets get the same
  check before connect. `AllowLocalNetwork = true` opts out for trusted intranet URLs.
- **TLS:** rustls + bundled Mozilla roots (webpki-roots); the OS trust store is not
  consulted. Verification is always on — *except* a request/socket that sets **both**
  `AllowLocalNetwork = true` and `AllowInvalidCerts = true`, which waives cert/hostname
  checks for that local target only (e.g. a self-signed or friendly-named intranet
  host). Public-facing requests can never skip verification.
- **Size caps:** response body 64 MiB *after* gzip/deflate (decompression-bomb
  safe); `UploadFile` bound by the same; `DownloadFile` streams to disk with a
  per-file (2 GiB) + global concurrent-bytes cap.
- **Timeouts:** a default total and connect timeout apply when unset (`Timeout = 0`
  means "use the default", not "unbounded").
- **WebSocket keepalive:** a quiet socket is auto-pinged (~30s); if nothing inbound
  arrives for ~90s it's failed with a terminal error instead of pinning a slot.
- **Concurrency caps:** bounded in-flight HTTP / live WebSockets (submits past the
  cap fail); bounded WS send queue (`Send`/`SendBinary` return `false` when full);
  capped buffered inbound WS payload + a per-message size limit.

## Building

Needs a Rust toolchain (**1.85+**, edition 2024) with your targets added
(`rustup target add x86_64-unknown-linux-gnu i686-unknown-linux-gnu`, or the
`*-pc-windows-msvc` pair). The committed `rust/Cargo.lock` pins everything.

A normal SourceMod build links it automatically: the extension is in the top-level
`AMBuildScript`, and `AMBuilder` runs `cargo` (re-running only when Rust sources
change) and links the staticlib into `webclient.ext`. If `cargo` isn't on `PATH`
the extension is skipped with a notice rather than failing the build. To iterate on
just the core: `cargo build --release --target <triple>` in `rust/`.

### Old-glibc floor

SourceMod targets a low glibc floor. The C++ half is fine
(`-static-libstdc++`/`-static-libgcc`); the floor comes from the Rust staticlib,
which references newer syscalls (`statx`, `getrandom`) only as *weak* symbols with
fallbacks. For a guaranteed floor, build in a manylinux2014 (glibc 2.17) container
— the floor is bound at the final link, not by `rustc`. (`webpki-roots` is bundled
instead of `rustls-native-certs` partly for this: no system cert access, fewer
syscalls.) Verify on the final binary:

```sh
objdump -T webclient.ext.so | grep -oE 'GLIBC_[0-9.]+' | sort -uV | tail
```

## Layout

| File | Role |
|------|------|
| `extension.{h,cpp}` | Lifecycle, frame-hook drain, callback dispatch, unload cancellation |
| `natives.cpp` | HTTPRequest / HTTPResponse / WebSocket natives; Pawn⇆C marshaling |
| `handles.cpp` | `Handle` types (id boxing, destroy hooks) |
| `ffi.h` | C ABI contract (must match `rust/src/lib.rs`) |
| `rust/src/lib.rs` | FFI surface (every export wrapped in `catch_unwind`) |
| `rust/src/{registry,http,ws,ssrf}.rs` | registry/queue, HTTP + WebSocket tasks, SSRF guard |
| `AMBuilder` | Runs cargo, links the staticlib |
