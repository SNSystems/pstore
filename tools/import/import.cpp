#include <bitset>
#include <cstdio>
#include <iostream>
#include <list>
#include <tuple>
#include <unordered_map>
#include <stack>

#include "pstore/core/database.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/json/json.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/os/file.hpp"
#include "pstore/os/memory_mapper.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/support/gsl.hpp"

#include "array_helpers.hpp"
#include "digest_from_string.hpp"
#include "import_fragment.hpp"
#include "import_names.hpp"
#include "import_error.hpp"
#include "import_non_terminals.hpp"
#include "import_terminals.hpp"
#include "import_compilations.hpp"

using namespace std::literals::string_literals;

using namespace pstore;

template <typename T>
using not_null = pstore::gsl::not_null<T>;


using parse_stack_pointer = not_null<state::parse_stack *>;


class fragment_object final : public state {
public:
    fragment_object (parse_stack_pointer s)
            : state (s) {}

private:
    std::error_code begin_object () { return {}; }
    std::error_code key (std::string const &) { return {}; }
    gsl::czstring name () const noexcept override { return "fragment_object"; }
};

class fragment_sections final : public state {
public:
    fragment_sections (parse_stack_pointer s)
            : state (s) {}

    gsl::czstring name () const noexcept override { return "fragment_sections"; }
    std::error_code key (std::string const & s) override;
    std::error_code end_object () override { return this->pop (); }

private:
    template <typename Content>
    struct create_consumer;
};


template <>
struct fragment_sections::create_consumer<pstore::repo::generic_section> {
    std::error_code operator() (state * const s) const {
        return s->push<object_rule<generic_section>> ();
    }
};

template <>
struct fragment_sections::create_consumer<pstore::repo::bss_section> {
    std::error_code operator() (state * const s) const {
        // TODO: implement BSS
        return {};
    }
};
template <>
struct fragment_sections::create_consumer<pstore::repo::debug_line_section> {
    std::error_code operator() (state * const s) const {
        return s->push<object_rule<debug_line_section>> ();
    }
};
template <>
struct fragment_sections::create_consumer<pstore::repo::dependents> {
    std::error_code operator() (state * const s) const {
        // TODO: implement dependents
        return {};
    }
};

std::error_code fragment_sections::key (std::string const & s) {
#define X(a) {#a, pstore::repo::section_kind::a},
    static std::unordered_map<std::string, pstore::repo::section_kind> const map{
        PSTORE_MCREPO_SECTION_KINDS};
#undef X
    auto const pos = map.find (s);
    if (pos == map.end ()) {
        return import_error::unknown_section_name;
    }
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a:                                                            \
        return create_consumer<                                                                    \
            pstore::repo::enum_to_section<pstore::repo::section_kind::a>::type>{}(this);

    switch (pos->second) {
        PSTORE_MCREPO_SECTION_KINDS
    case pstore::repo::section_kind::last: assert (false); // unreachable
    }
#undef X
    // digest_ = s; // decode the digest here.
    // return push (std::make_unique<object_consumer<int, next>> (stack (), &x));
    return {};
}

class debug_line_index final : public state {
public:
    debug_line_index (parse_stack_pointer s, transaction_pointer transaction)
            : state (s)
            , transaction_{transaction}
            , index_{pstore::index::get_index<pstore::trailer::indices::debug_line_header> (
                  transaction->db ())} {}

    gsl::czstring name () const noexcept override { return "debug_line_index"; }

    std::error_code string_value (std::string const & s) {
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
        index_->insert (*transaction_, std::make_pair (digest_, pstore::extent<std::uint8_t>{
                                                                    where, data.size ()}));
        return {};
    }

    std::error_code key (std::string const & s) override {
        if (maybe<uint128> const digest = digest_from_string (s)) {
            digest_ = *digest;
            return {};
        }
        return import_error::bad_digest;
    }
    std::error_code end_object () override { return pop (); }

private:
    transaction_pointer transaction_;
    std::shared_ptr<pstore::index::debug_line_header_index> index_;
    index::digest digest_;
};



class fragment_index final : public state {
public:
    fragment_index (parse_stack_pointer s, transaction_pointer transaction)
            : state (s)
            , transaction_{transaction} {}
    gsl::czstring name () const noexcept override { return "fragment_index"; }
    std::error_code key (std::string const & s) override {
        if (maybe<uint128> const digest = digest_from_string (s)) {
            digest_ = *digest;
            return push<object_rule<fragment_sections>> ();
        }
        return import_error::bad_digest;
    }
    std::error_code end_object () override { return pop (); }

private:
    transaction_pointer transaction_;
    uint128 digest_;
    std::vector<std::unique_ptr<pstore::repo::section_base>> sections_;
};


class transaction_contents final : public state {
public:
    transaction_contents (parse_stack_pointer stack, not_null<database *> db)
            : state (stack)
            , transaction_{begin (*db)}
            , names_{&transaction_} {}

private:
    std::error_code key (std::string const & s) override;
    std::error_code end_object () override;
    gsl::czstring name () const noexcept override;

    transaction_type transaction_;
    names names_;
};

gsl::czstring transaction_contents::name () const noexcept {
    return "transaction_contents";
}
std::error_code transaction_contents::key (std::string const & s) {
    // TODO: check that "names" is the first key that we see.
    if (s == "names") {
        return push_array_rule<names_array_members> (this, &names_);
    }
    if (s == "debugline") {
        return push_object_rule<debug_line_index> (this, &transaction_);
    }
    if (s == "fragments") {
        return push_object_rule<fragment_index> (this, &transaction_);
    }
    if (s == "compilations") {
        return push_object_rule<compilations_index> (this, &transaction_, &names_);
    }
    return import_error::unknown_transaction_object_key;
}

std::error_code transaction_contents::end_object () {
    names_.flush ();
    transaction_.commit ();
    return pop ();
}


class transaction_object final : public state {
public:
    transaction_object (parse_stack_pointer s, not_null<database *> db)
            : state (s)
            , db_{db} {}
    gsl::czstring name () const noexcept override { return "transaction_object"; }
    std::error_code begin_object () override { return push<transaction_contents> (db_); }
    std::error_code end_array () override { return pop (); }

private:
    not_null<database *> db_;
};

class transaction_array final : public state {
public:
    transaction_array (parse_stack_pointer s, not_null<database *> db)
            : state (s)
            , db_{db} {}
    gsl::czstring name () const noexcept override { return "transaction_array"; }
    std::error_code begin_array () override { return this->replace_top<transaction_object> (db_); }

private:
    not_null<database *> db_;
};

class root_object final : public state {
public:
    root_object (parse_stack_pointer stack, not_null<database *> db)
            : state (stack)
            , db_{db} {}
    gsl::czstring name () const noexcept;
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

private:
    not_null<database *> db_;

    enum { version, transactions };
    std::bitset<transactions + 1> seen_;
    std::uint64_t version_ = 0;
};

gsl::czstring root_object::name () const noexcept {
    return "root_object";
}

std::error_code root_object::key (std::string const & k) {
    // TODO: check that 'version' is the first key that we see.
    if (k == "version") {
        seen_[version] = true;
        return push<uint64_rule> (&version_);
    }
    if (k == "transactions") {
        seen_[transactions] = true;
        return push<transaction_array> (db_);
    }
    return import_error::unrecognized_root_key;
}

std::error_code root_object::end_object () {
    if (!seen_.all ()) {
        return import_error::root_object_was_incomplete;
    }
    return {};
}


class root final : public state {
public:
    root (parse_stack_pointer stack, not_null<database *> db)
            : state (stack)
            , db_{db} {}
    gsl::czstring name () const noexcept override;
    std::error_code begin_object () override;

private:
    not_null<database *> db_;
};

gsl::czstring root::name () const noexcept {
    return "root";
}

std::error_code root::begin_object () {
    return push<root_object> (db_);
}



int main (int argc, char * argv[]) {
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
            // std::cout << c;
            parser.input (&c, &c + 1);
            if (parser.has_error ()) {
                std::error_code const erc = parser.last_error ();
                // raise (erc);
                std::cerr << "Value: " << erc.value () << '\n';
                std::cerr << "Error: " << erc.message () << '\n';
                std::cout << "Row " << std::get<parser_type::row_index> (parser.coordinate ())
                          << ", column "
                          << std::get<parser_type::column_index> (parser.coordinate ()) << '\n';
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

