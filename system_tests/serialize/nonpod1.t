# %S = the test source directory
# %binaries = the directories containing the executable binaries
# %t = temporary file name unique to the test

REQUIRES: examples
RUN: "%binaries/example-nonpod1" > "%t"
RUN: diff "%t" "%S/nonpod1_expected.txt"
