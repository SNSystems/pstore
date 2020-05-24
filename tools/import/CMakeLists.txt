add_pstore_executable (pstore-import
    array_helpers.hpp
    digest_from_string.cpp
    digest_from_string.hpp
    import.cpp
    import_fragment.cpp
    import_fragment.hpp
    json_callbacks.cpp
    json_callbacks.hpp
)
target_link_libraries (pstore-import PRIVATE
    pstore-cmd-util
    pstore-core
    pstore-json-lib
    pstore-mcrepo
)
#run_pstore_unit_test (pstore-json pstore-json-unit-tests)
