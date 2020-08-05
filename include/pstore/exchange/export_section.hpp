#ifndef PSTORE_EXCHANGE_EXPORT_SECTION_HPP
#define PSTORE_EXCHANGE_EXPORT_SECTION_HPP

#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/exchange/export_fixups.hpp"

namespace pstore {
    namespace exchange {

        template <repo::section_kind Kind,
                  typename Content = typename repo::enum_to_section<Kind>::type>
        void emit_section (crude_ostream & os, database const & db,
                           export_name_mapping const & names, Content const & content) {
            {
                os << indent6 << R"("data": ")";
                repo::container<std::uint8_t> const payload = content.payload ();
                to_base64 (std::begin (payload), std::end (payload), ostream_inserter{os});
                os << "\",\n";
            }
            os << indent6 << R"("align": )" << content.align () << ",\n";

            repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
            os << indent6 << R"("ifixups": )";
            export_internal_fixups (os, std::begin (ifixups), std::end (ifixups)) << ",\n";

            repo::container<repo::external_fixup> const xfixups = content.xfixups ();
            os << indent6 << R"("xfixups" :)";
            export_external_fixups (os, db, names, std::begin (xfixups), std::end (xfixups))
                << '\n';
        }

        template <>
        void emit_section<repo::section_kind::bss, repo::bss_section> (
            crude_ostream & os, database const & db, export_name_mapping const & names,
            repo::bss_section const & content) {

            os << indent6 << R"("size": )" << content.size () << ",\n";
            os << indent6 << R"("align": )" << content.align () << ",\n";

            repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
            os << indent6 << R"("ifixups": )";
            export_internal_fixups (os, std::begin (ifixups), std::end (ifixups)) << ",\n";

            repo::container<repo::external_fixup> const xfixups = content.xfixups ();
            os << indent6 << R"("xfixups" :)";
            export_external_fixups (os, db, names, std::begin (xfixups), std::end (xfixups))
                << '\n';
        }

        template <>
        void emit_section<repo::section_kind::debug_line, repo::debug_line_section> (
            crude_ostream & os, database const & /*db*/, export_name_mapping const & /*names*/,
            repo::debug_line_section const & content) {

            assert (content.align () == 1U);
            assert (content.xfixups ().size () == 0U);

            os << indent6 << R"("header": ")" << content.header_digest ().to_hex_string ()
               << "\",\n";

            {
                os << indent6 << R"("data": ")";
                repo::container<std::uint8_t> const payload = content.payload ();
                to_base64 (std::begin (payload), std::end (payload), ostream_inserter{os});
                os << "\",\n";
            }

            repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
            os << indent6 << R"("ifixups": )";
            export_internal_fixups (os, std::begin (ifixups), std::end (ifixups)) << '\n';
        }

        template <>
        void emit_section<repo::section_kind::dependent, repo::dependents> (
            crude_ostream & os, database const & /*db*/, export_name_mapping const & /*names*/,
            repo::dependents const & content) {

            emit_array (
                os, std::begin (content), std::end (content), indent6,
                [] (crude_ostream & os1, typed_address<repo::compilation_member> const & d) {
                    // FIXME: represent as a Relative JSON Pointer
                    // (https://json-schema.org/draft/2019-09/relative-json-pointer.html).
                });
        }


    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_SECTION_HPP
