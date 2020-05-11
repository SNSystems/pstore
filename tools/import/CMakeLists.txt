add_pstore_executable (pstore-import import.cpp)
target_link_libraries (pstore-import PRIVATE
    pstore-cmd-util
    pstore-core
    pstore-json-lib
    pstore-mcrepo
)
#run_pstore_unit_test (pstore-json pstore-json-unit-tests)
