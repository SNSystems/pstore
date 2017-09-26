# %binaries = the directories containing the executable binaries
# %T = the test output directory
# %S = the test source directory
REQUIRES: examples
RUN: %binaries/example_write_integers > %T/ex1_actual.txt
RUN: diff %T/ex1_actual.txt %S/ex1_expected.txt
