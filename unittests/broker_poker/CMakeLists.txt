add_pstore_unit_test (pstore-broker-poker-unit-tests
    test_iota_generator.cpp
)

# Access the tool private include directory.
target_include_directories (
    pstore-broker-poker-unit-tests
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/broker_poker"
)
