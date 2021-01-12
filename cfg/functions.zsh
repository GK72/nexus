function colormap() {
    for i in {0..255}; do print -Pn "%K{$i} %k%F{$i}${(l:3::0:)i}%f " ${${(M)$((i%8)):#7}:+$'\n'}; done
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
