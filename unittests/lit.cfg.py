# *  _ _ _          __        *
# * | (_) |_   ___ / _| __ _  *
# * | | | __| / __| |_ / _` | *
# * | | | |_ | (__|  _| (_| | *
# * |_|_|\__(_)___|_|  \__, | *
# *                    |___/  *
# ===- unittests/lit.cfg.py -----------------------------------------------===//
#
#  Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
#  See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
#  information.
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# ===----------------------------------------------------------------------===//

# Configuration file for the 'lit' test runner.

import os
import lit.formats

# name: The name of this test suite.
config.name = 'pstore-Unit'

# suffixes: A list of file extensions to treat as test files.
config.suffixes = []

# test_source_root: The root path where tests are located.
# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.pstore_obj_root, 'unittests')
config.test_source_root = config.test_exec_root

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.GoogleTest(config.llvm_build_mode, 'unit-tests')

# Propagate the temp directory. Windows requires this because it uses \Windows\
# if none of these are present.
if 'TMP' in os.environ:
    config.environment['TMP'] = os.environ['TMP']
if 'TEMP' in os.environ:
    config.environment['TEMP'] = os.environ['TEMP']
