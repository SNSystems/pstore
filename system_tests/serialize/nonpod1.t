# %binaries = the directories containing the executable binaries
# %T = the test output directory
# %S = the test source directory
REQUIRES: examples
RUN: %binaries/example_nonpod1 > %T/nonpod1_actual.txt
RUN: diff %T/nonpod1_actual.txt %S/nonpod1_expected.txt
