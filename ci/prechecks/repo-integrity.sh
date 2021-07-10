#!/bin/bash

REPO_ROOT=$(git rev-parse --show-toplevel)
FILES=$(find "${REPO_ROOT}" -name '*.sh')

EXITCODE=0

function check-permission() {
    FILE="$1"
    EXPECTED_PERM=100755

    PERM=$(git ls-tree HEAD "$FILE" | cut -d' ' -f1)
    if [[ "$PERM" != "$EXPECTED_PERM" ]]; then
        FIRST_LINE=$(head -n1 "$FILE")
        if [[ "${FIRST_LINE}" =~ "#!/" ]]; then
            echo "Incorrect permission: $PERM on $FILE"
            exit 1
        fi
    fi
}

for FILE in $FILES; do
    check-permission "$FILE" || EXITCODE=1
done

exit $EXITCODE
