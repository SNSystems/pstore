# %S = the test source directory
# %binaries = the directories containing the executable binaries
# %t = temporary file name unique to the test

REQUIRES: examples
RUN: "%binaries/example-write-integers" > "%t"
RUN: diff -b "%t" "%S/ex1_expected.txt"
