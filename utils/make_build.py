#!/usr/bin/env python
#*                  _          _           _ _     _  *
#*  _ __ ___   __ _| | _____  | |__  _   _(_) | __| | *
#* | '_ ` _ \ / _` | |/ / _ \ | '_ \| | | | | |/ _` | *
#* | | | | | | (_| |   <  __/ | |_) | |_| | | | (_| | *
#* |_| |_| |_|\__,_|_|\_\___| |_.__/ \__,_|_|_|\__,_| *
#*                                                    *
#===- utils/make_build.py -------------------------------------------------===//
# Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
        _logger.warning('Environment variable "%s" is not defined. Cannot find Visual Studio', env_name)
        return None

    vswhere = os.path.join(program_files, 'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    if not os.path.exists(vswhere):
        _logger.warning('vswhere.exe is not found at "%s"', vswhere)
        return None

    _logger.info('Running vswhere at "%s"', vswhere)
    # Ask vswhere to disclose all of the vs installations.
    installations = json.loads(subprocess.check_output([vswhere, '-format', 'json'], stderr=subprocess.STDOUT))
    # Reduce the installations list to just the integer major version numbers.
    installations = [int(install['installationVersion'].split('.')[0]) for install in installations]
    _logger.debug('Installations are: %s', ','.join((str(inst) for inst in installations)))
    return ['-G', {
        16: 'Visual Studio 16 2019',
        15: 'Visual Studio 15 2017'
    }.get(max(installations)), '-A', 'x64', '-T', 'host=x64']


def _select_generator(system):
    """
    Select the most appropriate generator based on system.

    Will either return None if the default generator is to be used, a real cmake generator name, or a "psuedo"
    generator name. The latter is turned into a collection of cmake switches by _get_real_generator().

    :param system: The name of the target system.
    :return: The name of a cmake generator, a "psuedo" generator, or None if the default generator is preferred.
    """

    return {
        'win32': lambda: 'vs',
        'mac': lambda: 'Xcode',
        'linux': lambda: 'Ninja' if _find_on_path(_add_executable_extension('ninja')) is not None else 'make'
    }.get(system.lower(), lambda: None)()


def _get_real_generator(system, generator):
    if generator is None:
        generator = _select_generator(system)
    if generator is None:
        return None
    return {
        'make': lambda: ['-G', 'Unix Makefiles'],
        'vs': _select_vs_generator,
        'ninja': lambda: ['-G', 'Ninja'],
        'xcode': lambda: ['-G', 'Xcode'],
    }.get (generator.lower(), lambda: ['-G', generator])()


def _options (args):
    platform_name = sys.platform.lower ()
    # Prior to python 3.3, Linux included a version number in its name.
    if platform_name.startswith ('linux'):
        platform_name = 'linux'
    elif platform_name == 'darwin':
        platform_name = 'mac'

    parser = argparse.ArgumentParser (description='Generate a build using cmake')
    parser.add_argument ('build_dir', nargs='?', default=os.getcwd ())
    parser.add_argument ('-s', '--system', default=platform_name)
    parser.add_argument ('-c', '--configuration', default='Debug')
    parser.add_argument ('-o', '--directory', help='The directory in which the build will be created. Normally derived from the system name.')
    parser.add_argument ('-G', '--generator', help='The build system generator which will be used. Automatically derived from the system name.')
    parser.add_argument ('-D', '--define', help='Define a cmake variable as <var>:<type>=<value>', action='append')
    parser.add_argument ('-n', '--dry-run', action='store_true', help='Previews the operation without performing any action.')
    parser.add_argument ('--no-clean', dest='clean', action='store_false')
    options = parser.parse_args (args)

    if options.directory is None:
        options.directory = 'build_' + options.system
    options.generator = _get_real_generator(options.system, options.generator)
    _logger.debug('Selecting generator %s', str(options.generator))
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
    """Splits the search path into a list of the component paths."""

    return search_path.split (os.pathsep)


def _find_on_path (file_name):
    split_search_path = _split_search_path_windows if sys.platform == 'win32' else _split_search_path_generic
    for p in split_search_path (os.environ ['PATH']):
        if len (p) > 0:
            path = os.path.join (p, file_name)
            if os.path.exists (path):
                return path
    return None


def find_cmake ():
    """
    Searches for the cmake executable, returning its path if found or None otherwise.
    """

    cmake_exe = _add_executable_extension ('cmake')
    cmake_path = _find_on_path (cmake_exe)
    if not cmake_path:
        _logger.error ('Could not find %s on the path', cmake_exe)
        return None
    return cmake_path


def rmtree_error (function, path, excinfo):
    """
    Called from shutil.rmtree() if the file removal fails. Makes the file 
    writable before retrying the operation.
    """

    os.chmod (path, stat.S_IWRITE)
    function (path)


def create_build_directory (options):
    if options.clean and os.path.exists (options.directory):
        _logger.info ('rmtree "%s"', options.directory)
        if not options.dry_run:
            shutil.rmtree (options.directory, onerror=rmtree_error)

    if not os.path.exists (options.directory):
        _logger.info ('mkdir:%s', options.directory)
        if not options.dry_run:
            os.mkdir (options.directory)


def build_cmake_command_line (cmake_path, options):
    cmd = [ cmake_path ]
    if options.generator is not None:
        cmd.extend(options.generator)

    # Add the user variable definitions.
    for d in options.define if options.define else []:
        cmd.extend (( '-D', d ))

    # Finally add the build root directory.
    cmd.append (_as_native_path (os.path.abspath (options.build_dir)))
    return cmd


def main (args = sys.argv [1:]):
    exit_code = EXIT_SUCCESS
    logging.basicConfig (level=logging.DEBUG)
    options = _options (args)

    # Check for a cmakelists.txt
    cmakelists_path = os.path.abspath (os.path.join (options.build_dir, 'CMakeLists.txt'))
    _logger.info('Looking for CMakeLists.txt in "%s"', cmakelists_path)
    if not os.path.exists (cmakelists_path):
        _logger.error ('Did not find a CMake CMakeLists.txt file')
        exit_code = EXIT_FAILURE

    cmake_path = None
    if exit_code == EXIT_SUCCESS:
        cmake_path = find_cmake ()
        exit_code = EXIT_FAILURE if cmake_path is None else EXIT_SUCCESS

    if exit_code == EXIT_SUCCESS:
        create_build_directory (options)
        cmd = build_cmake_command_line (cmake_path, options)

        _logger.info ('cwd "%s"', options.directory)
        _logger.info ('Running: %s', _as_command_line (cmd))
        if not options.dry_run:
            status = subprocess.call (cmd, cwd=options.directory)
            if status:
                exit_code = EXIT_FAILURE

    return exit_code


if __name__ == '__main__':
    sys.exit (main ())
