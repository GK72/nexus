# Generate CMake, build it and run it
# Configuration via environment variables

# param1        Source path
# param2        Target name
# param3        Executable path (inside build path)
function gbuild() {
    if [[ -z "${GBUILD_ENV_SET}" ]]; then
        echo -n "GBUILD_ENV_SET is not set."
        return 1
    fi

    if [[ -z "${2}" ]]; then
        TARGET_NAME="all"
    else
        TARGET_NAME="${2}"
    fi

    BUILD_DIR="${1}/build/${GBUILD_TYPE:l}"      # bash: ${GBUILD_TYPE,,}
    mkdir -p "${BUILD_DIR}"

    # Generate build files if hasn't been already
    if [[ ! -e "${BUILD_DIR}/CMakeFiles" ]]; then
        cmake -DCMAKE_BUILD_TYPE=${GBUILD_TYPE} \
              -DCMAKE_C_COMPILER="${GCOMPILER_C}" \
              -DCMAKE_CXX_COMPILER="${GCOMPILER_CXX}" \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -G"${GBUILD_GEN}" \
              -S "${1}" \
              -B "${BUILD_DIR}"
    fi

    # Build
    cmake --build "${BUILD_DIR}" \
          --target "${TARGET_NAME}" \
          --parallel "${GBUILD_JOBS}"

    if [[ $? != 0 ]]; then
        echo -e "  ----==[ ${COLOR_RED}BUILD FAILED ! ]==----"
        return 2
    fi

    # Link compile_commands.json to the root of the repo
    if [[ ! -e "${BUILD_DIR}/compile_commands.json" ]]; then
        ln -s "${BUILD_DIR}/compile_commands.json" \
            "${1}/compile_commands.json"
    fi

    # Run executable
    if [[ -n "${3}" ]]; then
        "${BUILD_DIR}/${3}"
    fi
}
