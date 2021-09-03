# Black        0;30     Dark Gray     1;30
# Red          0;31     Light Red     1;31
# Green        0;32     Light Green   1;32
# Brown/Orange 0;33     Yellow        1;33
# Blue         0;34     Light Blue    1;34
# Purple       0;35     Light Purple  1;35
# Cyan         0;36     Light Cyan    1;36
# Light Gray   0;37     White         1;37

export COLOR_DEF='\033[0m'
export COLOR_RED='\033[1;31m'
export COLOR_GREEN='\033[1;32m'
export COLOR_PURPLE='\033[1;35m'

# Build configuration
export NXS_BUILD_ENV_SET=1
export NXS_BUILD_TYPE="Debug"
export NXS_COMPILER=gcc
# export NXS_COMPILER_VER=
export NXS_BUILD_GEN="Ninja"
export NXS_BUILD_JOBS=$(($(nproc) / 2))

export NXS_GITLOG_DATE=cd       # git log alias date: ad/cd (author date / commit date)

export SYMBOL_MUSICAL_NOTE='🎵'

CMD_SED_EPOCH2DT='sed -r '"'"'s/(.*)([[:digit:]]{10})([[:digit:]]{6})(.*)/echo \1$(date -d @\2.\3 +"%H:%M:%S:%N")\4/;e'"'"
