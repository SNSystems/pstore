#include "import_compilations.hpp"

#include "pstore/mcrepo/compilation.hpp"
#include "import_names.hpp"
#include "import_terminals.hpp"
#include "import_non_terminals.hpp"
#include "import_error.hpp"
#include "digest_from_string.hpp"

using namespace pstore;

template <typename T>
using not_null = pstore::gsl::not_null<T>;

//*     _      __ _      _ _   _           *
//*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
//* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
//* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
//*                                        *
class definition final : public state {
public:
    using container = std::vector<repo::compilation_member>;
    definition (parse_stack_pointer s, not_null<container *> definitions, names_pointer names)
            : state (s)
            , definitions_{definitions}
            , names_{names} {}

    gsl::czstring name () const noexcept override { return "definition"; }

    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

    static maybe<repo::linkage> decode_linkage (std::string const & linkage);
    static maybe<repo::visibility> decode_visibility (std::string const & visibility);

private:
    not_null<container *> const definitions_;
    names_pointer const names_;

    std::string digest_;
    std::uint64_t name_;
    std::string linkage_;
    std::string visibility_;
};

// key
// ~~~
std::error_code definition::key (std::string const & k) {
    if (k == "digest") {
        return push<string_rule> (&digest_);
    }
    if (k == "name") {
        return push<uint64_rule> (&name_);
    }
    if (k == "linkage") {
        return push<string_rule> (&linkage_);
    }
    if (k == "visibility") {
        return push<string_rule> (&visibility_);
    }
    return import_error::unknown_definition_object_key;
}

// end_object
// ~~~~~~~~~~
std::error_code definition::end_object () {
    maybe<index::digest> const digest = digest_from_string (digest_);
    if (!digest) {
        return import_error::bad_digest;
    }
    maybe<repo::linkage> const linkage = decode_linkage (linkage_);
    if (!linkage) {
        return import_error::bad_linkage;
    }
    maybe<repo::visibility> const visibility = decode_visibility (visibility_);
    if (!visibility) {
        return import_error::bad_visibility;
    }
    return pop ();
}

// decode_linkage
// ~~~~~~~~~~~~~~
maybe<repo::linkage> definition::decode_linkage (std::string const & linkage) {
#define X(a)                                                                                       \
    if (linkage == #a) {                                                                           \
        return just (repo::linkage::a);                                                            \
    }
    PSTORE_REPO_LINKAGES
#undef X
    return nothing<repo::linkage> ();
}

// decode_visibility
// ~~~~~~~~~~~~~~~~~
maybe<repo::visibility> definition::decode_visibility (std::string const & visibility) {
    if (visibility == "default") {
        return just (repo::visibility::default_vis);
    }
    if (visibility == "hidden") {
        return just (repo::visibility::hidden_vis);
    }
    if (visibility == "protected") {
        return just (repo::visibility::protected_vis);
    }
    return nothing<repo::visibility> ();
}


//*     _      __ _      _ _   _                _     _        _    *
//*  __| |___ / _(_)_ _ (_) |_(_)___ _ _    ___| |__ (_)___ __| |_  *
//* / _` / -_)  _| | ' \| |  _| / _ \ ' \  / _ \ '_ \| / -_) _|  _| *
//* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| \___/_.__// \___\__|\__| *
//*                                                |__/             *
class definition_object final : public state {
public:
    definition_object (parse_stack_pointer s, not_null<definition::container *> definitions,
                       not_null<names *> names)
            : state (s)
            , definitions_{definitions}
            , names_{names} {}
    gsl::czstring name () const noexcept override { return "definition_object"; }

    std::error_code begin_object () override {
        return this->push<definition> (definitions_, names_);
    }
    std::error_code end_array () override { return pop (); }

private:
    not_null<definition::container *> const definitions_;
    not_null<names *> const names_;
};

//*                    _ _      _   _           *
//*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _   *
//* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  *
//* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| *
//*              |_|                            *
class compilation final : public state {
public:
    compilation (parse_stack_pointer s, transaction_pointer transaction,
                 gsl::not_null<names *> names, index::digest digest)
            : state (s)
            , transaction_{transaction}
            , names_{names}
            , digest_{digest} {}

    gsl::czstring name () const noexcept override { return "compilation"; }

    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

private:
    transaction_pointer transaction_;
    not_null<names *> names_;
    uint128 const digest_;

    enum { path_index, triple_index };
    std::bitset<triple_index + 1> seen_;

    std::uint64_t path_ = 0;
    std::uint64_t triple_ = 0;
    definition::container definitions_;
};

// key
// ~~~
std::error_code compilation::key (std::string const & k) {
    if (k == "path") {
        seen_[path_index] = true;
        return push<uint64_rule> (&path_);
    }
    if (k == "triple") {
        seen_[triple_index] = true;
        return push<uint64_rule> (&triple_);
    }
    if (k == "definitions") {
        return push_array_rule<definition_object> (this, &definitions_, names_);
    }
    return import_error::unknown_compilation_object_key;
}

// end_object
// ~~~~~~~~~~
std::error_code compilation::end_object () {
    if (!seen_.all ()) {
        return import_error::incomplete_compilation_object;
    }
    // TODO: create the compilation!
    return pop ();
}


//*                    _ _      _   _               _         _          *
//*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _  ___ (_)_ _  __| |_____ __ *
//* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \(_-< | | ' \/ _` / -_) \ / *
//* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_/__/ |_|_||_\__,_\___/_\_\ *
//*              |_|                                                     *

pstore::gsl::czstring compilations_index::name () const noexcept {
    return "compilations_index";
}

std::error_code compilations_index::key (std::string const & s) {
    if (maybe<index::digest> const digest = digest_from_string (s)) {
        return push_object_rule<compilation> (this, transaction_, names_, index::digest{*digest});
    }
    return import_error::bad_digest;
}
std::error_code compilations_index::end_object () {
    return pop ();
}
