#include <bitset>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <stack>

#include "pstore/json/json.hpp"
#include "pstore/os/file.hpp"
#include "pstore/os/memory_mapper.hpp"
#include "pstore/core/database.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/base64.hpp"

using namespace std::literals::string_literals;

using namespace pstore;

class callbacks_base;
using parse_stack = std::stack<std::unique_ptr<callbacks_base>>;

class callbacks_base {
public:
    explicit callbacks_base (gsl::not_null<parse_stack *> s) : stack_{s} {}
    virtual ~callbacks_base () = default;
    virtual bool ok () const { return ok_; }

    virtual void int64_value (std::int64_t v) { ok_ = false; }
    virtual void uint64_value (std::uint64_t v) { ok_ = false; }
    virtual void double_value (double v) { ok_ = false; }
    virtual void string_value (std::string const & v) { ok_ = false; }
    virtual void boolean_value (bool v) { ok_ = false; }
    virtual void null_value () { ok_ = false; }
    virtual void begin_array () { ok_ = false; }
    virtual void end_array () { ok_ = false; }
    virtual void begin_object () { ok_ = false; }
    virtual void key (std::string const & k) { ok_ = false; }
    virtual void end_object () { ok_ = false; }

protected:
    gsl::not_null<parse_stack *> stack () { return stack_; }
private:
    gsl::not_null<parse_stack *> stack_;
    bool ok_ = true;
};

struct callbacks {
    using result_type = void;
    result_type result () {}

    template <typename Root, typename... Args>
    static callbacks make (Args && ...args) {
        auto stack = std::make_shared<parse_stack> ();
        return callbacks {stack, std::make_unique<Root> (stack.get (), std::forward<Args>(args)...)};
    }

    void int64_value (std::int64_t v) {
        stack_->top ()->int64_value (v);
    }
    void uint64_value (std::uint64_t v) {
        stack_->top ()->uint64_value (v);
    }
    void double_value (double v) {
        stack_->top ()->double_value (v);
    }
    void string_value (std::string const & v) {
        stack_->top ()->string_value (v);
    }
    void boolean_value (bool v) {
        stack_->top ()->boolean_value (v);
    }
    void null_value () {
        stack_->top ()->null_value ();
    }
    void begin_array () {
        stack_->top ()->begin_array ();
    }
    void end_array () {
        stack_->top ()->end_array ();
    }
    void begin_object () {
        stack_->top ()->begin_object ();
    }
    void key (std::string const & k) {
        stack_->top ()->key (k);
    }
    void end_object () {
        stack_->top ()->end_object ();
    }

private:
    callbacks (std::shared_ptr<parse_stack> const & stack, std::unique_ptr<callbacks_base> && root)
            : stack_{stack} {
        stack_->push (std::move (root));
    }

    std::shared_ptr <parse_stack> stack_;
};


maybe<repo::section_kind> section_from_string (std::string const & s) {
#define X(a) { #a, repo::section_kind::a },
    static std::unordered_map<std::string, repo::section_kind> map = {
PSTORE_MCREPO_SECTION_KINDS
    };
#undef X
    auto pos = map.find (s);
    if (pos == map.end ()) {
        return nothing<repo::section_kind> ();
    }
    return just (pos->second);
}


class ifixup_state final : public callbacks_base {
public:
    explicit ifixup_state (gsl::not_null<parse_stack *> s, std::vector<pstore::repo::internal_fixup> * fixups);
    bool ok () const override {
        return callbacks_base::ok () && seen_.all ();
    }

private:
    void string_value (std::string const & s) override;
    void uint64_value (std::uint64_t v) override;
    void key (std::string const & k) override;
    void end_object () override;

    enum { good, section, type, offset, addend };
    std::bitset<addend + 1> seen_;
    std::uint64_t * v_ = nullptr;
    std::string * str_ = nullptr;

    gsl::not_null<std::vector<pstore::repo::internal_fixup> *> fixups_;

    std::string section_;
    std::uint64_t type_ = 0;
    std::uint64_t offset_ = 0;
    std::uint64_t addend_ = 0;
};

ifixup_state::ifixup_state (gsl::not_null<parse_stack *> s, std::vector<pstore::repo::internal_fixup> * fixups)
        : callbacks_base (s)
        , fixups_{fixups} {
    seen_ [good] = true;
}

void ifixup_state::string_value (std::string const & s) {
    if (str_ == nullptr) {
        seen_[good] = false;
        return;
    }
    *str_ = s;
    str_ = nullptr;
}

void ifixup_state::uint64_value (std::uint64_t v) {
    if (v_ == nullptr) {
        seen_[good] = false;
        return;
    }
    *v_ = v;
    v_ = nullptr;
}

void ifixup_state::key (std::string const & k) {
    v_ = nullptr;
    str_ = nullptr;
    if (k == "section") {
        seen_[section] = true;
        str_ = &section_;
    } else if (k == "type") {
        seen_[type] = true;
        v_ = &type_;
    } else if (k == "offset") {
        seen_[offset] = true;
        v_ = &offset_;
    } else if (k == "addend") {
        seen_[addend] = true;
        v_ = &addend_;
    } else {
        seen_ [good] = false;
        std::cerr << "Bad ifixup object key: " << k << '\n';
    }
}

void ifixup_state::end_object () {
    maybe<repo::section_kind> const scn = section_from_string (section_);
    seen_[section] = scn.has_value ();

    if (ok ()) {
        // TODO: validate more values here.
        fixups_->emplace_back (*scn,
                               static_cast <repo::relocation_type> (type_),
                               offset_,
                               addend_);
    } else {
        std::cerr << "ifixup object was bad\n";
    }
    stack()->pop (); // suicide.
}


class xfixup_state final : public callbacks_base {
public:
    xfixup_state (gsl::not_null<parse_stack *> s, gsl::not_null<std::vector<pstore::repo::external_fixup> *> fixups);
    bool ok () const override {
        return callbacks_base::ok () && seen_.all ();
    }

private:
    void uint64_value (std::uint64_t v) override;
    void key (std::string const & k) override;
    void end_object () override;

    enum { good, name, type, offset, addend };
    std::bitset<addend + 1> seen_;
    std::uint64_t * v_ = nullptr;
    gsl::not_null<std::vector<pstore::repo::external_fixup> *> fixups_;

    std::uint64_t name_ = 0;
    std::uint64_t type_ = 0;
    std::uint64_t offset_ = 0;
    std::uint64_t addend_ = 0;
};

xfixup_state::xfixup_state (gsl::not_null<parse_stack *> s, gsl::not_null<std::vector<pstore::repo::external_fixup> *> fixups)
        : callbacks_base (s)
        , fixups_{fixups} {
    seen_ [good] = true;
}

void xfixup_state::key (std::string const & k) {
    if (k == "name") {
        seen_[name] = true;
        v_ = &name_;
    } else if (k == "type") {
        seen_[type] = true;
        v_ = &type_;
    } else if (k == "offset") {
        seen_[offset] = true;
        v_ = &offset_;
    } else if (k == "addend") {
        seen_[addend] = true;
        v_ = &addend_;
    } else {
        seen_ [good] = false;
        v_ = nullptr;
        std::cerr << "Bad xfixup object key: " << k << '\n';
    }
}

void xfixup_state::uint64_value (std::uint64_t v) {
    if (v_ == nullptr) {
        seen_[good] = false;
        return;
    }
    *v_ = v;
    v_ = nullptr;
}

void xfixup_state::end_object () {
    if (ok ()) {
        // TODO: validate some values here.
        fixups_->emplace_back (typed_address<indirect_string>::make (name_),
                               static_cast <repo::relocation_type> (type_),
                               offset_,
                               addend_);
    } else {
        std::cerr << "xfixup object was bad\n";
    }
    stack()->pop (); // suicide.
}


template <typename T, typename Next>
class array_object : public callbacks_base {
public:
    array_object (gsl::not_null<parse_stack *> s, std::vector<T> * fixups) : callbacks_base (s), fixups_{fixups} {}
    void begin_object () override {
        stack()->push (std::make_unique<Next> (stack(), fixups_));
    }
    void end_array () override {
        stack()->pop (); // suicide.
    }
private:
    std::vector<T> * fixups_;
};

template <typename T, typename Next>
class array_consumer : public callbacks_base {
public:
    array_consumer (gsl::not_null<parse_stack *> s, std::vector<T> * arr) : callbacks_base (s), arr_{arr} {}
    void begin_array () override {
        auto stack = this->stack();
        auto arr = arr_;
        stack->pop (); // suicide
        stack->push (std::make_unique<Next> (stack, arr));
    }
private:
    std::vector<T> * const arr_;
};

class generic_section : public callbacks_base {
public:
    generic_section (gsl::not_null<parse_stack *> stack, gsl::not_null<database *> db);

    void string_value (std::string const & s);
    void uint64_value (std::uint64_t v) override;
    void key (std::string const & k) override;
    void end_object () override;
private:
    enum { good, data, align, ifixups, xfixups };
    std::bitset<xfixups + 1> seen_;
    std::uint64_t * v_ = nullptr;
    std::string * str_ = nullptr;

    gsl::not_null<database const *> db_;
    std::string data_;
    std::uint64_t align_ = 0;
    std::vector<pstore::repo::internal_fixup> ifixups_;
    std::vector<pstore::repo::external_fixup> xfixups_;
};

generic_section::generic_section (gsl::not_null<parse_stack *> stack, gsl::not_null<database *> db)
    : callbacks_base (stack)
    , db_{db} {
}

void generic_section::string_value (std::string const & s) {
    if (str_ == nullptr) {
        seen_[good] = false;
        return;
    }
    *str_ = s;
    str_ = nullptr;
}

void generic_section::uint64_value (std::uint64_t v) {
    if (v_ == nullptr) {
        seen_[good] = false;
        return;
    }
    *v_ = v;
    v_ = nullptr;
}

void generic_section::key (std::string const & k) {
    str_ = nullptr;
    v_ = nullptr;

    if (k == "data") {
        seen_[data] = true; // string (ascii85
        str_ = &data_;
    } else if (k == "align") {
        seen_[align] = true; // integer
        v_ = &align_;
    } else if (k == "ifixups") {
        seen_[ifixups] = true;
        stack()->push (std::make_unique<array_consumer<pstore::repo::internal_fixup, array_object<pstore::repo::internal_fixup, ifixup_state>>> (stack(), &ifixups_));
    } else if (k == "xfixups") {
        seen_[xfixups] = true;
        stack()->push (std::make_unique<array_consumer<pstore::repo::external_fixup,
                                                    array_object<pstore::repo::external_fixup, xfixup_state>>> (stack(), &xfixups_));
    } else {
        seen_ [good] = false;
        std::cerr << "Bad xfixup object key: " << k << '\n';
    }
}

void generic_section::end_object () {
    std::vector<std::uint8_t> data;
    from_base64 (std::begin(data_), std::end (data_), std::back_inserter (data));
std::cout << "create generic section\n";
}


auto input = ""
"{\n"
"  \"text\" : {\n"
"    \"data\": \"VUiJ5UiD7BC/BgAAAOgAAAAASL8AAAAAAAAAAInGsADoAAAAADHJiUX8ichIg8QQXcM=\",\n"
"    \"align\": 16,\n"
"    \"ifixups\": [\n"
"      {\n"
"        \"section\": \"text\",\n"
"        \"type\": 1,\n"
"        \"offset\": 3,\n"
"        \"addend\": 0\n"
"      }\n"
"    ],\n"
"    \"xfixups\": [\n"
"      {\n"
"        \"name\": 7,\n"
"        \"type\": 4,\n"
"        \"offset\": 14,\n"
"        \"addend\": 18446744073709551612\n"
"      },\n"
"       {\n"
"        \"name\": 5,\n"
"        \"type\": 1,\n"
"        \"offset\": 20,\n"
"        \"addend\": 0\n"
"      },\n"
"      {\n"
"        \"name\": 3,\n"
"        \"type\": 4,\n"
"        \"offset\": 33,\n"
"        \"addend\": 18446744073709551612\n"
"      }\n"
"    ]\n"
"  }\n"
"}\n"s;

#if 0
auto input = "[\n"
"{\n"
"  \"name\": 1,\n"
"  \"type\": 4,\n"
"  \"offset\": 14,\n"
"  \"addend\": 18446744073709551612\n"
"},\n"
"{\n"
"  \"name\": 2,\n"
"  \"type\": 5,\n"
"  \"offset\": 15,\n"
"  \"addend\": 75\n"
"}\n"
"]"s;
#endif


int main (int argc, char * argv[]) {

//"  \"name\": 7,\n"

    gsl::czstring db_path = argv[1];
    pstore::database db{db_path, pstore::database::access_mode::writable};

    auto parser = json::make_parser (callbacks::make<generic_section> (gsl::not_null<database *> (&db)));
    parser.input (input);
    parser.eof ();
    if (parser.has_error ()) {
        std::error_code const erc = parser.last_error ();
        //raise (erc);
        std::cerr << "Error: " << erc.message () << '\n';
    }
    return 0;

    try {
        pstore::database db{argv[1], pstore::database::access_mode::writable};
        FILE * infile = argc == 3 ? std::fopen(argv[2], "r") : stdin;
        if (infile == nullptr) {
            std::perror ("File opening failed");
            return EXIT_FAILURE;
        }

        auto parser = json::make_parser (callbacks::make<generic_section> (&db));
        for (int ch = std::getc (infile); ch != EOF; ch = std::getc (infile)) {
            auto const c = static_cast<char> (ch);
            parser.input (&c, &c + 1);
            if (parser.has_error ()) {
                std::error_code const erc = parser.last_error ();
                //raise (erc);
                std::cerr << "Error: " << erc.message () << '\n';
            }
        }
        parser.eof ();

        if (std::feof (infile)) {
            printf("\n End of file reached.");
        } else {
            printf("\n Something went wrong.");
        }
        if (infile != nullptr && infile != stdin) {
            std::fclose (infile);
            infile = nullptr;
        }
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
    } catch (...) {
        std::cerr << "Error: an unknown error occurred\n";
    }
}

