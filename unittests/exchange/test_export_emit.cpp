#include "pstore/exchange/export_emit.hpp"

#include <sstream>

#include "gtest/gtest.h"

TEST (ExportEmit, SimpleString) {
    {
        std::ostringstream os1;
        pstore::exchange::emit_string (os1, "");
        EXPECT_EQ (os1.str (), "\"\"");
    }
    {
        std::ostringstream os2;
        pstore::exchange::emit_string (os2, "hello");
        EXPECT_EQ (os2.str (), "\"hello\"");
    }
}

TEST (ExportEmit, EscapeQuotes) {
    std::ostringstream os;
    pstore::exchange::emit_string (os, "a \" b");
    EXPECT_EQ (os.str (), "\"a \\\" b\"");
}

TEST (ExportEmit, EscapeBackslash) {
    std::ostringstream os;
    pstore::exchange::emit_string (os, "\\");
    EXPECT_EQ (os.str (), "\"\\\\\"");
}
