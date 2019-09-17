#!/usr/bin/env python
# *  _                       _              *
# * | | __ _ _   _  ___ _ __(_)_ __   __ _  *
# * | |/ _` | | | |/ _ \ '__| | '_ \ / _` | *
# * | | (_| | |_| |  __/ |  | | | | | (_| | *
# * |_|\__,_|\__, |\___|_|  |_|_| |_|\__, | *
# *          |___/                   |___/  *
# ===- utils/layering/layering.py -----------------------------------------===//
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
"""
A utility which verifies the pstore project's layering. That is, it checks that the
dependencies defined by the cmake source files matches the dependencies used by the
#include statements in the source files.

With the working directory at the project's root, run cmake with the --graphiz=xx option
to produce the project's dependency graph:

$ mkdir dep
$ cd dep
$ cmake --graphviz=pstore.dot ..
$ cd -

The with working directory again at the project's root, run layering.py:

$ ./utils/layering/layering.py dep/pstore.dot

Dependencies:
$ pip install networkx
$ pip install decorator
$ pip install pydot

"""

import argparse
import logging
import os
from pprint import pformat
import re
import sys

import networkx as nx

EXIT_SUCCESS = 0
EXIT_FAILURE = 1


def bind(x, f):
    """
    A monadic bind operation similar to Haskell's Maybe type. Used to enable function 
    composition where a function returning None indicates failure.

    :param x: The input value passed to callable f is not None.
    :param f: A callable which is passed 'x'
    :return: If 'x' is None on input, None otherwise the result of calling f(x).
    """

    if x is None:
        return None
    return f(x)


def include_files(path):
    """
    A generator which produces the #includes from the file at the given path.

    :param path:
    """

    with open(path, 'r') as f:
        for line in f:
            # Don't match up to the end of the line to allow for comments.
            r = re.match(r'\s*#\s*include\s*["<](.*)[">]', line)
            if r:
                yield r.group(1)


def split_all(path):
    """
    Splits a path into all of its individual components.

    :param path:
    :return: A list of path components.
    """

    allparts = []
    while True:
        # Split the path into (head,tail)
        parts = os.path.split(path)
        if parts[0] == path:  # sentinel for absolute paths
            allparts.insert(0, parts[0])
            break
        if parts[1] == path:  # sentinel for relative paths
            allparts.insert(0, parts[1])
            break
        allparts.insert(0, parts[1])
        path = parts[0]
    return allparts


EXCLUSIONS = ('cmakecxxcompilerid.cpp',)


def dependencies_from_source(source_directory):
    """
    Scans the source files contained within the directory hierarchy rooted at 'source_path' and returns a dictionary
    whose keys are the source file paths and where the corresponding value is a set of components referenced by that
    source file.

    :param source_directory:  The path of a directory containing source files (files whose extension is .hpp or .cpp)
    :return: A dictionary with the source file to component name mapping.
    """

    def sources_in(directory, extensions):
        """
        A generator which yields the relative paths of source files in the given directory. Only files whose
        extension is in the given by the 'extensions' parameter are returned.

        :param directory:
        :param extensions: A list of the file extensions to be included in the results.
        """

        for root, dirs, files in os.walk(directory):
            for name in files:
                lower_name = name.lower()
                if lower_name.endswith(extensions) and not lower_name in EXCLUSIONS:
                    yield os.path.relpath(os.path.join(root, name))

    def includes(path):
        """
        Returns the set of components referenced by the includes of the source file at 'path'.

        :param path: The path to the source file to be scanned.
        :return: A set of component names.
        """

        # We're include interested in includes with the pstore/ prefix.
        pstore_includes = [x for x in include_files(path) if split_all(x)[0] == 'pstore']

        # The pstore component is the path component _after_ the initial pstore/. Target names use dashes to separate
        #  words, whereas paths use underscores.
        includes_without_prefix = [split_all(x)[1].replace('_', '-') for x in pstore_includes]

        # Convert the include path to the cmake component name. This works around inconsistencies in the way that
        # targets and directories are named.
        return frozenset([{
                              'diff': 'diff-lib',
                              'dump': 'dump-lib',
                              'json': 'json-lib',
                              'vacuum': 'vacuum-lib',
                          }.get(x, x) for x in includes_without_prefix])

    return dict((path, includes(path)) for path in sources_in(source_directory, extensions=('.hpp', '.cpp')))


def cmake_dependency_graph(dot_path):
    """
    Parse the graphviz dot file at the path given by dot_path and return a networkx directed graph instance that
    reflects its contents.

    :param dot_path: The path of the file to be parse.
    :return: A networkx directed graph.
    """

    g = nx.drawing.nx_pydot.read_dot(dot_path)
    if not nx.algorithms.dag.is_directed_acyclic_graph(g):
        return None
    return g.to_directed()


PREFIX = 'pstore-'


def label(node_data):
    s = node_data['label'].strip('"')
    return s[len(PREFIX):] if s.startswith(PREFIX) else s


GOOGLE_TEST = ('gtest', 'gtest_main', 'gmock', 'gmock_main')


def reachability_dict(dag):
    """
    Scans through the DAG vertices. For each node, get the list of descendants (that is the collection of vertices
    that are transitively reachable from that node). Nodes that name one of the google-test/mock components are stripped.

    :param dag:
    :return: A dictionary whose keys are the target name and values are the set of targets that may be referenced
    from the corresponding target.
    """

    nodes = dag.nodes(data=True)

    return dict((label(data),
                 frozenset([x for x in [label(nodes[x]) for x in nx.algorithms.descendants(dag, node)] if
                            x not in GOOGLE_TEST]))
                for node, data in nodes if label(data) not in GOOGLE_TEST)


def cmake_target_from_path(p):
    """
    Converts a file path to a cmake target name.

    :param p: The path to be converted.
    :return: The cmake target name.
    """

    class Group:
        def __init__(self): pass

        lib = 1
        unit_test = 2
        tool = 3
        examples = 4

    def name_from_path(path):
        """
        Derives a component name from a path.

        :param path: The path to be examined.
        :return: A two-tuple: the first member is the base component name; the second member is the component group.
        """
        parts = split_all(path)

        # Special handling for the unit test harness code.
        if parts == ['unittests', 'harness.cpp']:
            return 'unit-test-harness', Group.unit_test

        if parts[:1] == ['lib']:
            return parts[1], Group.lib
        if parts[:2] == ['include', 'pstore']:
            return parts[2], Group.lib
        if parts[:1] == ['unittests']:
            return parts[1], Group.unit_test
        if parts[:1] == ['tools']:
            return parts[1], Group.tool
        if parts[:1] == ['examples']:
            # Just skip the examples for the time being.
            # return (parts[1], Group.examples)
            return None
        # Skip paths that we don't know about.
        return None

    def name_to_target(name):
        """
        Converts a two-tuple (component base name, component group) to its corresponding cmake target name.

        :param name:  A two-tuple defining the base component name and component group.
        :return:  The cmake target name (without the 'pstore-' prefix)
        """
        return {
            ('broker', Group.unit_test): 'broker-unit-tests',
            ('cmd-util', Group.unit_test): 'cmd-util-unit-tests',
            ('core', Group.unit_test): 'core-unit-tests',
            ('diff', Group.unit_test): 'diff-unit-tests',
            ('dump', Group.unit_test): 'dump-unit-tests',
            ('httpd', Group.unit_test): 'httpd-unit-tests',
            ('json', Group.unit_test): 'json-unit-tests',
            ('mcrepo', Group.unit_test): 'mcrepo-unit-tests',
            ('serialize', Group.unit_test): 'serialize-unit-tests',
            ('support', Group.unit_test): 'support-unit-tests',
            ('vacuum', Group.lib): 'vacuum-lib',
            ('vacuum', Group.tool): 'vacuumd',
            ('vacuum', Group.unit_test): 'vacuum-unit-tests',
        }.get(name, name[0])

    # Produce a component name from a path and convert '_' to '-' to match the convention used by the cmake targets.
    return bind(bind(bind(
        p,
        name_from_path),
        lambda x: (x[0].replace('_', '-'), x[1])),
        name_to_target)


def logging_config():
    logger = logging.getLogger(__name__)

    class LessThanFilter(logging.Filter):
        def __init__(self, max_level, name=""):
            super(LessThanFilter, self).__init__(name)
            self.__max_level = max_level

        def filter(self, record):
            # Is the specified record to be logged? Zero for no, non-zero for yes.
            return 1 if record.levelno < self.__max_level else 0

    formatter = logging.Formatter('%(levelname)s: %(message)s')

    logging_handler_out = logging.StreamHandler(sys.stdout)
    logging_handler_out.setLevel(logging.DEBUG)
    logging_handler_out.addFilter(LessThanFilter(logging.WARNING))
    logging_handler_out.setFormatter(formatter)
    logger.addHandler(logging_handler_out)

    logging_handler_err = logging.StreamHandler(sys.stderr)
    logging_handler_err.setLevel(logging.WARNING)
    logging_handler_err.setFormatter(formatter)
    logger.addHandler(logging_handler_err)
    return logger


def main(args=sys.argv[1:]):
    exit_code = EXIT_SUCCESS
    logger = logging_config()

    parser = argparse.ArgumentParser(description='layering check')
    parser.add_argument('dependencies', help='The cmake-generated dot dependency graph.')
    parser.add_argument('-s', '--source-dir', default='.', help='The project root directory.')
    parser.add_argument('-v', '--verbose', default=0, action='count', help='Produce more verbose output')
    options = parser.parse_args(args)

    # Set the visible log level according to the number of times that -v was specified by the user.
    logger.setLevel({
                        0: logging.WARNING,
                        1: logging.INFO,
                        2: logging.DEBUG,
                    }.get(options.verbose, logging.DEBUG))

    # Parse the dot graph produced by cmake so that we know the project's dependency graph.
    logger.info("Scanning cmake depenedency graph")
    dag = cmake_dependency_graph(options.dependencies)
    if dag is None:
        logger.error('The cmake dependency graph is not a DAG. Giving up.')
        return EXIT_FAILURE

    # For each target in the graph, build the set of targets against which it transitively links.
    logger.info('Building reachability dictionary')
    reachable = reachability_dict(dag)
    logger.debug(pformat(reachable))

    # Scan the source and header files discovering the files that they include.
    logger.info("Discovering source code dependencies")
    includes = dependencies_from_source(options.source_dir)

    # Check that the source code includes don't violate the constraints from the cmake dependency graph.
    logger.info("Checking dependencies")
    for path, dependencies in includes.items():
        logger.info('checking: "%s"', path)
        c = cmake_target_from_path(path)
        if c is None:
            logger.warning('skipping: "%s"', path)
            continue
        if reachable.get(c) is None:
            logger.error('unknown target: "%s"', path)
            exit_code = EXIT_FAILURE
            continue

        logger.debug('component: "%s"', c)
        logger.debug('reachable (from cmake): %s', reachable[c])
        logger.debug('included (by source code): %s', dependencies)
        for dependent in dependencies:
            # The "config" psuedo component is for the config.hpp file generated when running cmake. It's a pure header
            # file so there is no library to link.
            if dependent == 'config':
                continue

            if dependent != c and dependent not in reachable[c]:
                logger.error('cannot include from component "%s" from file "%s" (component "%s")', dependent, path, c)
                exit_code = EXIT_FAILURE

    return exit_code


if __name__ == '__main__':
    logging.getLogger().setLevel(logging.NOTSET)
    sys.exit(main())
