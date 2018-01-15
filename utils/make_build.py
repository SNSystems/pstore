#!/usr/bin/env python
#*                  _          _           _ _     _  *
#*  _ __ ___   __ _| | _____  | |__  _   _(_) | __| | *
#* | '_ ` _ \ / _` | |/ / _ \ | '_ \| | | | | |/ _` | *
#* | | | | | | (_| |   <  __/ | |_) | |_| | | | (_| | *
#* |_| |_| |_|\__,_|_|\_\___| |_.__/ \__,_|_|_|\__,_| *
#*                                                    *
#===- utils/make_build.py -------------------------------------------------===//
# Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
# All rights reserved.
#
# Developed by:
#   Toolchain Team
#   SN Systems, Ltd.
#   www.snsystems.com
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal with the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimers.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimers in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
#   Inc. nor the names of its contributors may be used to endorse or
#   promote products derived from this Software without specific prior
#   written permission.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
#===----------------------------------------------------------------------===//
import argparse
import logging
import os
import pipes
import shutil
import subprocess
import stat
import sys

if sys.platform == 'win32':
    import ntpath

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

PLATFORM_TO_DIR_MAP = {
    'darwin': 'mac',
}

PLATFORM_TO_GENERATOR_MAP = {
    'darwin': 'Xcode',
    'linux': 'Ninja',
    'win32': 'Visual Studio 15 2017 Win64',
}

_logger = logging.getLogger (__name__)

def _options (args):
    platform_name = sys.platform
    # Prior to python 3.3, Linux included a version number in its name.
    if platform_name.startswith ('linux'):
        platform_name = 'linux'

    parser = argparse.ArgumentParser (description='Generate a build using cmake')
    parser.add_argument ('-s', '--system', default=platform_name)
    parser.add_argument ('-c', '--configuration', default='Debug')
    parser.add_argument ('-o', '--directory', help='The directory in which the build will be created. Normally derived from the system name.')
    parser.add_argument ('-G', '--generator', help='The build system generator which will be used. Automatically derived from the system name.')
    parser.add_argument ('-D', '--define', help='Define a cmake variable as <var>:<type>=<value>', action='append')
    parser.add_argument ('-n', '--dry-run', action='store_true', help='Previews the operation without performing any action.')
    parser.add_argument ('--no-clean', dest='clean', action='store_false')
    options = parser.parse_args (args)


    if options.directory is None:
        options.directory = 'build_' + PLATFORM_TO_DIR_MAP.get (options.system, options.system)
    if options.generator is None:
        options.generator = PLATFORM_TO_GENERATOR_MAP.get (options.system, 'Unix Makefiles')
    return options


def _get_log_level (options):
    return logging.INFO if options.verbose else logging.WARNING


def _as_command_line (cmd):
    return ' '.join (( pipes.quote (x) for x in cmd ))


def _as_native_path (path):
    """Returns the supplied path in its native form. This is only necessary on cygwin where we can't
    pass a cygwin-style path to the cmake executable.
    """

    if sys.platform == 'cygwin':
        cmd = [ '/usr/bin/cygpath.exe', '-w', path ]
        _logger.info ('Converting path to native: %s', _as_command_line (cmd))
        path = subprocess.check_output (cmd).rstrip ()
        _logger.info ('Path is: "%s"', path)

    return path


def _add_executable_extension (file_name):
    """Adds the file extension for an executable, if the host system
    requires one."""

    if sys.platform == 'win32':
        # If we're running on Windows, then we need to add the ".exe"
        # suffix to the name of the executable if it isn't there already
        # (and it shouldn't be, for the sake of portability).
        root, extension = os.path.splitext (file_name)
        if extension != '.exe':
            if extension != '.':
                extension += '.'
            extension += 'exe'
            file_name = root + extension

    return file_name


def _split_search_path_windows (search_path):
    """Splits a Windows-style search path into a list of the component paths.

    Arguments:
        The search path to be split.

    Returns:
        A list containing each individual path in the search path.
    """

    quoting = False
    result = list ()
    current = ''
    for character in search_path:
        if   character == '"' and quoting:
            quoting = False
        elif character == '"':
            quoting = True
        elif character == ntpath.pathsep and not quoting:
            # Note that DOS path members may contain environment variables which
            # must be expanded.
            current = ntpath.expandvars (current)
            result.append (current)
            current = ""
        else:
            current += character

    if len (current) > 0:
        current = ntpath.expandvars (current)
        result.append (current)

    return result


def _split_search_path_generic (search_path):
    """Splits the search path into a list of the component paths.'''

    Arguments:
        The search path to be split.

    Returns:
        A list containing each individual path in the search path.
    """

    # On Unix-based systems, the path separator character cannot be escaped
    # even if it is part of the filename.
    result = search_path.split (os.pathsep)
    #FIXME: what's the reason for these lines?
    if result [-1] == '':
        result = result [:-1]
    return result





def _find_on_path (file_name):
    split_search_path = _split_search_path_windows if sys.platform == 'win32' else _split_search_path_generic
    for p in split_search_path (os.environ ['PATH']):
        if len (p) > 0:
            path = os.path.join (p, file_name)
            if os.path.exists (path):
                return path
    return None


def _use_build_type (generator):
    """Returns True if the named cmake generator may be a single-configuration generator.
    A return value of False means that an explicit CMAKE_BUILD_TYPE variable will only result
    in a warning, so should not be added.

    Not that a True return doesn't definitively mean that the warning will not be issued. This
    utility only knows about the common multi-configuration generators.
    """

    result = True
    if generator == 'Xcode':
        result = False
    elif generator.startswith ('Visual Studio'):
        result = False
    return result



def rmtree_error (function, path, excinfo):
    '''Called from shutil.rmtree() if the file removal fails. Makes the file 
    writable before retrying the operation.'''

    os.chmod (path, stat.S_IWRITE)
    function (path)


def main (args = sys.argv [1:]):
    exit_code = EXIT_SUCCESS
    logging.basicConfig (level=logging.DEBUG)
    options = _options (args)

    # Check for a cmakelists.txt
    if not os.path.exists ('CMakeLists.txt'):
        _logger.error ('Did not find a CMake CMakeLists.txt file')
        exit_code = EXIT_FAILURE

    cmake_path = None
    if exit_code == EXIT_SUCCESS:
        cmake_exe = _add_executable_extension ('cmake')
        cmake_path = _find_on_path (cmake_exe)
        if not cmake_path:
            _logger.error ('Could not find %s on the path', cmake_exe)
            exit_code = EXIT_FAILURE

    if exit_code == EXIT_SUCCESS:
        _logger.info ('Build directory is "%s"', options.directory)
        if os.path.exists (options.directory):
            if options.clean:
                _logger.info ('rmtree "%s"', options.directory)
                if not options.dry_run:
                    shutil.rmtree (options.directory, onerror=rmtree_error)

        if not os.path.exists (options.directory):
            _logger.info ('mkdir:%s', options.directory)
            if not options.dry_run:
                os.mkdir (options.directory)

        cmd = [
            cmake_path,
            '-G', options.generator,
        ]

        # Don't add CMAKE_BUILD_TYPE for the multi-configuration generators that
        # we know about. It avoids an unecessary warning from cmake.
        if _use_build_type (options.generator):
            cmd.extend (('-D', 'CMAKE_BUILD_TYPE:STRING=' + options.configuration))
        else:
            _logger.warning ('Build configuration was ignored for a multi-configuration generator')

        # Add the user variable definitions.
        if options.define:
            for d in options.define:
                cmd.extend (( '-D', d ))

        vs = 'Visual Studio '
        if options.generator [:len (vs)] == vs:
            cmd.extend (('-T', 'host=x64'))

        # Finally add the build root directory.
        cmd.append (_as_native_path (os.getcwd ()))

        _logger.info ('cwd "%s"', options.directory)
        _logger.info ('Running: %s', _as_command_line (cmd))
        if not options.dry_run:
            subprocess.call (cmd, cwd=options.directory)

    return exit_code


if __name__ == '__main__':
    sys.exit (main ())
# eof: utils/make_build.py
