# *  _   _                    _                                      *
# * | |_(_)_ __ ___   ___  __| |  _ __  _ __ ___   ___ ___  ___ ___  *
# * | __| | '_ ` _ \ / _ \/ _` | | '_ \| '__/ _ \ / __/ _ \/ __/ __| *
# * | |_| | | | | | |  __/ (_| | | |_) | | | (_) | (_|  __/\__ \__ \ *
# *  \__|_|_| |_| |_|\___|\__,_| | .__/|_|  \___/ \___\___||___/___/ *
# *                              |_|                                 *
# ===- system_tests/broker/timed_process.py -------------------------------===//
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
