# Black        0;30     Dark Gray     1;30
# Red          0;31     Light Red     1;31
# Green        0;32     Light Green   1;32
# Brown/Orange 0;33     Yellow        1;33
# Blue         0;34     Light Blue    1;34
# Purple       0;35     Light Purple  1;35
# Cyan         0;36     Light Cyan    1;36
# Light Gray   0;37     White         1;37

export COLOR_DEF='\033[0m'
export COLOR_RED='\033[0;31m'
export COLOR_GREEN='\033[0;32m'

# Build configuration
export NXS_BUILD_ENV_SET=1
export NXS_BUILD_TYPE="Debug"
export NXS_COMPILER="clang"
export NXS_COMPILER_VER="11"
export NXS_COMPILER_C="${NXS_COMPILER}-${NXS_COMPILER_VER}"
export NXS_COMPILER_CXX="${NXS_COMPILER}++-${NXS_COMPILER_VER}"
export NXS_BUILD_GEN="Ninja"
export NXS_BUILD_JOBS=4
