#!/usr/bin/env bash

function usage() {
    cat << EOF
usage: ${1##*/} [OPTIONS...]

Optional parameters:
    -f | --force                            Force stop container, i.e. restart container
    --                                      Forwarding arguments to 'docker run'
EOF
}

NONE="<none>"
FWD_ARGS="${NONE}"

while [[ $# -gt 0 ]]; do
    if [[ "${FWD_ARGS}" == "${NONE}" ]]; then
        case $1 in
            -m | --mount-home)      MOUNT_HOME="--volume ${HOME}:${HOME}";;
            -f | --force)           FORCE=1;;
            --)                     FWD_ARGS="";;
            -h | --help)            usage ${0##*/}; exit 0;;
            -*)                     usage; exit 255;;
            *)                      usage; exit 255;;
        esac
    else
        FWD_ARGS="${FWD_ARGS}$1 "
    fi
    shift
done

CMD="$@"

# -------------------------------------==[ Initializing ]==-----------------------------------------

CONTAINER_NAME="devenv"
IMAGE_NAME="nxs/devenv"
SHELL="/usr/bin/zsh"

if [[ -n $FORCE ]]; then
    docker rm -f "${CONTAINER_NAME}" > /dev/null
fi

# -----------------------------------------==[ MAIN ]==---------------------------------------------

DOCKER_PS=$(docker ps -af name='^'"${CONTAINER_NAME}"'$')

if [[ $(echo "${DOCKER_PS}" | wc -l) -eq 2 ]]; then
    if [[ $(docker inspect "${CONTAINER_NAME}" | jq '.[].State.Status') == '"exited"' ]]; then
        docker start "${CONTAINER_NAME}"
    fi
    docker attach "${CONTAINER_NAME}"
else
    docker run \
        -it \
        --volume /var/run/docker.sock:/var/run/docker.sock \
        ${MOUNT_HOME} \
        --name "${CONTAINER_NAME}" \
        ${FWD_ARGS/$NONE/""} \
        "${IMAGE_NAME}" \
        "${SHELL}"
fi
