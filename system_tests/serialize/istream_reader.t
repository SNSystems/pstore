# %S = the test source directory
# %binaries = the directories containing the executable binaries
# %t = temporary file name unique to the test

REQUIRES: examples
RUN: "%binaries/example-istream-reader" > "%t"
RUN: diff "%t" "%S/istream_reader_expected.txt"
