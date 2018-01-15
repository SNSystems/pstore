
if (PSTORE_IS_INSIDE_LLVM)
    # compiling inside LLVM. We don't add the googletest library
    # because LLVM provides a copy for us.
else ()
    # Tell gtest to link against the "Multi-threaded Debug DLL runtime
    # library" on Windows.
    set (gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
    add_subdirectory (googletest)
 endif ()


#eof 3rd_party/CMakeLists.txt
