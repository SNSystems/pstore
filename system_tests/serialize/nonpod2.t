# %S = the test source directory
# %binaries = the directories containing the executable binaries
# %t = temporary file name unique to the test

REQUIRES: examples

RUN: "%binaries/example-nonpod2" > "%t"
RUN: diff "%t" "%S/nonpod2_expected.txt"
