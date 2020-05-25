#include "import_error.hpp"

import_error_category::import_error_category () noexcept = default;

char const * import_error_category::name () const noexcept {
    return "import parser category";
}

std::string import_error_category::message (int const error) const {
    auto * result = "unknown import_error_category error";
    switch (static_cast<import_error> (error)) {
    case import_error::none: result = "none"; break;
    case import_error::unexpected_null: result = "unexpected null"; break;
    case import_error::unexpected_boolean: result = "unexpected boolean"; break;
    case import_error::unexpected_number: result = "unexpected number"; break;
    case import_error::unexpected_string: result = "unexpected string"; break;
    case import_error::unexpected_array: result = "unexpected array"; break;
    case import_error::unexpected_end_array: result = "unexpected end of array"; break;
    case import_error::unexpected_object: result = "unexpected object"; break;
    case import_error::unexpected_object_key: result = "unexpected object key"; break;
    case import_error::unexpected_end_object: result = "unexpected end object"; break;


    case import_error::unrecognized_ifixup_key:
        result = "unrecognized internal fixup object key";
        break;
    case import_error::ifixup_object_was_incomplete:
        result = "internal fixup object was not complete";
        break;
    case import_error::unrecognized_xfixup_key:
        result = "unrecognized external fixup object key";
        break;
    case import_error::xfixup_object_was_incomplete:
        result = "external fixup object was incomplete";
        break;
    case import_error::unrecognized_section_object_key:
        result = "unrecognized section object key";
        break;
    case import_error::generic_section_was_incomplete:
        result = "generic section object was incomplete";
        break;
    case import_error::root_object_was_incomplete: result = "root object was incomplete"; break;
    case import_error::unrecognized_root_key: result = "unrecognized root object key"; break;
    case import_error::unknown_transaction_object_key:
        result = "unrecognized transaction object key";
        break;
    case import_error::unknown_compilation_object_key:
        result = "unknown compilation object key";
        break;
    case import_error::unknown_definition_object_key:
        result = "unknown definition object key";
        break;

    case import_error::incomplete_debug_line_section:
        result = "debug line section object was incomplete";
        break;
    case import_error::incomplete_compilation_object:
        result = "compilation object was incomplete";
        break;

    case import_error::bad_linkage: result = "unknown linkage type"; break;
    case import_error::bad_base64_data: result = "bad Base64 data"; break;
    case import_error::bad_digest: result = "bad digest"; break;
    case import_error::bad_visibility: result = "unknown visibility"; break;
    case import_error::unknown_section_name: result = "unknown section name"; break;
    }
    return result;
}

std::error_category const & get_import_error_category () noexcept {
    static import_error_category const cat;
    return cat;
}

std::error_code make_error_code (import_error const e) noexcept {
    static_assert (std::is_same<std::underlying_type<import_error>::type, int>::value,
                   "The underlying type of import error must be int");
    return {static_cast<int> (e), get_import_error_category ()};
}
