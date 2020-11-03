# %binaries = the directories containing the executable binaries
# %S = the test source directory
# %t = temporary file name unique to the test

REQUIRES: examples

# Delete any existing results.
RUN: rm -rf "%t" && mkdir -p "%t"

RUN: %binaries/example-write-pod-struct > "%t/ex2_actual.txt"
RUN: diff -b "%t/ex2_actual.txt" "%S/ex2_expected.txt"
