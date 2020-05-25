add_pstore_executable (pstore-import
    array_helpers.hpp
    digest_from_string.cpp
    digest_from_string.hpp
    import.cpp
    import_compilations.cpp
    import_compilations.hpp
    import_error.cpp
    import_error.hpp
    import_fragment.cpp
    import_fragment.hpp
    import_names.cpp
    import_names.hpp
    import_non_terminals.cpp
    import_non_terminals.hpp
    import_rule.cpp
    import_rule.hpp
    import_terminals.cpp
    import_terminals.hpp
)
target_link_libraries (pstore-import PRIVATE
    pstore-cmd-util
    pstore-core
    pstore-json-lib
    pstore-mcrepo
)
#run_pstore_unit_test (pstore-json pstore-json-unit-tests)
