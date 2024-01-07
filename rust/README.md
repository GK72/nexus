# Building for Alpine

Use Rustup and build for musl as Alpine does not use glibc.

https://musl.libc.org/

```sh
rustup target add aarch64-unknown-linux-musl
rustup target add x86_64-unknown-linux-musl
```

To build a fully statically linked binary:

```sh
cargo build --release --target=aarch64-unknown-linux-musl
```
