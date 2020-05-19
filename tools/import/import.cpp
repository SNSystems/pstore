#include <bitset>
#include <cstdio>
#include <iostream>
#include <list>
#include <unordered_map>
#include <stack>

#include "pstore/json/json.hpp"
#include "pstore/os/file.hpp"
#include "pstore/os/memory_mapper.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/core/indirect_string.hpp"

#include "array_helpers.hpp"
#include "json_callbacks.hpp"

using namespace std::literals::string_literals;

using namespace pstore;
using transaction_type = transaction<transaction_lock>;

class names {
public:
    explicit names (gsl::not_null<transaction_type *> transaction)
            : transaction_{transaction}
            , names_index_{
                  pstore::index::get_index<pstore::trailer::indices::name> (transaction->db ())} {}

    void add_string (std::string const & str) {
        strings_.push_back (str);
        std::string const & x = strings_.back ();
        views_.emplace_back (pstore::make_sstring_view (x));
        auto & s = views_.back ();
        adder_.add (*transaction_, names_index_, &s);
    }
    void flush () { adder_.flush (*transaction_); }

private:
    gsl::not_null<transaction_type *> transaction_;
    std::shared_ptr<pstore::index::name_index> names_index_;

    pstore::indirect_string_adder adder_;
    std::list<std::string> strings_;
    std::vector<pstore::raw_sstring_view> views_;
};

class names_array_members final : public state {
public:
    names_array_members (gsl::not_null<parse_stack *> s, gsl::not_null<names *> n)
            : state (s)
            , names_{n} {}
    std::error_code string_value (std::string const & str) override {
        names_->add_string (str);
        return {};
    }
    std::error_code end_array () override { return this->suicide (); }

private:
    gsl::not_null<names *> names_;
};

class transaction_contents final : public state {
public:
    transaction_contents (gsl::not_null<parse_stack *> stack,
                          gsl::not_null<transaction_type *> transaction)
            : state (stack)
            , transaction_{transaction}
            , names_{transaction} {}
    std::error_code key (std::string const & s) override;
    std::error_code end_object () override;

private:
    gsl::not_null<transaction_type *> transaction_;
    names names_;
};

std::error_code transaction_contents::key (std::string const & s) {
    std::cout << "transaction::" << s << '\n';
    if (s == "names") {
        stack ()->push (
            std::make_unique<array_consumer<names, names_array_members>> (stack (), &names_));
        return {};
    }
    if (s == "fragments") {
        return {};
    }
    if (s == "compilations") {
        return {};
    }
    return make_error_code (import_error::unknown_transaction_object_key);
}

std::error_code transaction_contents::end_object () {
    names_.flush ();
    transaction_->commit ();
    return this->suicide ();
}


class transaction_object final : public state {
public:
    transaction_object (gsl::not_null<parse_stack *> s, gsl::not_null<database *> db)
            : state (s)
            , transaction_{begin (*db)} {}
    std::error_code begin_object () override {
        stack ()->push (std::make_unique<transaction_contents> (stack (), &transaction_));
        return {};
    }
    std::error_code end_array () override { return this->suicide (); }

private:
    transaction_type transaction_;
};

class transaction_array final : public state {
public:
    transaction_array (gsl::not_null<parse_stack *> s, gsl::not_null<database *> db)
            : state (s)
            , db_{db} {}
    std::error_code begin_array () override;

private:
    gsl::not_null<database *> db_;
};

std::error_code transaction_array::begin_array () {
    auto stack = this->stack ();
    this->suicide ();
    stack->push (std::make_unique<transaction_object> (stack, db_));
    return {};
}

class root_object final : public state {
public:
    root_object (gsl::not_null<parse_stack *> stack, gsl::not_null<database *> db)
            : state (stack)
            , db_{db} {}
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

private:
    gsl::not_null<database *> db_;

    enum { version, transactions };
    std::bitset<transactions + 1> seen_;
    std::uint64_t version_ = 0;
};

std::error_code root_object::key (std::string const & k) {
    if (k == "version") {
        seen_[version] = true;
        stack ()->push (std::make_unique<expect_uint64> (stack (), &version_));
        return {};
    }
    if (k == "transactions") {
        seen_[transactions] = true;
        stack ()->push (std::make_unique<transaction_array> (stack (), db_));
        return {};
    }
    return make_error_code (import_error::unrecognized_root_key);
}

std::error_code root_object::end_object () {
    if (!seen_.all ()) {
        return make_error_code (import_error::root_object_was_incomplete);
    }
    return {};
}


class root final : public state {
public:
    root (gsl::not_null<parse_stack *> stack, gsl::not_null<database *> db)
            : state (stack)
            , db_{db} {}
    std::error_code begin_object () override {
        stack ()->push (std::make_unique<root_object> (stack (), db_));
        return {};
    }

private:
    gsl::not_null<database *> db_;
};



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

#if 0
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
#endif

try {
    pstore::database db{argv[1], pstore::database::access_mode::writable};
    FILE * infile = argc == 3 ? std::fopen (argv[2], "r") : stdin;
    if (infile == nullptr) {
        std::perror ("File opening failed");
        return EXIT_FAILURE;
    }

    auto parser = json::make_parser (callbacks::make<root> (&db));
    for (int ch = std::getc (infile); ch != EOF; ch = std::getc (infile)) {
        auto const c = static_cast<char> (ch);
        parser.input (&c, &c + 1);
        if (parser.has_error ()) {
            std::error_code const erc = parser.last_error ();
            // raise (erc);
            std::cerr << "Error: " << erc.message () << '\n';
            std::cout << "Row " << std::get<1> (parser.coordinate ()) << ", column "
                      << std::get<0> (parser.coordinate ()) << '\n';
            break;
        }
    }
    parser.eof ();

    if (std::feof (infile)) {
        printf ("\n End of file reached.");
    } else {
        printf ("\n Something went wrong.");
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

