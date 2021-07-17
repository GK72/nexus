#!/usr/bin/env bash

IMAGE_NAME="nxs/devenv"

docker build \
    --pull \
    --tag "${IMAGE_NAME}" \
    --file ~/nxs/env/Dockerfile.env \
    --build-arg DOCKER_GID=$(getent group docker | cut -d: -f3) \
    --build-arg USER_ID=$(id -u) \
    ~/nxs/env
