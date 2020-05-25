//
//  import_rule.cpp
//  gmock
//
//  Created by Paul Bowen-Huggett on 25/05/2020.
//

#include "import_rule.hpp"
#include "import_error.hpp"

state::~state () = default;

std::error_code state::int64_value (std::int64_t) {
    return make_error_code (import_error::unexpected_number);
}
std::error_code state::uint64_value (std::uint64_t) {
    return make_error_code (import_error::unexpected_number);
}
std::error_code state::double_value (double) {
    return make_error_code (import_error::unexpected_number);
}
std::error_code state::boolean_value (bool) {
    return make_error_code (import_error::unexpected_boolean);
}
std::error_code state::null_value () {
    return make_error_code (import_error::unexpected_null);
}
std::error_code state::begin_array () {
    return make_error_code (import_error::unexpected_array);
}
std::error_code state::string_value (std::string const &) {
    return make_error_code (import_error::unexpected_string);
}
std::error_code state::end_array () {
    return make_error_code (import_error::unexpected_end_array);
}
std::error_code state::begin_object () {
    return make_error_code (import_error::unexpected_object);
}
std::error_code state::key (std::string const &) {
    return make_error_code (import_error::unexpected_object_key);
}
std::error_code state::end_object () {
    return make_error_code (import_error::unexpected_end_object);
}
