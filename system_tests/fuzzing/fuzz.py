#!/usr/bin/env python
# *   __                *
# *  / _|_   _ ________ *
# * | |_| | | |_  /_  / *
# * |  _| |_| |/ / / /  *
# * |_|  \__,_/___/___| *
# *                     *
# ===- system_tests/fuzzing/fuzz.py ---------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ===----------------------------------------------------------------------===//
from __future__ import print_function

import argparse
import errno
import os
import platform
import shutil
import subprocess
import tempfile

# The extra fluff that must be appended to the name of an executable program on Windows
xtension = '.exe' if platform.system () == 'Windows' else ''

def _setup (binary_dir, temp_dir, src_file):
    write_tool = os.path.abspath (os.path.join (binary_dir, 'pstore-write' + xtension))
    setup_commands = [
        [ write_tool, '--compact=disabled', '--add=key1,value1', src_file ],
        [ write_tool, '--compact=disabled', '--add=key2,value2', src_file ],
    ]

    try:
        os.unlink (src_file)
    except OSError as osex:
        # do nothing if the file wasn't present.
        if osex.errno != errno.ENOENT:
            raise

    print ("Performing setup")
    for command in setup_commands:
        print (' '.join (command))
        subprocess.check_call (command, executable = command [0])
    print ("Setup complete")


def run_tests (name, binary_dir, temp_dir, iterations, args):
    print ('Running tests for {0}'.format (name))
    print ('binary_dir = "{0}"'.format (binary_dir))
    print ('temp_dir = "{0}"'.format (temp_dir))
    print ('iterations = {0}'.format (iterations))
    print ('args = {0}'.format (args))

    src_file = os.path.join (temp_dir, name + '-src')
    fuzz_file = os.path.join (temp_dir, name + '-dest')

    _setup (binary_dir, temp_dir, src_file)

    args.append (fuzz_file)

    for count in range (1, iterations + 1):
        print ("Run {0}".format (count))

        # Starting with our "known good" file (src_file), make a copy so that
        # the mangler can do its evil work.
        shutil.copyfile (src_file, fuzz_file)

        # Fuzz the file
        mangle_exe = os.path.join (binary_dir, 'pstore-mangle' + xtension)

        try:
            subprocess.check_call ([ mangle_exe, fuzz_file ],
                                   executable = mangle_exe,
                                   shell = False)
        except subprocess.CalledProcessError as ex:
            print ("args {0}, return code={1}".format (' '.join (args), ex.returncode))
            if ex.returncode != 1:
                raise

        print ('Done')
        # Now run the tool under test. It's allowed to return 0 or 1 as success
        # or controlled failure. Anything else is a test failure.
        try:
            subprocess.check_call (args,
                                   executable = args [0],
                                   shell = False)
        except subprocess.CalledProcessError as ex:
            print ("args {0}, return code={1}".format (' '.join (args), ex.returncode))
            if ex.returncode != 1:
                raise
        except OSError as osex:
            print ('An OSError was raised by subprocess.check_call()')
            print ('Arguments were: {0}'.format (args))
            print ("osex.keys()=" % dir(osex))
            print ('filename="{0}"'.format (osex.filename))
            raise


def main ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ('name', nargs = 1, help = 'The name of the test.')
    parser.add_argument ('args', nargs = '+', help = 'The command to run.')
    parser.add_argument ('--iterations', default = 100, help = 'The number of times that the data is fuzzed')
    parser.add_argument ('--temp-dir', default = tempfile.gettempdir (), help = 'The directory use for test files')
    parser.add_argument ('--binary-dir', default = '.', help = 'The directory in which binaries can be found')
    args = parser.parse_args ()
    command = args.args
    command [0] += xtension
    run_tests (args.name [0], args.binary_dir, args.temp_dir, args.iterations, command)

if __name__ == '__main__':
    main ()
