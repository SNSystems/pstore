if (PSTORE_VALGRIND)
    find_program (VALGRIND_PATH valgrind)
    if (VALGRIND_PATH STREQUAL "VALGRIND_PATH-NOTFOUND")
        set (PSTORE_VALGRIND No)
        message (WARNING "Valgrind was requested but could not be found")
    else ()
        message (STATUS "Running unit tests with Valgrind memcheck")
    endif ()
endif ()


function (run_pstore_unit_test prelink_target test_target)
    set (OUT_XML "${CMAKE_BINARY_DIR}/${test_target}.xml")

    if (PSTORE_VALGRIND)
        set (OUT_LOG "${CMAKE_BINARY_DIR}/${test_target}-memcheck.log")
        add_custom_command (TARGET ${prelink_target}
            PRE_LINK
            COMMAND "${VALGRIND_PATH}"
                    --tool=memcheck --leak-check=full --show-reachable=yes
                    --undef-value-errors=yes --track-origins=no
                    --child-silent-after-fork=no --trace-children=no
                    --log-file=${OUT_LOG}
                    "$<TARGET_FILE:${test_target}>" "--gtest_output=xml:${OUT_XML}"
            COMMAND cat ${OUT_LOG}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Valgrind Running ${test_target}"
            DEPENDS ${test_target}
            BYPRODUCTS ${OUT_LOG} ${OUT_XML}
            VERBATIM
        )
    else ()
        add_custom_command (TARGET ${prelink_target}
            PRE_LINK
            COMMAND ${test_target} "--gtest_output=xml:${OUT_XML}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Running ${test_target}"
            DEPENDS ${test_target}
            BYPRODUCTS ${OUT_XML}
            VERBATIM
        )
    endif ()
endfunction (run_pstore_unit_test)

