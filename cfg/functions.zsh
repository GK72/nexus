function findinfiles() {
    noparam=0
    while getopts 'i:' flag; do
        case "${flag}" in
            i) shift
               find . -type f -print0 | xargs -0 grep -i "$1" -n
               noparam=1
               ;;
        esac
    done

    if [ $noparam -eq 0  ] ; then
        find . -type f -print0 | xargs -0 grep "$1" -n
    fi
}

function colormap() {
    for i in {0..255}; do print -Pn "%K{$i} %k%F{$i}${(l:3::0:)i}%f " ${${(M)$((i%8)):#7}:+$'\n'}; done
}
