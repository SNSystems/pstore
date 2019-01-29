set (PSTORE_RUN_KLEE  "${CMAKE_CURRENT_SOURCE_DIR}/run_klee.py")

add_custom_target (pstore-klee-run-all)

function (configure_klee_test_target name)
    target_include_directories (${name} PRIVATE /home/klee/klee_src/include)
    #target_compile_options (${name} PRIVATE
    #    -fsanitize=signed-integer-overflow
    #    -fsanitize=unsigned-integer-overflow
    #)
    #set_target_properties (${name} PROPERTIES LINK_FLAGS
    #    "-fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow"
    #)
    set_property (TARGET ${name} PROPERTY CXX_STANDARD 11)
    set_property (TARGET ${name} PROPERTY CXX_STANDARD_REQUIRED Yes)
endfunction ()


function (add_klee_test category name)
    set (tname_base "pstore-klee-${category}-${name}")

    set (bc_tname "${tname_base}-bc")
    set (exe_tname "${tname_base}-exe")


    # The bitcode library.

    add_library ("${bc_tname}" OBJECT ${name}.cpp)
    configure_klee_test_target ("${bc_tname}")
    target_compile_options ("${bc_tname}" PRIVATE -emit-llvm)
    
    get_target_property (pstore_support_includes pstore-support INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories ("${bc_tname}" PUBLIC "${pstore_support_includes}")

    # The executable.

    add_executable (${exe_tname} ${name}.cpp)
    set_target_properties ("${exe_tname}" PROPERTIES FOLDER "pstore-klee")
    configure_klee_test_target (${exe_tname})
    target_compile_definitions (${exe_tname} PRIVATE -DPSTORE_KLEE_RUN)
    target_link_libraries (${exe_tname} PRIVATE
        pstore-support
        /home/klee/klee_build/klee/lib/libkleeRuntest.so
    )

    add_custom_target (
        "${tname_base}-run"
        COMMAND klee
                --libc=uclibc
                --posix-runtime
                --only-output-states-covering-new
                --optimize
                "-link-llvm-lib=$<TARGET_FILE:pstore-support-bc>"
                "$<TARGET_OBJECTS:${bc_tname}>"

        COMMAND "${PSTORE_RUN_KLEE}"
                # --ktest-tool
                "$<TARGET_FILE:${exe_tname}>"
                "$<TARGET_OBJECTS:${bc_tname}>"

        DEPENDS "${bc_tname}"
                ${exe_tname}
                pstore-support-bc
        COMMENT "Running KLEE for '${bc_tname}'"
    )
    set_target_properties ("${tname_base}-run" PROPERTIES FOLDER "pstore-klee")

    add_dependencies (pstore-klee-run-all ${tname_base}-run)
endfunction (add_klee_test)



add_subdirectory (path)
add_subdirectory (uint128)
add_subdirectory (utf)
