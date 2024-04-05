# Building GFX application

There is a dependency version mismatch in `winit`.

`glium` version needs to be updated in `imgui-glium-renderer/Cargo.toml`

* https://github.com/imgui-rs/imgui-rs/issues/759
* https://github.com/imgui-rs/imgui-rs/pull/765/commits/1a3e7ca748584c561ff0fb395a215f23b9c0e4da#diff-e74620692fda6a49120226a2c1c4bfe2080cf2e0d9066e8159fe9fc6906c054a

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