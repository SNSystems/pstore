# Check the internal shell handling component of the ShTest format.
#
# RUN: not %{lit} -j 1 -v %{inputs}/shtest-shell > %t.out
# FIXME: Temporarily dump test output so we can debug failing tests on
# buildbots.
# RUN: cat %t.out
# RUN: FileCheck --input-file %t.out %s
#
# END.

# CHECK: -- Testing:


# CHECK: FAIL: shtest-shell :: diff-error-0.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-0.txt' FAILED ***
# CHECK: $ "diff" "temp1.txt" "temp2.txt"
# CHECK: # command stderr:
# CHECK: Unsupported: 'diff' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-1.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-1.txt' FAILED ***
# CHECK: $ "diff" "-u" "temp1.txt" "temp2.txt"
# CHECK: # command stderr:
# CHECK: Error: 'diff' only supports a fromfile and tofile
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-2.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-2.txt' FAILED ***
# CHECK: $ "diff" "temp.txt"
# CHECK: # command stderr:
# CHECK: Error: 'diff' only supports a fromfile and tofile
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-3.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-3.txt' FAILED ***
# CHECK: $ "diff" "temp.txt" "temp1.txt"
# CHECK: # command stderr:
# CHECK: Error: 'diff' command failed
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: diff-error-4.txt
# CHECK: *** TEST 'shtest-shell :: diff-error-4.txt' FAILED ***
# CHECK: $ "diff"
# CHECK: # command stderr:
# CHECK: Error: The contexts of fromfile and tofile are different
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: error-0.txt
# CHECK: *** TEST 'shtest-shell :: error-0.txt' FAILED ***
# CHECK: $ "not-a-real-command"
# CHECK: # command stderr:
# CHECK: 'not-a-real-command': command not found
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# FIXME: The output here sucks.
#
# CHECK: FAIL: shtest-shell :: error-1.txt
# CHECK: *** TEST 'shtest-shell :: error-1.txt' FAILED ***
# CHECK: shell parser error on: 'echo "missing quote'
# CHECK: ***

# CHECK: FAIL: shtest-shell :: error-2.txt
# CHECK: *** TEST 'shtest-shell :: error-2.txt' FAILED ***
# CHECK: Unsupported redirect:
# CHECK: ***

# CHECK: FAIL: shtest-shell :: mkdir-error-0.txt
# CHECK: *** TEST 'shtest-shell :: mkdir-error-0.txt' FAILED ***
# CHECK: $ "mkdir" "-p" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'mkdir' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: mkdir-error-1.txt
# CHECK: *** TEST 'shtest-shell :: mkdir-error-1.txt' FAILED ***
# CHECK: $ "mkdir" "-i" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'mkdir' does not support -i option
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: mkdir-error-2.txt
# CHECK: *** TEST 'shtest-shell :: mkdir-error-2.txt' FAILED ***
# CHECK: $ "mkdir" "-p"
# CHECK: # command stderr:
# CHECK: Error: 'mkdir' is missing an operand
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: PASS: shtest-shell :: redirects.txt

# CHECK: FAIL: shtest-shell :: rm-error-0.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-0.txt' FAILED ***
# CHECK: $ "rm" "-rf" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'rm' cannot be part of a pipeline
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: rm-error-1.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-1.txt' FAILED ***
# CHECK: $ "rm" "-f" "-i" "temp"
# CHECK: # command stderr:
# CHECK: Unsupported: 'rm' does not support -i option
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: rm-error-2.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-2.txt' FAILED ***
# CHECK: $ "rm" "-r" "hello"
# CHECK: # command stderr:
# CHECK: Error: 'rm' command failed
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: FAIL: shtest-shell :: rm-error-3.txt
# CHECK: *** TEST 'shtest-shell :: rm-error-3.txt' FAILED ***
# CHECK: $ "rm" "-f" "test"
# CHECK: # command stderr:
# CHECK: Error: test is a directory
# CHECK: error: command failed with exit status: 127
# CHECK: ***

# CHECK: PASS: shtest-shell :: sequencing-0.txt
# CHECK: XFAIL: shtest-shell :: sequencing-1.txt
# CHECK: PASS: shtest-shell :: valid-shell.txt
# CHECK: Failing Tests (15)
