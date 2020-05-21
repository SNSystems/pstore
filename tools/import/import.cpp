#include <bitset>
#include <cstdio>
#include <iostream>
#include <list>
#include <tuple>
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
    std::list<pstore::raw_sstring_view> views_;
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
    std::error_code end_array () override { return pop (); }

private:
    gsl::not_null<names *> names_;
};

class fragment_object final : public state {
public:
    fragment_object (gsl::not_null<parse_stack *> s)
            : state (s) {}
    std::error_code begin_object () { return {}; }
    std::error_code key (std::string const & s) {}
};



class fragment_index final : public state {
public:
    fragment_index (gsl::not_null<parse_stack *> s, gsl::not_null<transaction_type *> transaction)
            : state (s)
            , transaction_{transaction} {}
    std::error_code begin_object () override {
        // key is digest, value fragment.
        return {};
    }
    int x;
    std::error_code key (std::string const & s) override {
        digest_ = s; // deocde the digest here.
        // return push (std::make_unique<object_consumer<int, next>> (stack (), &x));
        return {};
    }
    std::error_code end_object () override { return pop (); }

private:
    gsl::not_null<transaction_type *> transaction_;
    std::string digest_;
    std::vector<std::unique_ptr<pstore::repo::section_base>> sections_;
};

class compilations_index final : public state {
public:
    compilations_index (gsl::not_null<parse_stack *> s,
                        gsl::not_null<transaction_type *> transaction)
            : state (s)
            , transaction_{transaction} {}
    std::error_code begin_object () override {
        // key is digest, value compilation.
        return {};
    }
    int x;
    std::error_code key (std::string const & s) override {
        digest_ = s; // deocde the digest here.
        // return push (std::make_unique<object_consumer<int, next>> (stack (), &x));
        return {};
    }
    std::error_code end_object () override { return pop (); }

private:
    gsl::not_null<transaction_type *> transaction_;
    std::string digest_;
    //    std::vector<std::unique_ptr<pstore::repo::section_base>> sections_;
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
    if (s == "names") {
        return push<array_consumer<names, names_array_members>> (&names_);
    }
    if (s == "fragments") {
        object_consumer<fragment_index, gsl::not_null<transaction_type *>> c{stack (),
                                                                             transaction_};
        // return push<object_consumer<fragment_index>> (make_object_consumer<fragment_index> (stack
        // (), transaction_));
        //        return push<fragment_index> (transaction_);
    }
    if (s == "compilations") {
        return push<compilations_index> (transaction_);
    }
    return make_error_code (import_error::unknown_transaction_object_key);
}

std::error_code transaction_contents::end_object () {
    names_.flush ();
    transaction_->commit ();
    return pop ();
}


class transaction_object final : public state {
public:
    transaction_object (gsl::not_null<parse_stack *> s, gsl::not_null<database *> db)
            : state (s)
            , transaction_{begin (*db)} {}
    std::error_code begin_object () override { return push<transaction_contents> (&transaction_); }
    std::error_code end_array () override { return pop (); }

private:
    transaction_type transaction_;
};

class transaction_array final : public state {
public:
    transaction_array (gsl::not_null<parse_stack *> s, gsl::not_null<database *> db)
            : state (s)
            , db_{db} {}
    std::error_code begin_array () override { return this->replace_top<transaction_object> (db_); }

private:
    gsl::not_null<database *> db_;
};

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
        return push<expect_uint64> (&version_);
    }
    if (k == "transactions") {
        seen_[transactions] = true;
        return push<transaction_array> (db_);
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
    using parser_type = decltype (parser);
    for (int ch = std::getc (infile); ch != EOF; ch = std::getc (infile)) {
        auto const c = static_cast<char> (ch);
        std::cout << c;
        parser.input (&c, &c + 1);
        if (parser.has_error ()) {
            std::error_code const erc = parser.last_error ();
            // raise (erc);
            std::cerr << "Error: " << erc.message () << '\n';
            std::cout << "Row " << std::get<parser_type::row_index> (parser.coordinate ())
                      << ", column " << std::get<parser_type::column_index> (parser.coordinate ())
                      << '\n';
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

