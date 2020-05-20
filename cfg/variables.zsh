# Black        0;30     Dark Gray     1;30
# Red          0;31     Light Red     1;31
# Green        0;32     Light Green   1;32
# Brown/Orange 0;33     Yellow        1;33
# Blue         0;34     Light Blue    1;34
# Purple       0;35     Light Purple  1;35
# Cyan         0;36     Light Cyan    1;36
# Light Gray   0;37     White         1;37

COLOR_DEF='\033[0m'
COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'

# Build configuration
GBUILD_ENV_SET=1
GBUILD_TYPE="Debug"
GCOMPILER="clang"
GCOMPILER_VER="11"
GCOMPILER_C="${GCOMPILER}-${GCOMPILER_VER}"
GCOMPILER_CXX="${GCOMPILER}++-${GCOMPILER_VER}"
GBUILD_GEN="Ninja"
GBUILD_JOBS=4
