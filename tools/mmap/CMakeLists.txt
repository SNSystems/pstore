include (add_pstore)
add_pstore_tool (pstore-mmap mmap.cpp)
target_link_libraries (pstore-mmap PRIVATE pstore-os pstore-cmd-util)
