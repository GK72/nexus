function colormap() {
    for i in {0..255}; do print -Pn "%K{$i} %k%F{$i}${(l:3::0:)i}%f " ${${(M)$((i%8)):#7}:+$'\n'}; done
}

function c2() {
    local sourceFile="${1}"
    shift
    local cxx_flags="$@"

    cppfront "${sourceFile}"
    g++ -I$HOME/.local/include -std=${CXXSTD:=c++20} ${cxx_flags} "${sourceFile%.cpp2}.cpp"
}

# Change git worktree
function cw() {
    local SELECTION=$(git worktree list | field 1 | fzf --height=20% --layout=reverse)
    [[ -n "${SELECTION}" ]] && cd "${SELECTION}"
}

function disas() {
    objdump -D -M intel "$1" | bat --language asm --paging=never
}

function godbolt() {
    local SOURCE_FILE="$1"
    shift
    local CXX_FLAGS="$@"
    gcc -std=c++20 -O3 "$CXX_FLAGS" "$SOURCE_FILE" -S -o - -masm=intel | c++filt | grep -vE '\s+\.'
}

function field() {
    FIELD_NUM="$1"
    while read INPUT; do
        awk '{print $'"$FIELD_NUM"'}' <<< "$INPUT"
    done
}

# git log
function gl() {
    if [ $# -eq 0 ]; then
        ARGS="-n10"
    else
        ARGS="$@"
    fi

    git --no-pager log \
        --decorate \
        --graph \
        --date=format:'%a %d %b %Y %H:%M:%S' \
        --pretty="tformat:%C(178)%h %C(029)%${NXS_GITLOG_DATE:-cd} %C(027)%<(20,trunc)%cn %C(250)%s %C(auto)%d" \
        ${(z)ARGS}
}

function find-time-range() {
    PATH_FIND="$1"
    PATTERN="$2"
    START_SECONDS=$3
    RANGE_SECONDS=$4
    COMMAND="$5"

    MINUTES_AGO=$((($(date +%s)-$START_SECONDS)/60))

    find "${PATH_FIND}" \
        -name "${PATTERN}" \
        -cmin -"${MINUTES_AGO}" \
        -cmin +$(($MINUTES_AGO-($RANGE_SECONDS/60))) \
        -exec "${(z)COMMAND}" {} \;
        # -exec $(echo "${COMMAND}") {} \;          # Bash
}

function diffj() {
    DIFF_MINUS="$1"
    DIFF_PLUS="$2"

    if [[ ${#FLAGS_DIFFJ} -eq 0 ]]; then FLAGS_DIFFJ=(); fi
    diff <(jq . "$DIFF_MINUS") <(jq . "$DIFF_PLUS") "$FLAGS_DIFFJ[@]"
}

function random-string() {
    local LENGTH="$1"
    local PATTERN="$2"

    if [[ -z $LENGTH ]]; then
        echo -e "Usage: random-string LENGTH [PATTERN]\n\nPattern is given to 'tr'"
    else
        cat /dev/urandom | base64 | tr -dc "${PATTERN:="0-9a-zA-Z"}" | head -c${LENGTH}
    fi
}
