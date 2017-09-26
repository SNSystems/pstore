add_library (optionparser
    STATIC
    optionparser.cpp
    optionparser.h
)
   
set_property (TARGET optionparser PROPERTY CXX_STANDARD 11)
set_property (TARGET optionparser PROPERTY CXX_STANDARD_REQUIRED Yes)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    target_compile_definitions (optionparser PUBLIC -DUNICODE -D_UNICODE)
endif ()

target_include_directories (optionparser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
#eof optionparser/CMakeLists.txt
