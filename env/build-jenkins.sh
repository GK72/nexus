#!/usr/bin/env bash

cd ~/nxs/env

docker build \
    --build-arg DOCKER_GROUP_ID=$(getent group docker | cut -d: -f3) \
    -f ~/nxs/env/Dockerfile.jenkins \
    -t nxs/ci .
