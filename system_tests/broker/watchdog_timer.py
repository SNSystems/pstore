# *                _       _         _               _   _                      *
# * __      ____ _| |_ ___| |__   __| | ___   __ _  | |_(_)_ __ ___   ___ _ __  *
# * \ \ /\ / / _` | __/ __| '_ \ / _` |/ _ \ / _` | | __| | '_ ` _ \ / _ \ '__| *
# *  \ V  V / (_| | || (__| | | | (_| | (_) | (_| | | |_| | | | | | |  __/ |    *
# *   \_/\_/ \__,_|\__\___|_| |_|\__,_|\___/ \__, |  \__|_|_| |_| |_|\___|_|    *
# *                                          |___/                              *
# ===- system_tests/broker/watchdog_timer.py ------------------------------===//
#  Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
#  All rights reserved.
#
#  Developed by:
#    Toolchain Team
#    SN Systems, Ltd.
#    www.snsystems.com
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the
#  "Software"), to deal with the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
#
#  - Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimers.
#
#  - Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimers in the
#    documentation and/or other materials provided with the distribution.
#
#  - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
#    Inc. nor the names of its contributors may be used to endorse or
#    promote products derived from this Software without specific prior
#    written permission.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#  IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
#  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#  SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
# ===----------------------------------------------------------------------===//
import logging
import threading
import time

class WatchdogTimer(threading.Thread):
    """Run *callback* in *timeout* seconds unless the timer is restarted."""

    def __init__(self, timeout, callback, timer=time.time, args=(), kwargs=None, name=None):
        threading.Thread.__init__(self, name=name)
        self.daemon = True

        self.__timeout = timeout
        self.__callback = callback
        self.__args = args
        self.__kwargs = kwargs if kwargs is not None else {}
        self.__timer = timer
        self.__cancelled = threading.Event()
        self.blocked = threading.Lock()
        self.__deadline = self.__timer() + self.__timeout

    def run(self):
        self.restart()  # don't start timer until `.start()` is called
        # wait until timeout happens or the timer is canceled
        while not self.__cancelled.wait(self.__deadline - self.__timer()):
            # don't test the timeout while something else holds the lock
            # allow the timer to be restarted while blocked
            with self.blocked:
                if self.__deadline <= self.__timer() and not self.__cancelled.is_set():
                    return self.__callback(*self.__args, **self.__kwargs)  # on timeout

    def restart(self):
        """Restart the watchdog timer."""

        self.__deadline = self.__timer() + self.__timeout

    def cancel(self):
        self.__cancelled.set()


def get_process_output(process, watchdog):
    watchdog.restart()
    logging.debug('process readline')
    line = process.stdout.readline(1024)
    rc = process.poll()
    if line == '' and rc is not None:
        raise RuntimeError('process exited ({0})'.format(rc))

    if line and line[-1] == '\n':
        line = line[0:-1]
    logging.info('process said: %s', line)
    return line
