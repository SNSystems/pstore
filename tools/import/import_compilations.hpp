#ifndef import_compilations_hpp
#define import_compilations_hpp

#include "import_rule.hpp"
#include "import_names.hpp"


class compilations_index final : public state {
public:
    compilations_index (parse_stack_pointer s, transaction_pointer transaction, names_pointer names)
            : state (s)
            , transaction_{transaction}
            , names_{names} {}
    pstore::gsl::czstring name () const noexcept override;
    std::error_code key (std::string const & s) override;
    std::error_code end_object () override;

private:
    transaction_pointer transaction_;
    names_pointer names_;
    //    std::vector<std::unique_ptr<pstore::repo::section_base>> sections_;
};

#endif /* import_compilations_hpp */
