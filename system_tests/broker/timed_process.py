# ===- system_tests/broker/timed_process.py -------------------------------===//
# *  _   _                    _                                      *
# * | |_(_)_ __ ___   ___  __| |  _ __  _ __ ___   ___ ___  ___ ___  *
# * | __| | '_ ` _ \ / _ \/ _` | | '_ \| '__/ _ \ / __/ _ \/ __/ __| *
# * | |_| | | | | | |  __/ (_| | | |_) | | | (_) | (_|  __/\__ \__ \ *
# *  \__|_|_| |_| |_|\___|\__,_| | .__/|_|  \___/ \___\___||___/___/ *
# *                              |_|                                 *
# ===----------------------------------------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===//

import errno
import subprocess
import threading

# The default process timeout in seconds
DEFAULT_PROCESS_TIMEOUT = 3 * 60.0


class TimedProcess(threading.Thread):
    def __init__(self, args, timeout=DEFAULT_PROCESS_TIMEOUT, name='TimedProcess', creation_flags=0):
        threading.Thread.__init__(self, name=name)
        self.__stdout = ''
        self.__stderr = ''
        self.__did_timeout = False
        self.__timer = threading.Timer(timeout, self.timeout)

        self.__cmd = args
        self.__creation_flags = creation_flags

        self.__process = None
        # If an exception is thrown in TimedProcess.run(), it is recorded here so that it
        # can be discovered by the main thread.
        self.__exception = None

    def run(self):
        """Runs the process, recording everything written to stdout."""

        try:
            self.__output = ''
            self.__process = subprocess.Popen(args=self.__cmd, creationflags=self.__creation_flags,
                                              stdout=subprocess.PIPE, universal_newlines=True)
            self.__timer.start()

            (self.__stdout, self.__stderr) = self.__process.communicate()
            return_code = self.__process.returncode
            if return_code != 0:
                raise subprocess.CalledProcessError(return_code, self.__cmd)
        except Exception as ex:
            self.__exception = ex
        #            raise
        finally:
            if self.__timer is not None:
                self.__timer.cancel()

    def output(self):
        """After the thread has joined (and hence the process has exited),
        returns everything that it wrote to stdout."""

        return self.__stdout

    def exception(self):
        return self.__exception

    def did_timeout(self):
        return self.__did_timeout

    def timeout(self):
        """Injects a timeout to a running process, forcing it to exit."""

        self.__timer.cancel()
        if self.__process is not None and self.__process.poll() is None:
            try:
                self.__process.kill()
                self.__did_timeout = True
            except OSError as e:
                if e.errno != errno.ESRCH:
                    raise

    def send_signal(self, signal):
        """Sends a signal to the child process."""

        if self.__process:
            self.__process.send_signal(signal)

    def __execute(self):
        output = self.__process.stdout
        for line in iter(output.readline, ""):
            yield line
        output.close()
        return_code = self.__process.wait()
        if return_code != 0:
            raise subprocess.CalledProcessError(return_code, self.__cmd)
