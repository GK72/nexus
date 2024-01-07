# syntax=docker/dockerfile:1
FROM alpine

COPY target/aarch64-unknown-linux-musl/release/nexus-rs /opt/bin/nexus-rs

ENTRYPOINT ["/opt/bin/nexus-rs"]
