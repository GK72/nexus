#!/bin/zsh

function usage() {
    cat << EOF
usage: ${1##*/} -p | --path <REPO PATH> [OPTIONS...]

    -p | --path <REPO PATH>                 Path to the repository

Optional parameters:
    -b | --build_type <BUILD TYPE>          Type of the build e.g. Debug, Release
    -c | --compiler <COMPILER NAME>         Compiler used by CMake
    -d | --delete                           Make a clean build
    -h | --help                             Print help
    -j | --jobs <N>                         Number of parallel jobs
    -l | --list                             List XBUILD environment variables
    -r | --run                              Finds the executable in the build dir and runs it
    -t | --target <TARGET NAME>             Target to build
    -v | --compiler-ver                     Compiler version
         --args <"arg1, arg2, ...">         Arguments to the executed binary (in quotes)
         --gcov-dir                         Directory in which the GCOV should run (goes through find)
         --gcov-exclude                     Source file exclude filter for GCOV
         --gcov-filter                      Source file filter for GCOV
         --gcov-keep                        Do not delete gcda files after processing
         --gdb                              Run in gdb
         --valgrind                         Run valgrind
         --                                 Forwarding arguments to CMake
EOF
}

function list() {
    echo "Default build settings"
    echo ""
    echo "NXS_BUILD_TYPE   = ${NXS_BUILD_TYPE}"
    echo "NXS_BUILD_GEN    = ${NXS_BUILD_GEN}"
    echo "NXS_BUILD_JOBS   = ${NXS_BUILD_JOBS}"
    echo "NXS_COMPILER     = ${NXS_COMPILER}"
    echo "NXS_COMPILER_VER = ${NXS_COMPILER_VER}"
}

GCOV_FAIL_LIMIT=90
GCOV_HTML_DIR="${HOME}/tmp/coverage-test"

NONE="<none>"
FWD_ARGS="${NONE}"

while [[ $# -gt 0 ]]; do
    if [[ "${FWD_ARGS}" == "${NONE}" ]]; then
        case $1 in
            -b | --build_type)      BUILD_TYPE="$2";    shift; shift ;;
            -c | --compiler)        COMPILER="$2";      shift; shift ;;
            -d | --delete)          DELETE=1;           shift ;;
            -j | --jobs)            JOBS="$2";          shift; shift ;;
            -l | --list)            list; exit 0;       shift ;;
            -p | --path)            PATH_REPO="$2";     shift; shift ;;
            -r | --run)             RUN=1;              shift ;;
            -t | --target)          TARGET="$2";        shift; shift ;;
            -v | --compiler-ver)    COMPILER_VER="$2";  shift; shift ;;
                 --args)            BINARY_ARGS="$2";   shift; shift ;;
                 --gcov-dir)        GCOV_DIR="$2";      shift; shift ;;
                 --gcov-exclude)    GCOV_EXCLUDE="$2";  shift; shift ;;
                 --gcov-filter)     GCOV_FILTER="$2";   shift; shift ;;
                 --gcov-keep)       GCOV_KEEP=1;        shift;;
                 --gdb)             GDB=1;              shift;;
                 --valgrind)        VALGRIND=1;         shift;;
            --)                     FWD_ARGS="";        shift ;;
            -h | --help)            usage ${0##*/};     exit 0; shift ;;
            *)                      usage; exit 255;    shift ;;
        esac
    else
        FWD_ARGS="${FWD_ARGS}$1 "
        shift
    fi
done

# -------------------------------------==[ Initializing ]==-----------------------------------------

# Error checking
if [[ -z "${PATH_REPO}" ]]; then
    echo "No path to the repository provided."
    echo "Don't know what to build!"
    exit 1
fi

BUILD_TYPE="${BUILD_TYPE-$NXS_BUILD_TYPE}"
if [[ -z "${BUILD_TYPE}" ]]; then
    echo "No build type specified"
    exit 2
fi

# Setting default values
if [[ -z "${COMPILER}" ]]; then
    COMPILER="${NXS_COMPILER}"
    COMPILER_VER="${NXS_COMPILER_VER}"
fi

GENERATOR="${NXS_BUILD_GEN-"Unix Makefiles"}"
TARGET="${TARGET-"all"}"

if [[ -z "${JOBS}" ]]; then
    JOBS="${NXS_BUILD_JOBS-1}"
fi

# Setting up compilers
if [[ -z "${COMPILER_VER}" ]]; then
    if [[ "${COMPILER}" == "gcc" ]]; then
        COMPILER_C="gcc"
        COMPILER_CXX="g++"
    elif [[ "${COMPILER}" == "clang" ]]; then
        COMPILER_C="clang"
        COMPILER_CXX="clang++"
    fi
else
    if [[ "${COMPILER}" == "gcc" ]]; then
        COMPILER_C="gcc-${COMPILER_VER}"
        COMPILER_CXX="g++-${COMPILER_VER}"
    elif [[ "${COMPILER}" == "clang" ]]; then
        COMPILER_C="clang-${COMPILER_VER}"
        COMPILER_CXX="clang++-${COMPILER_VER}"
    fi
fi

# Checking build file according to the provided generator
if [[ "${GENERATOR}" == "Ninja" ]]; then
    BUILD_FILE="build.ninja"
elif [[ "${GENERATOR}" == "Unix Makefiles" ]]; then
    BUILD_FILE="Makefile"
fi


BUILD_DIR="${PATH_REPO}/build/${BUILD_TYPE:l}-${COMPILER}${COMPILER_VER}"

if [[ -n "${DELETE}" ]]; then
    echo "Removing ${BUILD_DIR}"
    rm -rfI "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

if [[ "${FWD_ARGS}" == "${NONE}" ]]; then
    FWD_ARGS=""
fi

# -----------------------------------------==[ MAIN ]==---------------------------------------------

# Generate build files if hasn't been already
if [[ ! -e "${BUILD_DIR}/${BUILD_FILE}" ]]; then
    (cd "${BUILD_DIR}" &&
    cmake "${FWD_ARGS}" \
          -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
          -DCMAKE_C_COMPILER="${COMPILER_C}" \
          -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -G"${GENERATOR}" \
          "${PATH_REPO}"
    )
fi

# Link compile_commands.json to the root of the repo
if [[ ! -e "${PATH_REPO}/compile_commands.json" ]]; then
    ln -sv "${BUILD_DIR}/compile_commands.json" \
        "${PATH_REPO}/compile_commands.json"
fi

# Build
cmake --build "${BUILD_DIR}" \
      --target "${TARGET}" \
      -- -j"${JOBS}"

if [[ $? != 0 ]]; then
    echo -e "${COLOR_RED}  ----==[ BUILD FAILED ! ]==----"
    return 2
fi

# VALGRIND_OPTIONS="--tool=memcheck"
VALGRIND_OPTIONS="--tool=massif"

# Run executable
if [[ -n "${RUN}" ]]; then
    PATH_BIN=$(find "${BUILD_DIR}" -name "${TARGET}")
    if [[ $(echo "${PATH_BIN}" | wc -l) -gt 1 ]]; then
        echo -e "Multiple executable found:\n${PATH_BIN}"
        exit 3
    fi

    echo "Executable found at: ${PATH_BIN}"
    if [[ -n "${GDB}" ]]; then
        gdb --args "${PATH_BIN}" ${(z)BINARY_ARGS}
    elif [[ -n "${VALGRIND}" ]]; then
        valgrind "${(z)VALGRIND_OPTIONS}" "${PATH_BIN}" ${(z)BINARY_ARGS}
    else
        "${PATH_BIN}" ${(z)BINARY_ARGS}
        RETCODE_BIN=$?
    fi

    if [[ $RETCODE_BIN != 0 ]]; then
        if [[ "${PATH_BIN}" =~ /test/ ]]; then
            echo -e "${COLOR_RED}  ----==[ TEST FAILED ! ]==----"
        else
            echo -e "${COLOR_RED}  ----==[ Executable returned with error ! ]==----"
        fi
        exit $RETCODE_BIN
    fi
fi

if [[ "${BUILD_TYPE}" == "coverage" ]]; then
    if [[ -z "${GCOV_DIR}" ]]; then
        echo -e "${COLOR_BLUE}  ----==[ Missing gcov dir ! ]==----"
        exit 1
    fi

    if [[ -e "${GCOV_HTML_DIR}" ]]; then
        rm -rf "${GCOV_HTML_DIR}"
    fi

    mkdir -p "${GCOV_HTML_DIR}"

    GCOV_WORK_DIR=$(find "${BUILD_DIR}" -type d -name "${GCOV_DIR}")
    RUN_GCOV="gcovr ${GCOV_WORK_DIR} \
        --object-directory ${GCOV_WORK_DIR} \
        --root ${PATH_REPO} \
        --filter .\*${GCOV_DIR}/.\*${GCOV_FILTER}.\* \
        --exclude .\*test/Test.\* \
        --fail-under-line ${GCOV_FAIL_LIMIT} \
        --print-summary \
        --html-details \
        --output ${GCOV_HTML_DIR}/main.html"

        # --fail-under-branch ${GCOV_FAIL_LIMIT} \

    if [[ -n "${GCOV_EXCLUDE}" ]]; then
        RUN_GCOV+=" --exclude .\*${GCOV_EXCLUDE}.\*"
    fi

    if [[ -z "${GCOV_KEEP}" ]]; then
        RUN_GCOV+=" --delete"
    fi

    eval "(cd ${GCOV_WORK_DIR} && ${RUN_GCOV})"

    RETCODE_COV=$?
    if [[ $RETCODE_COV != 0 ]]; then
        echo -e "${COLOR_PURPLE}  ----==[ COVERAGE TEST FAILED ! ]==----"
        if [[ $RETCODE_COV == 2 ]]; then
            echo -e "${COLOR_PURPLE}         Line coverage under ${GCOV_FAIL_LIMIT}%"
        elif [[ $RETCODE_COV == 4 ]]; then
            echo -e "${COLOR_PURPLE}        Branch coverage under ${GCOV_FAIL_LIMIT}%"
        elif [[ $RETCODE_COV == 6 ]]; then
            echo -e "${COLOR_PURPLE}    Line and branch coverage under ${GCOV_FAIL_LIMIT}%"
        else
            echo -e "${COLOR_RED}  ----==[ Unknown GCOV error ! ]==----"
        fi
        exit 4
    else
        echo -e "${COLOR_GREEN}  ----==[ COVERAGE TEST PASSED ]==----"
    fi
fi
