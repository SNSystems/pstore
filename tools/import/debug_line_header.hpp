#ifndef PSTORE_IMPORT_DEBUG_LINE_HEADER_HPP
#define PSTORE_IMPORT_DEBUG_LINE_HEADER_HPP

#include <system_error>

class debug_line_index final : public state {
public:
    debug_line_index (parse_stack_pointer s, transaction_pointer transaction);
    gsl::czstring name () const noexcept override;

    std::error_code string_value (std::string const & s);
    std::error_code key (std::string const & s) override;
    std::error_code end_object () override;

private:
    transaction_pointer transaction_;
    std::shared_ptr<pstore::index::debug_line_header_index> index_;
    index::digest digest_;
};

#endif // PSTORE_IMPORT_DEBUG_LINE_HEADER_HPP
