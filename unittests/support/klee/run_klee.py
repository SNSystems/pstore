#!/usr/bin/env python

from __future__ import print_function
import argparse
import fnmatch
import os
import re
import subprocess
import sys

import threading
import time
import itertools

class PipeReader (threading.Thread):
    def __init__ (self, pipe):
        super (PipeReader, self).__init__()
        self.__pipe = pipe
        self.__output = ''

    def run (self):
        try:
            for line in iter (self.__pipe.readline, ''):
                if len (line) > 0:
                    line = line.decode ('utf-8')
                    print (line, end='')
                    self.__output += line
                #time.sleep (0.25)
        except ValueError:
            pass

    def close (self):
        self.__pipe.close ()
    def output (self):
        return self.__output


def run_process (args, env={}):
    print ("running:", ' '.join (args))
    child = subprocess.Popen (args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    stdout = PipeReader (child.stdout)
    stdout.start ()
    stderr = PipeReader (child.stderr)
    stderr.start ()

    return_code = child.wait ()
    stdout.close ()
    stderr.close ()
    if return_code != 0:
        raise subprocess.CalledProcessError (return_code, self.__cmd)

    stdout.join ()
    stderr.join ()
    return (stdout.output (), stderr.output ())


def main (argv):
    parser = argparse.ArgumentParser (description='run klee')
    parser.add_argument ('-t', '--ktest-tool', action='store_true')
    parser.add_argument ('executable')
    parser.add_argument ('bcobject', nargs='+')
    args = parser.parse_args (args=argv)

    output_dir = os.path.join (os.path.dirname (args.bcobject [0]), 'klee-last')
    for dir_path, sub_dirs, files in os.walk (output_dir):
        for file_name in fnmatch.filter (files, '*.ktest'):
            test_file = os.path.join (dir_path, file_name)
            if args.ktest_tool:
                _, _ = run_process ([
                    'ktest-tool',
                    '--write-ints',
                    test_file
                ])

            _, _ = run_process ([
                args.executable
            ], env = {'KTEST_FILE': test_file})

    return 0

if __name__ == '__main__':
    sys.exit (main (sys.argv[1:]))
