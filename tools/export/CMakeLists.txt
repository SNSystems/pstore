add_pstore_executable (pstore-export export.cpp)
target_link_libraries (pstore-export PRIVATE pstore-cmd-util pstore-core pstore-mcrepo)
#run_pstore_unit_test (pstore-json pstore-json-unit-tests)
