include (add_pstore)
add_pstore_unit_test (pstore-exchange-unit-tests
    test_export_emit.cpp
)
target_link_libraries (pstore-exchange-unit-tests PRIVATE pstore-exchange)
