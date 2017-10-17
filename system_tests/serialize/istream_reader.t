# %binaries = the directories containing the executable binaries
# %T = the test output directory
# %S = the test source directory
REQUIRES: examples
RUN: %binaries/example-istream-reader > %T/istream_reader_actual.txt
RUN: diff %T/istream_reader_actual.txt %S/istream_reader_expected.txt
