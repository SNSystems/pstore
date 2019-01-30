set (PSTORE_RUN_KLEE  "${PSTORE_ROOT_DIR}/unittests/support/klee/run_klee.py")

# The suport code assumes that we're using the KLEE Docker container which has
# the sources and libraries in the locations given by the following two variables:
set (PSTORE_KLEE_SRC_DIR "/home/klee/klee_src/include")
set (PSTORE_KLEE_LIB_DIR "/home/klee/klee_build/klee/lib/libkleeRuntest.so")


# pstore_can_klee
# ~~~~~~~~~~~~~~~
# Sets the return variable to a true value if the klee tests can be run. The
# host compiler must be clang, the klee executable must be on the path, and we
# must be using cmake version >= 3.9.

function (pstore_can_klee result)

    set (${result} No PARENT_SCOPE)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")

        # The klee executable must be on the path somewhere.
        find_program (PSTORE_KLEE_PATH klee)
	if (NOT "${PSTORE_KLEE_PATH}" STREQUAL "PSTORE_KLEE_PATH-NOTFOUND")

	    # The klee custom cmake targets requires that we running cmake 3.9
	    # or later.
            if (NOT (${CMAKE_MAJOR_VERSION} EQUAL 3 AND ${CMAKE_MINOR_VERSION} LESS 9))
	       set (${result} Yes PARENT_SCOPE)
            endif ()

	endif ()
    endif ()

endfunction (pstore_can_klee)


# pstore_add_klee_run_all_target
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function (pstore_add_klee_run_all_target bitcode_enabled)

    set (${bitcode_enabled} No PARENT_SCOPE)

    pstore_can_klee (can_klee)
    if (can_klee)
        message (STATUS "KLEE targets are enabled")
        set (${bitcode_enabled} Yes PARENT_SCOPE)
	add_custom_target (pstore-klee-run-all)
    else ()
        message (STATUS "KLEE targets are disabled")
    endif (can_klee)

endfunction (pstore_add_klee_run_all_target)


# pstore_configure_klee_test_target
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Configures a KLEE test target with its includes, compiler switches, and so
# on. This function only performs configurations that are common to both the
# bitcode and executable targets.

function (pstore_configure_klee_test_target name)
    target_include_directories (${name} PRIVATE "${PSTORE_KLEE_SRC_DIR}")
    # TODO: enable these switches (and more santizers).
    #target_compile_options (${name} PRIVATE
    #    -fsanitize=signed-integer-overflow
    #    -fsanitize=unsigned-integer-overflow
    #)
    #set_target_properties (${name} PROPERTIES LINK_FLAGS
    #    "-fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow"
    #)
    set_target_properties (${name} PROPERTIES
        FOLDER "pstore-klee"
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED Yes
    )

endfunction (pstore_configure_klee_test_target)


# pstore_add_klee_test
# ~~~~~~~~~~~~~~~~~~~~
function (pstore_add_klee_test category name)

    pstore_can_klee (can_klee)
    if (can_klee)
        set (tname_base "pstore-klee-${category}-${name}")

        set (bc_tname "${tname_base}-bc")
        set (exe_tname "${tname_base}-exe")

        # The bitcode library.

        add_library ("${bc_tname}" OBJECT ${name}.cpp)
        pstore_configure_klee_test_target ("${bc_tname}")
        target_compile_options ("${bc_tname}" PRIVATE -emit-llvm)

        get_target_property (pstore_support_includes pstore-support INTERFACE_INCLUDE_DIRECTORIES)
        target_include_directories ("${bc_tname}" PUBLIC "${pstore_support_includes}")

        # The executable.

        add_executable (${exe_tname} ${name}.cpp)
        pstore_configure_klee_test_target (${exe_tname})
        target_compile_definitions (${exe_tname} PRIVATE -DPSTORE_KLEE_RUN)
        target_link_libraries (${exe_tname} PRIVATE
	    pstore-support
	    "${PSTORE_KLEE_LIB_DIR}"
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
    endif (can_klee)

endfunction (pstore_add_klee_test)
