#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "pstore/core/database.hpp"
#include "pstore/core/generation_iterator.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/diff/diff.hpp"
#include "pstore/mcrepo/compilation.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/support/gsl.hpp"

class stdio_output : public std::iterator<std::output_iterator_tag, char> {
public:
    stdio_output & operator= (char c) {
        std::fputc (c, stdout);
        return *this;
    }
    stdio_output & operator* () noexcept { return *this; }
    stdio_output & operator++ () noexcept { return *this; }
    stdio_output operator++ (int) noexcept { return *this; }
};


namespace {

    constexpr bool comments = false;

    auto footers (pstore::database const & db) {
        auto const num_generations = db.get_current_revision () + 1;
        std::vector<pstore::typed_address<pstore::trailer>> footers;
        footers.reserve (num_generations);

        pstore::generation_container generations{db};
        std::copy (std::begin (generations), std::end (generations), std::back_inserter (footers));
        assert (footers.size () == num_generations);
        std::reverse (std::begin (footers), std::end (footers));
        return footers;
    }

    std::string get_string (pstore::database const & db,
                            pstore::typed_address<pstore::indirect_string> addr) {
        return pstore::serialize::read<pstore::indirect_string> (
                   pstore::serialize::archive::database_reader{db, addr.to_address ()})
            .to_string ();
    }

    void show_string (pstore::database const & db,
                      pstore::typed_address<pstore::indirect_string> addr) {
        if (comments) {
            std::printf ("  // \"%s\"", get_string (db, addr).c_str ());
        }
    }


    class name_mapping {
    public:
        void add (pstore::address addr) {
            assert (names_.find (addr) == names_.end ());
            assert (names_.size () <= std::numeric_limits<std::uint64_t>::max ());
            names_[addr] = static_cast <std::uint64_t> (names_.size ());
        }
        std::uint64_t index (pstore::address addr) const {
            auto const pos = names_.find (addr);
            assert (pos != names_.end ());
            return pos->second;
        }
        std::uint64_t index (pstore::typed_address<pstore::indirect_string> addr) const {
            return index (addr.to_address ());
        }

    private:
        std::unordered_map<pstore::address, std::uint64_t> names_;
    };

    template <typename Iterator>
    void emit_string (Iterator first, Iterator last) {
        std::fputc ('"', stdout);
        auto pos = first;
        while ((pos = std::find_if (first, last, [] (char c) { return c == '"' || c == '\\'; })) !=
               last) {
            std::fwrite (&*first, sizeof (char), pos - first, stdout);
            std::fputc ('\\', stdout);
            std::fputc (*pos, stdout);
            first = pos + 1;
        }
        if (first != last) {
            std::fwrite (&*first, sizeof (char), last - first, stdout);
        }
        std::fputc ('"', stdout);
    }

    void emit_string (pstore::raw_sstring_view const & view) {
        emit_string (std::begin (view), std::end (view));
    }

    template <typename Iterator, typename Function>
    void emit_array (Iterator first, Iterator last, pstore::gsl::czstring indent, Function fn) {
        auto sep = "\n";
        auto tail_sep = "";
        auto tail_sep_indent = "";
        std::fputs ("[", stdout);
        std::for_each (first, last, [&] (auto const & element) {
            std::fputs (sep, stdout);
            fn (element);
            sep = ",\n";
            tail_sep = "\n";
            tail_sep_indent = indent;
        });
        std::fputs (tail_sep, stdout);
        std::fputs (tail_sep_indent, stdout);
        std::fputs ("]", stdout);
    }

#define INDENT1 "  "
#define INDENT2 INDENT1 INDENT1
#define INDENT3 INDENT2 INDENT1
#define INDENT4 INDENT3 INDENT1
#define INDENT5 INDENT4 INDENT1
#define INDENT6 INDENT5 INDENT1
#define INDENT7 INDENT6 INDENT1
#define INDENT8 INDENT7 INDENT1
#define INDENT9 INDENT8 INDENT1


    void names (pstore::database const & db, unsigned const generation,
                name_mapping * const string_table) {

        auto strings = pstore::index::get_index<pstore::trailer::indices::name> (db);
        auto const container = pstore::diff::diff (db, *strings, generation - 1U);
        emit_array (std::begin (container), std::end (container), INDENT3,
                    [&] (pstore::address addr) {
                        pstore::indirect_string const str = strings->load_leaf_node (db, addr);
                        pstore::shared_sstring_view owner;
                        pstore::raw_sstring_view view = str.as_db_string_view (&owner);
                        std::fputs (INDENT4, stdout);
                        emit_string (view);
                        string_table->add (addr);
                    });
    }

    pstore::gsl::czstring section_name (pstore::repo::section_kind section) {
        auto result = "unknown";
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a: result = #a; break;
        switch (section) {
            PSTORE_MCREPO_SECTION_KINDS
        case pstore::repo::section_kind::last: break;
        }
#undef X
        return result;
    }

    void
    emit_section_ifixups (pstore::repo::container<pstore::repo::internal_fixup> const & ifixups) {
        printf (INDENT7 "\"ifixups\": ");
        emit_array (std::begin (ifixups), std::end (ifixups), INDENT7,
                    [] (pstore::repo::internal_fixup const & ifx) {
                        std::printf (INDENT8 "{\n");
                        std::printf (INDENT9 "\"section\": \"%s\",\n", section_name (ifx.section));
                        std::printf (INDENT9 "\"type\": %u,\n", ifx.type);
                        std::printf (INDENT9 "\"offset\": %" PRIu64 ",\n", ifx.offset);
                        std::printf (INDENT9 "\"addend\": %" PRIu64 "\n", ifx.addend);
                        std::printf (INDENT8 "}");
                    });
    }

    void
    emit_section_xfixups (pstore::database const & db, name_mapping const & names,
                          pstore::repo::container<pstore::repo::external_fixup> const & xfixups) {
        std::printf (INDENT7 "\"xfixups\": ");
        emit_array (std::begin (xfixups), std::end (xfixups), INDENT7,
                    [&] (pstore::repo::external_fixup const & xfx) {
                        std::printf (INDENT8 "{\n");
                        std::printf (INDENT9 "\"name\": %" PRIu64 ",", names.index (xfx.name));
                        show_string (db, xfx.name);
                        std::printf ("\n");
                        std::printf (INDENT9 "\"type\": %u,\n", xfx.type);
                        std::printf (INDENT9 "\"offset\": %" PRIu64 ",\n", xfx.offset);
                        std::printf (INDENT9 "\"addend\": %" PRIu64 "\n", xfx.addend);
                        std::printf (INDENT8 "}");
                    });
    }

    template <pstore::repo::section_kind Kind,
              typename Content = typename pstore::repo::enum_to_section<Kind>::type>
    void emit_section (pstore::database const & db, name_mapping const & names,
                       Content const & content) {
        {
            std::printf (INDENT7 "\"data\": \"");
            pstore::repo::container<std::uint8_t> const payload = content.payload ();
            pstore::to_base64 (std::begin (payload), std::end (payload), stdio_output{});
            std::printf ("\",\n");
        }
        std::printf (INDENT7 "\"align\": %u,\n", content.align ());
        emit_section_ifixups (content.ifixups ());
        std::printf (",\n");
        emit_section_xfixups (db, names, content.xfixups ());
        std::printf ("\n");
    }

    template <>
    void emit_section<pstore::repo::section_kind::bss, pstore::repo::bss_section> (
        pstore::database const & db, name_mapping const & names,
        pstore::repo::bss_section const & bss) {
        std::printf (INDENT7 "\"size\": %d\n", bss.size ());
        std::printf (INDENT7 "\"align\": %d\n", bss.align ());
        emit_section_ifixups (bss.ifixups ());
        std::printf (",\n");
        emit_section_xfixups (db, names, bss.xfixups ());
        std::printf ("\n");
    }

    template <>
    void emit_section<pstore::repo::section_kind::dependent, pstore::repo::dependents> (
        pstore::database const & db, name_mapping const & names,
        pstore::repo::dependents const & dependents) {

        emit_array (std::begin (dependents), std::end (dependents), INDENT7,
                    [&] (pstore::typed_address<pstore::repo::compilation_member> const & d) {
                        // FIXME: how to represent this in the JSON?
                    });
    }

    void fragments (pstore::database const & db, unsigned const generation,
                    name_mapping const & names) {
        auto fragments = pstore::index::get_index<pstore::trailer::indices::fragment> (db);
        if (!fragments->empty ()) {
            auto fragment_sep = "\n";
            assert (generation > 0U);
            for (pstore::address const addr :
                 pstore::diff::diff (db, *fragments, generation - 1U)) {
                auto const & kvp = fragments->load_leaf_node (db, addr);
                std::string const digest = kvp.first.to_hex_string ();
                std::printf ("%s" INDENT4 "\"%s\": [\n", fragment_sep, digest.c_str ());

                auto fragment = db.getro (kvp.second);
                auto section_sep = INDENT5;
                for (pstore::repo::section_kind section : *fragment) {
                    std::printf ("%s{\n" INDENT6 "\"%s\": {\n", section_sep,
                                 section_name (section));
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a:                                                            \
        emit_section<pstore::repo::section_kind::a> (                                              \
            db, names, fragment->at<pstore::repo::section_kind::a> ());                            \
        break;
                    switch (section) {
                        PSTORE_MCREPO_SECTION_KINDS
                    case pstore::repo::section_kind::last:
                        // llvm_unreachable...
                        assert (false);
                        break;
                    }
#undef X
                    std::printf (INDENT6 "}\n");
                    std::printf (INDENT5 "}");
                    section_sep = ",\n" INDENT5;
                }
                std::printf ("\n" INDENT4 "]");
                fragment_sep = ",\n";
            }
        }
    }

#define X(a)                                                                                       \
    case pstore::repo::linkage::a: result = #a; break;
    pstore::gsl::czstring linkage_name (pstore::repo::linkage linkage) {
        auto result = "unknown";
        switch (linkage) { PSTORE_REPO_LINKAGES }
        return result;
    }
#undef X

    pstore::gsl::czstring visibility_name (pstore::repo::visibility visibility) {
        auto result = "unknown";
        switch (visibility) {
        case pstore::repo::visibility::default_vis: result = "default"; break;
        case pstore::repo::visibility::hidden_vis: result = "hidden"; break;
        case pstore::repo::visibility::protected_vis: result = "protected"; break;
        }
        return result;
    }

    void compilations (pstore::database const & db, unsigned const generation,
                       name_mapping const & names) {
        auto compilations = pstore::index::get_index<pstore::trailer::indices::compilation> (db);
        if (!compilations->empty ()) {
            auto sep = "\n";
            assert (generation > 0);
            for (pstore::address const addr :
                 pstore::diff::diff (db, *compilations, generation - 1U)) {
                auto const & kvp = compilations->load_leaf_node (db, addr);
                printf ("%s" INDENT4 "\"%s\": {\n", sep, kvp.first.to_hex_string ().c_str ());

                auto compilation = db.getro (kvp.second);
                printf (INDENT5 "\"path\": %" PRIu64 ",", names.index (compilation->path ()));
                show_string (db, compilation->path ());
                printf ("\n");
                printf (INDENT5 "\"triple\": %" PRIu64 ",", names.index (compilation->triple ()));
                show_string (db, compilation->triple ());
                printf ("\n");
                printf (INDENT5 "\"definitions\": ");
                emit_array (compilation->begin (), compilation->end (), INDENT5,
                            [&] (pstore::repo::compilation_member const & d) {
                                printf (INDENT6 "{\n");
                                printf (INDENT7 "\"digest\": \"%s\",\n",
                                        d.digest.to_hex_string ().c_str ());
                                printf (INDENT7 "\"name\": %" PRIu64 ",", names.index (d.name));
                                show_string (db, d.name);
                                printf ("\n");
                                printf (INDENT7 "\"linkage\": \"%s\",\n",
                                        linkage_name (d.linkage ()));
                                printf (INDENT7 "\"visibility\": \"%s\"\n",
                                        visibility_name (d.visibility ()));
                                printf (INDENT6 "}");
                            });
                printf ("\n" INDENT4 "}");
                sep = ",\n";
            }
        }
    }

} // end anonymous namespace

int main (int argc, char * argv[]) {
#if 0
    emit_string ("hello"); printf ("\n");
    emit_string ("a \" b"); printf ("\n");
    emit_string ("\\"); printf ("\n");
#endif

    pstore::database db{argv[1], pstore::database::access_mode::read_only};

    name_mapping string_table;
    std::printf ("{\n");
    std::printf (INDENT1 "\"version\": 1,\n");
    std::printf (INDENT1 "\"generations\": ");

    auto const f = footers (db);
    emit_array (std::begin (f), std::end (f), INDENT1, [&] (pstore::typed_address<pstore::trailer> footer_pos) {
        auto const footer = db.getro (footer_pos);
        unsigned const generation = footer->a.generation;
        db.sync (generation);
        std::printf (INDENT2 "{\n");
        if (comments) {
            std::printf (INDENT3 "// generation %u\n", generation);
        }
        std::printf (INDENT3 "\"time\": %" PRIu64 ",\n", footer->a.time.load ());
        std::printf (INDENT3 "\"names\": ");
        names (db, generation, &string_table);
        std::printf (",\n" INDENT3 "\"fragments\": {");
        fragments (db, generation, string_table);
        std::printf ("\n" INDENT3 "},\n");
        std::printf (INDENT3 "\"compilations\": {");
        compilations (db, generation, string_table);
        std::printf ("\n" INDENT3 "}\n");
        std::printf (INDENT2 "}");
    });
    std::printf ("\n}\n");
}
