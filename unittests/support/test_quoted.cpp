#include "pstore/support/quoted.hpp"

#include <gtest/gtest.h>

TEST (Quoted, Empty) {
    std::stringstream str;
    str << pstore::quoted ("");
    EXPECT_EQ (str.str (), R"("")");
}

TEST (Quoted, Simple) {
    std::stringstream str;
    str << pstore::quoted ("simple");
    EXPECT_EQ (str.str (), R"("simple")");
}

TEST (Quoted, SimpleStdString) {
    std::stringstream str;
    str << pstore::quoted (std::string{"simple"});
    EXPECT_EQ (str.str (), R"("simple")");
}

TEST (Quoted, Backslash) {
    std::stringstream str;
    str << pstore::quoted (R"(one\two)");
    EXPECT_EQ (str.str (), R"("one\two")") << "Backslash characters should not be escaped";
}
