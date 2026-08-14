FROM debian:11

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        cmake \
        g++ \
    && rm -rf /var/lib/apt/lists/*

# HOME is not valid for an "external user" (host user ID). It is used by config
# loading mechanism.
ENV HOME=/tmp
