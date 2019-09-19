#!/usr/bin/python3

import os
import pandas as pd
import curses as crs
import threading

from time import sleep
from itertools import cycle
from multiprocessing.dummy import Pool as ThreadPool

def printProgressBar(iteration, total, prefix='', suffix='', decimals=1, length=100, fill='█'):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s [%s] %s%% %s' % (prefix, bar, percent, suffix), end = '\r')
    # Print New Line on Complete
    if iteration == total: 
        print()


class Random(threading.Thread):
    def __init__(self, i):
        super(Random, self).__init__()
        self.total = i

    def run(self):
        for i in range(20000000):
            self.total += 1


class Progress(threading.Thread):
    def __init__(self):
        super(Progress, self).__init__()
        self._do = True

    def run(self):
        s = 0
        for frame in cycle(['   ', '.  ', '.. ', '...']):
            print('\r', frame, ' ', s, sep='', end='', flush=True)
            s += 0.1
            sleep(0.1)
            if self._do == False:
                break

    def stop(self):
        self._do = False


def main(screen):
    crs.curs_set(0)

    screen.addstr('x')
    screen.refresh()
    sleep(0.5)


#if __name__ == '__main__':
    #crs.wrapper(main)

#pool = ThreadPool(2)
#pool.map(write, )

th1 = Random(1)
th1.start()

th2 = Progress()
th2.start()

th1.join()

th2.stop()
th2.join()