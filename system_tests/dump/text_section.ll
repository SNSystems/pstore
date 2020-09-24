; Test for dumping of a fragment's text section. When pstore is built inside LLVM,
; we use the latter's disassembler library to produce nice disassembled output
; rather than ASCII-encoded binary. To pass we need simply to dump a section of
; type text.

; %S = the test source directory
; %binaries = the directories containing the executable binaries
; %s = source path (path to the file currently being run)
; %t = temporary file name unique to the test

; REQUIRES: is_inside_llvm
; RUN: rm -f "%t.db"
; RUN: env REPOFILE="%t.db" "%binaries/opt" -O0 -S %s -o %t
; RUN: env REPOFILE="%t.db" "%binaries/llc" -filetype=obj %t
; RUN: "%binaries/pstore-dump" --all-fragments "%t.db" | "%binaries/FileCheck" %s

; CHECK: type : text

target triple = "x86_64-pc-linux-gnu-repo"

define i32 @f(){
  ret i32 1
}
