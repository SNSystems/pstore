#ifndef json_callbacks_hpp
#define json_callbacks_hpp

#include <stack>
#include <system_error>

#include "pstore/support/gsl.hpp"


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
    unrecognized_section_key,
    unrecognized_root_key,
    root_object_was_incomplete,
    unknown_transaction_object_key,
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



class state;
using parse_stack = std::stack<std::unique_ptr<state>>;

class state {
public:
    explicit state (pstore::gsl::not_null<parse_stack *> s)
            : stack_{s} {}
    virtual ~state ();

    virtual std::error_code int64_value (std::int64_t v);
    virtual std::error_code uint64_value (std::uint64_t v);
    virtual std::error_code double_value (double v);
    virtual std::error_code string_value (std::string const & v);
    virtual std::error_code boolean_value (bool v);
    virtual std::error_code null_value ();
    virtual std::error_code begin_array ();
    virtual std::error_code end_array ();
    virtual std::error_code begin_object ();
    virtual std::error_code key (std::string const & k);
    virtual std::error_code end_object ();

protected:
    pstore::gsl::not_null<parse_stack *> stack () { return stack_; }
    std::error_code suicide () {
        stack ()->pop ();
        return {};
    }

private:
    pstore::gsl::not_null<parse_stack *> stack_;
};


class expect_uint64 final : public state {
public:
    expect_uint64 (pstore::gsl::not_null<parse_stack *> stack,
                   pstore::gsl::not_null<std::uint64_t *> v)
            : state (stack)
            , v_{v} {}
    std::error_code uint64_value (std::uint64_t v) override {
        *v_ = v;
        return this->suicide ();
    }

private:
    pstore::gsl::not_null<std::uint64_t *> v_;
};



class callbacks {
public:
    using result_type = void;
    result_type result () {}

    template <typename Root, typename... Args>
    static callbacks make (Args &&... args) {
        auto stack = std::make_shared<parse_stack> ();
        return {stack, std::make_unique<Root> (stack.get (), std::forward<Args> (args)...)};
    }

    std::error_code int64_value (std::int64_t v) { return stack_->top ()->int64_value (v); }
    std::error_code uint64_value (std::uint64_t v) { return stack_->top ()->uint64_value (v); }
    std::error_code double_value (double v) { return stack_->top ()->double_value (v); }
    std::error_code string_value (std::string const & v) {
        return stack_->top ()->string_value (v);
    }
    std::error_code boolean_value (bool v) { return stack_->top ()->boolean_value (v); }
    std::error_code null_value () { return stack_->top ()->null_value (); }
    std::error_code begin_array () { return stack_->top ()->begin_array (); }
    std::error_code end_array () { return stack_->top ()->end_array (); }
    std::error_code begin_object () { return stack_->top ()->begin_object (); }
    std::error_code key (std::string const & k) { return stack_->top ()->key (k); }
    std::error_code end_object () { return stack_->top ()->end_object (); }

private:
    callbacks (std::shared_ptr<parse_stack> const & stack, std::unique_ptr<state> && root)
            : stack_{stack} {
        stack_->push (std::move (root));
    }

    std::shared_ptr<parse_stack> stack_;
};

#endif /* json_callbacks_hpp */
