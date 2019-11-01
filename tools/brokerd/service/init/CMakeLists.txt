set (PSTORE_BROKERD "${CMAKE_INSTALL_PREFIX}/bin/pstore-brokerd")

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/pstore-brokerd.init"
    "${CMAKE_CURRENT_BINARY_DIR}/pstore-brokerd.init"
    ESCAPE_QUOTES
    @ONLY
)
install (
    PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/pstore-brokerd.init"
    RENAME pstore-brokerd
    DESTINATION /etc/init.d/
    COMPONENT pstore
)
