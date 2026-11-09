#!/usr/bin/env bash
#
# Renders the book with Nexus's doc.sh, adding the book's own extensions (doc/lib/*.rb), if any.
#
#   tools/doc.sh doc              render to build/doc/@name@/
#   tools/doc.sh --serve doc      re-render on every change

set -euo pipefail
shopt -s nullglob

PROJECT_DIR="$(realpath "$(dirname "$(realpath "$0")")/..")"
NEXUS_DIR="${NEXUS_DIR:-$(realpath -m "${PROJECT_DIR}/../nexus")}"

requires=()
for lib in "${PROJECT_DIR}"/doc/lib/*.rb; do
    requires+=(-r "${lib}")
done
exec "${NEXUS_DIR}/tools/doc.sh" "${requires[@]}" "$@"
