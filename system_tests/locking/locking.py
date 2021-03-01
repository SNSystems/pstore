#!/usr/bin/env python
# ===- system_tests/locking/locking.py ------------------------------------===//
# *  _            _    _              *
# * | | ___   ___| | _(_)_ __   __ _  *
# * | |/ _ \ / __| |/ / | '_ \ / _` | *
# * | | (_) | (__|   <| | | | | (_| | *
# * |_|\___/ \___|_|\_\_|_| |_|\__, | *
# *                            |___/  *
# ===----------------------------------------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===//
from __future__ import print_function
import argparse
import logging
import os.path
import subprocess
import sys
import threading
import time

EXIT_SUCCESS = 0
EXIT_FAILURE = 1


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


def start_lock_test_process(binaries, database):
    args = [os.path.join(binaries, 'pstore-lock-test'), database]
    logging.info('start process: %s', str(args))
    process = subprocess.Popen(args,
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               universal_newlines=True)
    logging.debug('process ID=%d', process.pid)
    return process


class State:
    def __init__(self):
        self.p1_holding_lock = threading.Event()
        self.p2_was_blocked = threading.Event()
        self.p1_released_lock = threading.Event()

        self.exit_code = EXIT_SUCCESS

    def check_companion_thread(self):
        if self.exit_code != EXIT_SUCCESS:
            raise RuntimeError('an error in the companion thread')


def get_process_output(state, process, watchdog):
    watchdog.restart()
    logging.debug('process readline')
    state.check_companion_thread()
    line = process.stdout.readline(1024)
    state.check_companion_thread()
    rc = process.poll()
    if line == '' and rc is not None:
        raise RuntimeError('process exited ({0})'.format(rc))

    if line and line[-1] == '\n':
        line = line[0:-1]
    logging.info('process said: %s', line)
    return line


def send(process):
    logging.info('sending to stdin')
    process.stdin.write('a\n')
    process.stdin.flush()
    logging.info('sent')


def kill(state, process):
    logging.error('Watchdog timeout: killing process')
    process.kill()
    state.exit_code = EXIT_FAILURE


def wait(state, event):
    state.check_companion_thread()
    while not event.is_set() and state.exit_code == EXIT_SUCCESS:
        event.wait(TIMEOUT)
        logging.debug('iterate')
        state.check_companion_thread()
    state.check_companion_thread()


TIMEOUT = 60
PAUSE = 2


def run_p1(state, binaries, database):
    try:
        process = start_lock_test_process(binaries, database)
        watchdog = WatchdogTimer(timeout=TIMEOUT, callback=kill, args=(state, process,), name='p1-watchdog')
        try:
            watchdog.start()

            l1 = get_process_output(state, process, watchdog)
            if l1 != 'start':
                raise RuntimeError('process stdout should have been "start"')

            out1 = get_process_output(state, process, watchdog)
            if out1 != 'pre-lock':
                raise RuntimeError('process stdout should have been "pre-lock", got "{0}"'.format(out1))
            send(process)

            out2 = get_process_output(state, process, watchdog)
            while out2 == 'blocked':
                out2 = get_process_output(state, process, watchdog)
            if out2 != 'holding-lock':
                raise RuntimeError('process stdout should have been "holding-lock", got "{0}"'.format(out2))

            logging.info('notifying that we saw "holding-lock"')
            state.p1_holding_lock.set()

            # Wait for the other process to say "blocked"
            logging.info('wait for p2 to see "blocked"')
            wait(state, state.p2_was_blocked)

            (stdout, stderr) = process.communicate("a\n")
            if stdout != 'done\n':
                raise RuntimeError('process stdout should have been "done", got "{0}"'.format(stdout))
            if stderr is not None:
                raise RuntimeError('stderr contained: "{0}"'.format(stderr))

            logging.info('done')
            state.p1_released_lock.set()
        finally:
            watchdog.cancel()
    except Exception as ex:
        logging.error(str(ex), exc_info=ex)
        state.exit_code = EXIT_FAILURE


def run_p2(state, binaries, database):
    try:
        time.sleep(1)
        process = start_lock_test_process(binaries, database)
        watchdog = WatchdogTimer(timeout=TIMEOUT, callback=kill, args=(state, process,), name='p2-watchdog')
        try:
            watchdog.start()

            l1 = get_process_output(state, process, watchdog)
            if l1 != 'start':
                raise RuntimeError('process stdout should have been "start"')

            l1 = get_process_output(state, process, watchdog)
            if l1 != 'pre-lock':
                raise RuntimeError('process stdout should have been "pre-lock"')

            # wait for the companion thread to see "holding lock"
            logging.info('waiting for p1 to see "holding-lock"')
            wait(state, state.p1_holding_lock)

            logging.info('got it.')
            send(process)

            # We need to "blocked" indicating that another process has the lock.
            out2 = get_process_output(state, process, watchdog)
            while out2 == 'blocked':
                state.p2_was_blocked.set()
                out2 = get_process_output(state, process, watchdog)

            logging.info('waiting for p1 to release the lock')
            wait(state, state.p1_released_lock)

            if out2 != 'holding-lock':
                raise RuntimeError('process stdout should have been "holding-lock", got "{0}"'.format(out2))

            (stdout, stderr) = process.communicate("a\n")
            if stdout != 'done\n':
                raise RuntimeError('process stdout should have been "done", got "{0}"'.format(stdout))
            if stderr is not None:
                raise RuntimeError('stderr contained: "{0}"'.format(stderr))
            logging.info('done')
        finally:
            watchdog.cancel()
    except Exception as ex:
        logging.error(str(ex), exc_info=ex)
        state.exit_code = EXIT_FAILURE


def test(binaries, database):
    """
    Run the test.

    :param binaries: The directory containing the pstore executables.
    :param database: The path of the database file to be used for the test
    :return: EXIT_SUCCESS if the test is successful, EXIT_FAILURE otherwise.
    """
    state = State()
    kwargs = {
        'state': state,
        'binaries': binaries,
        'database': database
    }
    t1 = threading.Thread(target=run_p1, kwargs=kwargs, name='run_p1')
    t2 = threading.Thread(target=run_p2, kwargs=kwargs, name='run_p2')
    t1.start()
    t2.start()
    logging.debug('joining run_p1')
    t1.join()
    logging.debug('joining run_p2')
    t2.join()
    logging.debug('done')
    return state.exit_code


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG,
                        format='%(levelname)s - %(asctime)s - %(threadName)s - %(filename)s:%(lineno)d - %(message)s')
    logging.debug('Starting')
    parser = argparse.ArgumentParser('locking test tool')
    parser.add_argument('binaries', help='Directory containing the pstore binaries')
    parser.add_argument('database', help='Path of the database file used for the test')
    options = parser.parse_args()

    exit_code = test(options.binaries, options.database)

    logging.info('Exit-code is %d', exit_code)
    logging.shutdown()

    sys.exit(exit_code)
