#!/usr/bin/env python
# *                  _          _           _ _     _  *
# *  _ __ ___   __ _| | _____  | |__  _   _(_) | __| | *
# * | '_ ` _ \ / _` | |/ / _ \ | '_ \| | | | | |/ _` | *
# * | | | | | | (_| |   <  __/ | |_) | |_| | | | (_| | *
# * |_| |_| |_|\__,_|_|\_\___| |_.__/ \__,_|_|_|\__,_| *
# *                                                    *
# ===- utils/make_build.py ------------------------------------------------===//
#  Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
import argparse
import json
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

_logger = logging.getLogger(__name__)


def _select_vs_generator():
    """
    On Windows, discover the correct generator to request from cmake.

    We must first run the vswhere.exe utility to discover all of the Visual Studio installations
    on the host. From this we then find the latest version and map this to the corresponding
    cmake generator name.

    :return: The name of a cmake generator or None if we were not able to determine the latest
     installed version of Visual Studio.
    """

    env_name = 'ProgramFiles(x86)'
    program_files = os.getenv(env_name)
    if program_files is None:
        _logger.error('Environment variable "%s" is not defined. Cannot find Visual Studio', env_name)
        return None

    vswhere = os.path.join(program_files, 'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    if not os.path.exists(vswhere):
        _logger.error('vswhere.exe is not found at "%s"', vswhere)
        return None

    _logger.info('Running vswhere at "%s"', vswhere)
    # Ask vswhere to disclose all of the vs installations.
    output = subprocess.check_output([vswhere, '-products', '*', '-format', 'json', '-utf8'], stderr=subprocess.STDOUT)
    _logger.info('vswhere said: %s', output)
    installations = json.loads(output)
    if len (installations) == 0:
        _logger.error ('vswhere listed no Visual Studio installations')
        return None

    # Reduce the installations list to just the integer major version numbers.
    installations = [int(install['installationVersion'].split('.')[0]) for install in installations]
    _logger.debug('Installations are: %s', ','.join((str(inst) for inst in installations)))
    return ['-G', {
        16: 'Visual Studio 16 2019',
        15: 'Visual Studio 15 2017'
    }.get(max(installations)), '-A', 'x64', '-T', 'host=x64']


def _get_real_generator_args(system, generator):
    """
    Turns a target system name and optional generator name into a series of CMake switches.

    :param system: The name of the target system.
    :param generator:  The name of the CMake generator (or a "pseudo" generator) or None if the generator is to
                       be derived from the target system name.
    :return: An array of cmake switches.
    """

    if generator is None:
        #    Select the most appropriate generator based on system.
        generator = {
            'linux': lambda: 'Ninja' if _find_on_path(_add_executable_extension('ninja')) is not None else 'make',
            'mac': lambda: 'Xcode',
            'win32': lambda: 'vs',
        }.get(system.lower(), lambda: None)()

    if generator is None:
        _logger.debug('System is unknown ("%s"), no special generator', system)
        return None

    # Turn a generator or "pseudo generator" name into a collection of one or more cmake switches.
    _logger.debug('Turning psuedo generator "%s" into cmake switches', generator)
    return {
        'make': lambda: ['-G', 'Unix Makefiles'],
        'ninja': lambda: ['-G', 'Ninja'],
        'vs': _select_vs_generator,
        'xcode': lambda: ['-G', 'Xcode'],
    }.get(generator.lower(), lambda: ['-G', generator])()


def _options(args):
    platform_name = sys.platform.lower()
    # Prior to python 3.3, Linux included a version number in its name.
    if platform_name.startswith('linux'):
        platform_name = 'linux'
    elif platform_name == 'darwin':
        platform_name = 'mac'

    parser = argparse.ArgumentParser(description='Generate a build using cmake')
    parser.add_argument('build_dir', nargs='?', default=os.getcwd())
    parser.add_argument('-s', '--system', default=platform_name, help='The target system name')
    parser.add_argument('-c', '--configuration', help='The build configuration (Debug/Release)')
    parser.add_argument('-o', '--directory',
                        help='The directory in which the build will be created. Normally derived from the system name.')
    parser.add_argument('-G', '--generator',
                        help='The build system generator which will be used. Automatically derived from the system name.')
    parser.add_argument('-D', '--define',
                        help='Define a cmake variable as <var>:<type>=<value>', action='append', default=[])
    parser.add_argument('-n', '--dry-run',
                        action='store_true', help='Previews the operation without performing any action.')
    parser.add_argument('-v', '--verbose', action='store_true', help='Produce verbose output.')
    parser.add_argument('--no-clean', dest='clean', action='store_false')
    return parser.parse_args(args)


def _process_options(options):
    if options.directory is None:
        options.directory = 'build_' + options.system

    if options.configuration:
        # The configuration value precedes other defines to allow them to override.
        configuration = ['CMAKE_BUILD_TYPE:STRING=' + options.configuration]
        configuration.extend(options.define)
        options.define = configuration

    options.generator = _get_real_generator_args(options.system, options.generator)
    _logger.debug('Selecting generator %s', str(options.generator) if options.generator is not None else "(default)")
    return options

def _as_command_line(cmd):
    """
    Returns an array of arguments into a string which are suitably quoted for the command-shell.

    :param cmd: An array of command-line arguments.
    :return: A string containing the space-separated arguments with quoting as necessary for the shell.
    """

    return ' '.join((pipes.quote(x) for x in cmd))


def _as_native_path_cygwin(path):
    """
    Returns the supplied path in its native form. This is only necessary on cygwin where we can't
    pass a cygwin-style path to the cmake executable.
    """

    cmd = ['/usr/bin/cygpath.exe', '-w', path]
    _logger.info('Converting path to native: %s', _as_command_line(cmd))
    path = subprocess.check_output(cmd).rstrip()
    _logger.info('Path is: "%s"', path)
    return path


def _add_executable_extension_windows(file_name):
    """
    Adds the Windows .exe file extension for an executable if it isn't there already.

    :param file_name: The name of an executable.
    :return: The name of an executable with the .exe extension.
    """

    root, extension = os.path.splitext(file_name)
    if extension != '.exe':
        if extension != '.':
            extension += '.'
        extension += 'exe'
        file_name = root + extension

    return file_name


def _split_search_path_windows(search_path):
    """
    Splits a Windows-style search path into a list of the component paths.

    :param search_path: The search path to be split.
    :return: A list containing each individual path in the search path.
    """

    quoting = False
    result = list()
    current = ''
    for character in search_path:
        if character == '"' and quoting:
            quoting = False
        elif character == '"':
            quoting = True
        elif character == ntpath.pathsep and not quoting:
            # Note that DOS path members may contain environment variables which
            # must be expanded.
            result.append(ntpath.expandvars(current))
            current = ''
        else:
            current += character

    if len(current) > 0:
        current = ntpath.expandvars(current)
        result.append(current)

    return result


_as_native_path = _as_native_path_cygwin if sys.platform == 'cygwin' else lambda path: path

_add_executable_extension = _add_executable_extension_windows if sys.platform == 'win32' else lambda name: name

_split_search_path = _split_search_path_windows if sys.platform == 'win32' else lambda sp: sp.split(os.pathsep)


def _find_on_path(file_name):
    """
    Searches for a file with the supplied name on the search path defined by the PATH environment variable.

    :param file_name:  The file name to be found.
    :return: The path to a file with the supplied name or None if not found.
    """

    for p in _split_search_path(os.environ['PATH']):
        if len(p) > 0:
            path = os.path.join(p, file_name)
            if os.path.exists(path):
                return path
    return None


def find_cmake():
    """
    Searches for the cmake executable, returning its path if found or None otherwise.
    """

    cmake_exe = _add_executable_extension('cmake')
    cmake_path = _find_on_path(cmake_exe)
    if not cmake_path:
        _logger.error('Could not find %s on the path', cmake_exe)
        return None
    return cmake_path


def create_build_directory(options):
    """
    Creates the specified build directory, removing an old copy if present and --no-clean was not specified.

    :param options: The user options.
    :return: Nothing
    """

    if options.clean and os.path.exists(options.directory):
        _logger.info('rmtree "%s"', options.directory)
        if not options.dry_run:
            def rmtree_error(function, path, _):
                """
                Called from shutil.rmtree() if the file removal fails. Makes the file
                writable before retrying the operation.
                """

                os.chmod(path, stat.S_IWRITE)
                function(path)

            shutil.rmtree(options.directory, onerror=rmtree_error)

    if not os.path.exists(options.directory):
        _logger.info('mkdir:%s', options.directory)
        if not options.dry_run:
            os.mkdir(options.directory)


def build_cmake_command_line(cmake_path, options):
    cmd = [cmake_path]
    cmd.extend(options.generator if options.generator else [])

    # Add the user variable definitions.
    cmd.extend('-D' + d for d in options.define)

    # Finally add the build root directory.
    cmd.append(_as_native_path(os.path.abspath(options.build_dir)))
    return cmd


def check_for_cmakelists(options):
    cmakelists_path = os.path.abspath(os.path.join(options.build_dir, 'CMakeLists.txt'))
    _logger.info('Looking for CMakeLists.txt in "%s"', cmakelists_path)
    if not os.path.exists(cmakelists_path):
        _logger.error('Did not find a CMake CMakeLists.txt file')
        return ''
    return cmakelists_path


def run_cmake(options, cmake_path):
    create_build_directory(options)
    cmd = build_cmake_command_line(cmake_path, options)
    _logger.info('cd "%s"', options.directory)
    _logger.info('Running: %s', _as_command_line(cmd))
    if options.dry_run:
        return True
    return subprocess.call(cmd, cwd=options.directory) == 0


def bind(x, f):
    """
    A monadic bind operation similar to Haskell's Maybe type. Used to enable function
    composition where a function returning a false-like value indicates failure.

    :param x: The input value passed to callable f is not None.
    :param f: A callable which is passed 'x'
    :return: If 'x' is a false value on input, 'x' otherwise the result of calling f(x).
    """

    return f(x) if x else x


def main(args=sys.argv[1:]):
    options = _options(args)
    logging.basicConfig(level=logging.DEBUG if options.verbose else logging.INFO)
    options = _process_options (options)
    _logger.debug('Defines are: %s', str(options.define))

    return bind(check_for_cmakelists(options),
                lambda _: bind(find_cmake(),
                               lambda cmake_path: run_cmake(options, cmake_path)))


if __name__ == '__main__':
    sys.exit(EXIT_SUCCESS if main() else EXIT_FAILURE)
