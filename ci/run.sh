#!/bin/bash

function usage() {
    cat << EOF
usage: ${1##*/} -p | --param <ARG> [OPTIONS...]

    -p | --param <ARG>                      Some parameter with argument

Optional parameters:
EOF
}

NONE="<none>"
FWD_ARGS="${NONE}"

while [[ $# -gt 0 ]]; do
    if [[ "${FWD_ARGS}" == "${NONE}" ]]; then
        case $1 in
            -b | --branch)          BRANCH="$2"; shift ;;
            -d | --docker)          DOCKER_IMG="$2"; shift ;;
            --)                     FWD_ARGS="";;
            -h | --help)            usage ${0##*/}; exit 0;;
            -*)                     usage; exit 255;;
            *)                      break;;
        esac
    else
        FWD_ARGS="${FWD_ARGS}$1 "
        shift
    fi
    shift
done

CMD="$@"

# -------------------------------------==[ Initializing ]==-----------------------------------------


# -----------------------------------------==[ MAIN ]==---------------------------------------------

docker run \
    --rm \
    --volume "${HOME}/repos/nexus.git/${BRANCH:-master}":/opt/nexus \
    --env NXS_COMPILER=gcc \
    --env NXS_GENERATOR=ninja \
    --env NXS_BUILD_TYPE=release \
    --env NXS_BUILD_JOBS=$NXS_BUILD_JOBS \
    --workdir /opt/nexus \
    "${DOCKER_IMG:-nxs/build}" \
    /bin/bash -c $CMD
