set (PSTORE_BROKERD "${CMAKE_INSTALL_PREFIX}/bin/pstore-brokerd")

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/pstore-brokerd.init"
    "${CMAKE_CURRENT_BINARY_DIR}/pstore-brokerd.init"
    ESCAPE_QUOTES
    @ONLY
)
install (
    FILES "${CMAKE_CURRENT_BINARY_DIR}/pstore-brokerd.init"
    RENAME pstore-brokerd
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION etc/init.d
    COMPONENT pstore
)
