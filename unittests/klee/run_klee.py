#!/usr/bin/env python
# *                     _    _            *
# *  _ __ _   _ _ __   | | _| | ___  ___  *
# * | '__| | | | '_ \  | |/ / |/ _ \/ _ \ *
# * | |  | |_| | | | | |   <| |  __/  __/ *
# * |_|   \__,_|_| |_| |_|\_\_|\___|\___| *
# *                                       *
# ===- unittests/klee/run_klee.py -----------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===//
from __future__ import print_function
import argparse
import fnmatch
import os
import subprocess
import sys
import threading


class PipeReader (threading.Thread):
    def __init__ (self, process, pipe):
        super (PipeReader, self).__init__()
        self.__process = process
        self.__pipe = pipe
        self.__output = ''

    def run (self):
        try:
            while True:
                line = self.__pipe.readline ()
                if len (line) == 0 and self.__process.poll() is not None:
                    break
                if line:
                    line = line.decode ('utf-8')
                    print (line, end='')
                    self.__output += line
        except ValueError:
            pass

    def output (self):
        return self.__output


def find_on_path (name):
    for directory in os.environ.get ('PATH', '').split (':'):
        path = os.path.join (directory, name)
        if os.path.exists (path):
            return path
    raise IOError ("not found")


def run_process (args, env={}):
    print ("\n-- running:", ' '.join (args))
    if len (env) > 0:
        print ("-- environment:", env)

    child = subprocess.Popen (args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    stdout = PipeReader (child, child.stdout)
    stdout.start ()
    stderr = PipeReader (child, child.stderr)
    stderr.start ()

    return_code = child.wait ()
    if return_code != 0:
        raise subprocess.CalledProcessError (return_code, args)

    print ("-- done")
    return (stdout.output (), stderr.output ())


def main (argv):
    parser = argparse.ArgumentParser (description='run klee')
    parser.add_argument ('-t', '--ktest-tool', action='store_true')
    parser.add_argument ('executable')
    parser.add_argument ('bcobject', nargs='+')
    args = parser.parse_args (args=argv)

    ktest_tool_path = find_on_path ('ktest-tool') if args.ktest_tool else None

    output_dir = os.path.join (os.path.dirname (args.bcobject [0]), 'klee-last')
    for dir_path, sub_dirs, files in os.walk (output_dir):
        for file_name in fnmatch.filter (files, '*.ktest'):
            test_file = os.path.join (dir_path, file_name)
            if args.ktest_tool:
                _, _ = run_process ([
                    ktest_tool_path,
                    test_file
                ])

            _, _ = run_process ([
                args.executable
            ], env = {'KTEST_FILE': test_file})

    return 0

if __name__ == '__main__':
    sys.exit (main (sys.argv[1:]))
