#===- cmake/clang_tidy.cmake ----------------------------------------------===//
#*       _                     _   _     _        *
#*   ___| | __ _ _ __   __ _  | |_(_) __| |_   _  *
#*  / __| |/ _` | '_ \ / _` | | __| |/ _` | | | | *
#* | (__| | (_| | | | | (_| | | |_| | (_| | |_| | *
#*  \___|_|\__,_|_| |_|\__, |  \__|_|\__,_|\__, | *
#*                     |___/               |___/  *
#===----------------------------------------------------------------------===//
#
# Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//

# pstore_can_tidy
# ~~~~~~~~~~~~~~~
# Sets the return variable to a true value if the host supports running clang-tidy.
# At the time of writing, we don't support Windows because the compiler's input
# in pstore_find_clang_tidy() comes from /dev/null. That function only knows how
# to extract the paths from GCC/Clang so only these compilers are supported.

function (pstore_can_tidy result)
    set (${result} No PARENT_SCOPE)

    if (PSTORE_CLANG_TIDY_ENABLED)
        if (NOT WIN32)
            # (Note that I allow for "Clang" and "AppleClang" and any other clang
            # variants out there.)

            if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang$"))
                set (${result} Yes PARENT_SCOPE)
            endif ()
        endif (NOT WIN32)
    endif (PSTORE_CLANG_TIDY_ENABLED)

endfunction (pstore_can_tidy)


# pstore_add_tidy_all_target
# ~~~~~~~~~~~~~~~~~~~~~~~~~~
function (pstore_add_tidy_all_target)

    pstore_can_tidy (can_tidy)
    if (can_tidy)
         find_program (PSTORE_CLANG_TIDY "clang-tidy")
         if (NOT PSTORE_CLANG_TIDY STREQUAL "PSTORE_CLANG_TIDY-NOTFOUND")
             add_custom_target (pstore-clang-tidy-all)
        endif ()
    endif ()

endfunction (pstore_add_tidy_all_target)



# pstore_find_clang_tidy
# ~~~~~~~~~~~~~~~~~~~~~~
# Locates the clang-tidy executable (if we're using a supported host/compiler
# combination) and extras the built-in ("system") include paths from it so that
# they can be passed to clang-tidy.
#
# Usage: pstore_find_clang_tidy <var1> <var2>
#
# On return sets the result variables as follows:
# - tidy_path: Set to the location of the clang-tidy executable if found and the
#   host platform is supported. If not found set to "<var1>-NOTFOUND" (in the
#   same fashion as the find_program() function).
#
# - sys_includes: Set to the list of the host compiler's default include paths.
#   If the host platform is not supported, set to "<var2>-NOTFOUND".
#
# As a side-effect the function may create two cache variables:
# - PSTORE_CLANG_TIDY
# - PSTORE_SYS_INCLUDES

function (pstore_find_clang_tidy tidy_path sys_includes)

    set (${tidy_path} "${tidy_path}-NOTFOUND" PARENT_SCOPE)
    set (${sys_includes} "${sys_includes}-NOTFOUND" PARENT_SCOPE)

    pstore_can_tidy (can_tidy)
    if (can_tidy)
        find_program (PSTORE_CLANG_TIDY "clang-tidy")
        if (NOT PSTORE_CLANG_TIDY STREQUAL "PSTORE_CLANG_TIDY-NOTFOUND")
            set (${tidy_path} "${PSTORE_CLANG_TIDY}" PARENT_SCOPE)
        endif ()

        # If the PSTORE_SYS_INCLUDES cache variable has not been set by an
        # earlier call to this function...

        if (NOT PSTORE_SYS_INCLUDES)
            if (PSTORE_CLANG_TIDY STREQUAL "PSTORE_CLANG_TIDY-NOTFOUND")
                message (STATUS "clang-tidy was not found: targets omitted")
                set (PSTORE_SYS_INCLUDES "${tidy_path}")
            else ()
                message (STATUS "clang-tidy path is: ${PSTORE_CLANG_TIDY}")

                # Extract the default system include paths from the compiler. We
                # run it with '-v' and those are written to stderr along with
                # other information data doesn't interest us.
                execute_process (
                    COMMAND g++ -E -x c++ - -v
                    INPUT_FILE "/dev/null"
                    ERROR_VARIABLE include_paths
                    OUTPUT_QUIET
                )

                # Remove any text that doesn't relate to the include paths. That's
                # everything after "#include <...> search starts here:" and before
                # "End of search list.". I also remove any macOS framework directories.
                execute_process (
                    COMMAND echo "${include_paths}"
                    COMMAND sed -e "1,/> search starts here:/d"
                                -e "s/ \\(.*\\)/\\1/"
                                -e "/of search list/,$d"
                                -e "/\\(framework directory\\)/d"
                    OUTPUT_VARIABLE include_paths
                )

                # Now turn this string into a Cmake list.
                string (REPLACE "\n" ";" include_paths ${include_paths})

                # Finally we get to build the system include directory
                # switches. Each path has leading and trailing whitespace stripped
                # and an -isystem switch preceeding it.
                foreach (P ${include_paths})
                    string (STRIP "${P}" P)
                    string (LENGTH "${P}" PLENGTH)
                    if (PLENGTH GREATER 0)
                        list (APPEND sys_includes "-isystem" "${P}")
                    endif ()
                endforeach (P)
                set (PSTORE_SYS_INCLUDES ${sys_includes} CACHE INTERNAL "The compiler's system include paths")
            endif ()
        endif (NOT PSTORE_SYS_INCLUDES)

        set (${sys_includes} "${PSTORE_SYS_INCLUDES}" PARENT_SCOPE)
    endif (can_tidy)

endfunction (pstore_find_clang_tidy)



function (add_clang_tidy_target source_target)

    pstore_find_clang_tidy (tidy_path sys_includes)

    if (NOT tidy_path STREQUAL "tidy_path-NOTFOUND")
        # Collect the *.cpp files: clang-tidy will scan these.
        get_target_property (source_files "${source_target}" SOURCES)

        # In cmake 3.6 or later we could use
        #   list (FILTER source_files INCLUDE REGEX "cpp")
        # However, this needs to work in 3.4...

        get_target_property (source_target_dir "${source_target}" SOURCE_DIR)

        set (cpp_files "")
        foreach (src_file ${source_files})
            get_filename_component (src_file_name "${src_file}" NAME)
            get_filename_component (src_file_directory "${src_file}" DIRECTORY)
            if (NOT src_file_directory)
               get_target_property (src_file_directory "${source_target}" SOURCE_DIR)
            endif ()

            # Only target source files ending in .cpp and in the target's
            # source directory.

            if (    ("${src_file_name}" MATCHES "\.cpp$")
                AND ("${src_file_directory}" STREQUAL "${source_target_dir}")
            )
                list (APPEND cpp_files "${src_file}")
            endif ()
        endforeach (src_file)

        set (tidy_target "${source_target}-tidy")

        add_custom_target ("${tidy_target}"
            COMMAND "${tidy_path}"
                    -header-filter="${PROJECT_SOURCE_DIR}/include/.*"
                    ${cpp_files}
                    --
                    -std=c++14
                    -D NDEBUG
                    "-I$<JOIN:$<TARGET_PROPERTY:${source_target},INCLUDE_DIRECTORIES>,;-I>"
                    ${sys_includes}
            WORKING_DIRECTORY "${source_target_dir}"
            COMMENT "Running clang-tidy for target ${source_target}"
            COMMAND_EXPAND_LISTS
        )
        set_target_properties ("${tidy_target}" PROPERTIES FOLDER "pstore clang-tidy")
        add_dependencies (pstore-clang-tidy-all "${tidy_target}")
    endif()

endfunction ()
