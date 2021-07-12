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
