# %binaries = the directories containing the executable binaries
# %T = the test output directory
# %S = the test source directory
REQUIRES: examples
RUN: %binaries/example-ostream-writer > %T/ostream_writer_actual.txt
RUN: diff %T/ostream_writer_actual.txt %S/ostream_writer_expected.txt
