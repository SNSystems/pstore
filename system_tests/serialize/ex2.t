# %binaries = the directories containing the executable binaries
# %T = the test output directory
# %S = the test source directory
REQUIRES: examples
RUN: %binaries/example-write-pod-struct > %T/ex2_actual.txt
RUN: diff %T/ex2_actual.txt %S/ex2_expected.txt
