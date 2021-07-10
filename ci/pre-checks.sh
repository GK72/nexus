#!/bin/bash

PATH_SCRIPT=$(realpath "$0")
CHECKS=$(find "${PATH_SCRIPT%/*}/prechecks" -name '*.sh')

EXITCODE=0

for FILE in "$CHECKS"; do
    "$FILE" || {
        >&2 echo "$(basename "${FILE}") check failed!"
        EXITCODE=1
    }
done

exit $EXITCODE
