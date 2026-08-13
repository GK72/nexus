# Smoke-tests that the statically-linked baldr binary (built with
# -DBALDR_STATIC_LINK=ON) is genuinely standalone: Alpine ships no
# glibc/libstdc++ at all, so any missed dynamic dependency fails immediately
# on `docker run`.
#
# Build context is a throwaway directory containing just this Dockerfile and
# the `baldr` binary (see baldr/tests/static-link.sh) — not the repo root.

FROM alpine:latest

COPY baldr /baldr

ENTRYPOINT ["/baldr"]
