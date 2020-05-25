#include "debug_line_header.hpp"

debug_line_index::debug_line_index (parse_stack_pointer s, transaction_pointer transaction)
        : state (s)
        , transaction_{transaction}
        , index_{pstore::index::get_index<pstore::trailer::indices::debug_line_header> (
              transaction->db ())} {}

gsl::czstring debug_line_index::name () const noexcept {
    return "debug_line_index";
}

std::error_code debug_line_index::string_value (std::string const & s) {
    // Decode the received string to get the raw binary.
    std::vector<std::uint8_t> data;
    maybe<std::back_insert_iterator<decltype (data)>> oit =
        from_base64 (std::begin (s), std::end (s), std::back_inserter (data));
    if (!oit) {
        return import_error::bad_base64_data;
    }

    // Create space for this data in the store.
    std::shared_ptr<std::uint8_t> out;
    pstore::typed_address<std::uint8_t> where;
    std::tie (out, where) = transaction_->alloc_rw<std::uint8_t> (data.size ());

    // Copy the data to the store.
    std::copy (std::begin (data), std::end (data), out.get ());

    // Add an index entry for this data.
    index_->insert (*transaction_,
                    std::make_pair (digest_, pstore::extent<std::uint8_t>{where, data.size ()}));
    return {};
}

std::error_code debug_line_index::key (std::string const & s) {
    if (maybe<uint128> const digest = digest_from_string (s)) {
        digest_ = *digest;
        return {};
    }
    return import_error::bad_digest;
}
std::error_code debug_line_index::end_object () {
    return pop ();
}
