#!/usr/bin/env bash

set -euo pipefail

PATH_ACCESS_TOKEN="${HOME}/tmp/.spt"
ACCESS_TOKEN=""

function msg() {
    >&2 echo "$*"
}

function access-token() {
    if [[ ! -e "${PATH_ACCESS_TOKEN}" ]]; then
        msg "Refreshing access token"
        get-access-token
    fi

    ACCESS_TOKEN=$(jq --raw-output ".access_token" "${PATH_ACCESS_TOKEN}")
}

function get() {
    curl "https://api.spotify.com/v1/me/$1" \
        --header "Authorization: Bearer ${ACCESS_TOKEN}"
}

function get-access-token() {
    local creds="$(echo -n "${SPOTIFY_CLI_CLIENT_ID}:${SPOTIFY_CLI_CLIENT_SECRET}" | base64 -w0)"
    curl "https://accounts.spotify.com/api/token" \
        --silent \
        --header "Content-Type: application/x-www-form-urlencoded" \
        --header "Authorization: Basic ${creds}" \
        --data "grant_type=client_credentials" \
        --output "${PATH_ACCESS_TOKEN}"
}

access-token
get playlists
