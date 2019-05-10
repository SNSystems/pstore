include (klee)

pstore_can_klee (can_klee)
if (can_klee)
    set (target pstore-klee-cxxstdlib)
    add_library (${target}-bc OBJECT cxxstdlib.cpp)
    target_compile_options (${target}-bc PRIVATE -emit-llvm)
    pstore_configure_klee_test_target (${target}-bc)
endif (can_klee)
