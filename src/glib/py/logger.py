import time
import datetime

from threading import Thread

class Logger:
    def __init__(self, filepath=None):
        self._filepath = filepath
        self._end = '\n'

    def log(self, *msg):
        self._print_log(*msg)

    def _print_log(self, *msg, logTypeStr="[  LOG  ]", newline=True):
        time = datetime.datetime.now()

        if newline:
            self._end = '\n'
        else:
            self._end = ''

        print(logTypeStr, time, ":  ", *msg, end=self._end)


logger = Logger()



t1 = time.time()
while time.time() - t1 < 1:
    logger.log("Hello")

