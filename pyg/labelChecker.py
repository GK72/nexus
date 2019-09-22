#!/usr/bin/python3

import os
import re


def readLabel(path):
    if os.path.isfile(path):
        inf = open(path)
        str = ''

        for line in inf:
            str += line

        return str
    else:
        print('File does not exist!')
        return ''


def search():
    regex = re.search(pattern, text, re.MULTILINE)
    if regex:
        print('%03d-%03d: %s' % (regex.start(), regex.end(), regex[0]))
    else:
        print('No match has been found.')


file = r'/mnt/c/dev/repos/glib/pyg/label.txt'
text = readLabel(file)
pattern = '^water'

