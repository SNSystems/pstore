# %S = the test source directory
# %binaries = the directories containing the executable binaries
# %t = temporary file name unique to the test

REQUIRES: examples

RUN: "%binaries/example-ostream-writer" > "%t"
RUN: diff "%t" "%S/ostream_writer_expected.txt"
