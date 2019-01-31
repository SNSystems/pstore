include (add_pstore)

set (PSTORE_HTTPD_PUBLIC_INCLUDE "${PSTORE_ROOT_DIR}/include/pstore/httpd")
add_pstore_library (pstore-httpd-lib
    "${PSTORE_HTTPD_PUBLIC_INCLUDE}/media_type.hpp"
    "${PSTORE_HTTPD_PUBLIC_INCLUDE}/query_to_kvp.hpp"
    "${PSTORE_HTTPD_PUBLIC_INCLUDE}/server.hpp"

    media_type.cpp
    query_to_kvp.cpp
    server.cpp
)
target_link_libraries (pstore-httpd-lib PUBLIC pstore-broker-intf pstore-romfs pstore-support)
add_clang_tidy_target (pstore-httpd-lib)
