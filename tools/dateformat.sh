#!/bin/bash

DAY=$(date +'%d')
if [[ ${DAY:1} == 3 ]]; then
    DAY_SUFFIX="rd"
elif [[ ${DAY:1} == 2 ]]; then
    DAY_SUFFIX="nd"
elif [[ ${DAY:1} == 1 ]]; then
    DAY_SUFFIX="st"
fi

date +"%A, %d${DAY_SUFFIX} %B %Y - %H:%M"
