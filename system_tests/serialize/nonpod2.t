# %binaries = the directories containing the executable binaries
# %T = the test output directory
# %S = the test source directory
REQUIRES: examples
RUN: %binaries/example-nonpod2 > %T/nonpod2_actual.txt
RUN: diff %T/nonpod2_actual.txt %S/nonpod2_expected.txt
