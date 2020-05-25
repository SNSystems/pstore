#ifndef PSTORE_IMPORT_IMPORT_ERROR_HPP
#define PSTORE_IMPORT_IMPORT_ERROR_HPP

#include <system_error>

enum class import_error : int {
    none,

    unexpected_null,
    unexpected_boolean,
    unexpected_number,
    unexpected_string,
    unexpected_array,
    unexpected_end_array,
    unexpected_object,
    unexpected_object_key,
    unexpected_end_object,

    unrecognized_ifixup_key,
    ifixup_object_was_incomplete,
    unrecognized_xfixup_key,
    xfixup_object_was_incomplete,
    generic_section_was_incomplete,
    unrecognized_section_object_key,
    unrecognized_root_key,
    root_object_was_incomplete,
    unknown_transaction_object_key,
    unknown_compilation_object_key,
    unknown_definition_object_key,

    incomplete_compilation_object,
    incomplete_debug_line_section,

    bad_digest,
    bad_base64_data,
    bad_linkage,
    bad_visibility,
    unknown_section_name,
};


class import_error_category : public std::error_category {
public:
    import_error_category () noexcept;
    char const * name () const noexcept override;
    std::string message (int error) const override;
};

std::error_category const & get_import_error_category () noexcept;
std::error_code make_error_code (import_error const e) noexcept;


namespace std {

    template <>
    struct is_error_code_enum<import_error> : std::true_type {};

} // end namespace std

#endif // PSTORE_IMPORT_IMPORT_ERROR_HPP
