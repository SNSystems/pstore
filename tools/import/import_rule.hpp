//
//  import_rule.hpp
//  gmock
//
//  Created by Paul Bowen-Huggett on 25/05/2020.
//

#ifndef import_rule_hpp
#define import_rule_hpp

#include <stack>
#include <system_error>
#include <iostream> // TODO: remove this!
#include "pstore/support/gsl.hpp"

class state {
public:
    using parse_stack = std::stack<std::unique_ptr<state>>;
    using parse_stack_pointer = pstore::gsl::not_null<parse_stack *>;

    explicit state (parse_stack_pointer stack)
            : stack_{stack} {}
    state (state const &) = delete;
    virtual ~state ();

    state & operator= (state const &) = delete;

    virtual pstore::gsl::czstring name () const noexcept = 0;

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

    // protected:
    template <typename T, typename... Args>
    std::error_code push (Args &&... args) {
        stack_->push (std::make_unique<T> (stack_, std::forward<Args> (args)...));
        std::cout << std::string (stack_->size () * trace_indent, ' ') << '+'
                  << stack_->top ()->name () << '\n';
        return {};
    }

    template <typename T, typename... Args>
    std::error_code replace_top (Args &&... args) {
        auto p = std::make_unique<T> (stack_, std::forward<Args> (args)...);
        auto stack = stack_;
        std::cout << indent (*stack_) << '-' << stack_->top ()->name () << '\n';

        stack->pop (); // Destroys this object.
        stack->push (std::move (p));
        std::cout << indent (*stack) << '+' << stack->top ()->name () << '\n';
        return {};
    }

    std::error_code pop () {
        std::cout << indent (*stack_) << '-' << stack_->top ()->name () << '\n';
        stack_->pop ();
        return {};
    }

private:
    static constexpr auto trace_indent = std::size_t{2};
    static std::string indent (parse_stack const & stack) {
        return std::string (stack.size () * trace_indent, ' ');
    }
    parse_stack_pointer stack_;
};


class callbacks {
public:
    using result_type = void;
    result_type result () {}

    template <typename Root, typename... Args>
    static callbacks make (Args &&... args) {
        auto stack = std::make_shared<state::parse_stack> ();
        return {stack, std::make_unique<Root> (stack.get (), std::forward<Args> (args)...)};
    }

    std::error_code int64_value (std::int64_t v) { return top ()->int64_value (v); }
    std::error_code uint64_value (std::uint64_t v) { return top ()->uint64_value (v); }
    std::error_code double_value (double v) { return top ()->double_value (v); }
    std::error_code string_value (std::string const & v) { return top ()->string_value (v); }
    std::error_code boolean_value (bool v) { return top ()->boolean_value (v); }
    std::error_code null_value () { return top ()->null_value (); }
    std::error_code begin_array () { return top ()->begin_array (); }
    std::error_code end_array () { return top ()->end_array (); }
    std::error_code begin_object () { return top ()->begin_object (); }
    std::error_code key (std::string const & k) { return top ()->key (k); }
    std::error_code end_object () { return top ()->end_object (); }

private:
    callbacks (std::shared_ptr<state::parse_stack> const & stack, std::unique_ptr<state> && root)
            : stack_{stack} {
        stack_->push (std::move (root));
    }

    std::unique_ptr<state> & top () {
        assert (!stack_->empty ());
        return stack_->top ();
    }
    std::shared_ptr<state::parse_stack> stack_;
};


#endif /* import_rule_hpp */
