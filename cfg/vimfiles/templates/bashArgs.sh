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
            --)                     FWD_ARGS="";            shift ;;
            -h | --help)            usage ${0##*/};         exit 0; shift ;;
            *)                      usage; exit 255;        shift ;;
        esac
    else
        FWD_ARGS="${FWD_ARGS}$1 "
        shift
    fi
done

# -------------------------------------==[ Initializing ]==-----------------------------------------


# -----------------------------------------==[ MAIN ]==---------------------------------------------

