#ifndef PSTORE_EXCHANGE_IMPORT_LINKED_DEFINITIONS_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_LINKED_DEFINITIONS_SECTION_HPP

#include <bitset>
#include <vector>

#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/linked_definitions_section.hpp"

#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {

        using linked_definitions_container = std::vector<repo::linked_definitions::value_type>;

        template <typename OutputIterator>
        class import_linked_definition final : public import_rule {
        public:
            import_linked_definition (parse_stack_pointer const stack,
                                      not_null<OutputIterator *> const out) noexcept
                    : import_rule (stack)
                    , out_{out} {}
            import_linked_definition (import_linked_definition const &) = delete;
            import_linked_definition (import_linked_definition &&) = delete;

            ~import_linked_definition () override = default;

            import_linked_definition & operator= (import_linked_definition const &) = delete;
            import_linked_definition & operator= (import_linked_definition &&) = delete;

            gsl::czstring name () const noexcept override { return "linked definition"; }

            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            not_null<OutputIterator *> const out_;

            enum { compilation, index };
            std::bitset<index + 1> seen_;

            std::string compilation_;
            std::uint64_t index_ = 0U;
        };

        // key
        // ~~~
        template <typename OutputIterator>
        std::error_code import_linked_definition<OutputIterator>::key (std::string const & k) {
            if (k == "compilation") {
                seen_[compilation] = true;
                return this->push<string_rule> (&compilation_);
            }
            if (k == "index") {
                seen_[index] = true;
                return this->push<uint64_rule> (&index_);
            }
            return import_error::unrecognized_section_object_key;
        }

        // end object
        // ~~~~~~~~~~
        template <typename OutputIterator>
        std::error_code import_linked_definition<OutputIterator>::end_object () {
            if (!seen_.all ()) {
                return import_error::incomplete_linked_definition_object;
            }
            auto const compilation = index::digest::from_hex_string (compilation_);
            if (!compilation) {
                return import_error::bad_digest;
            }
            // TODO: range check index.

            auto const index = static_cast<std::uint32_t> (index_);
            **out_ = repo::linked_definitions::value_type (
                *compilation, index, typed_address<repo::compilation_member>::make (index_));
            return pop ();
        }


        //-MARK: import linked definitions section
        template <typename OutputIterator>
        class import_linked_definitions_section final : public import_rule {
        public:
            import_linked_definitions_section (parse_stack_pointer const stack,
                                               not_null<linked_definitions_container *> const ld,
                                               not_null<OutputIterator *> const out)
                    : import_rule (stack)
                    , ld_{ld}
                    , out_{out}
                    , links_out_{std::back_inserter (*ld)} {}

            import_linked_definitions_section (import_linked_definitions_section const &) = delete;
            import_linked_definitions_section (import_linked_definitions_section &&) noexcept =
                delete;

            ~import_linked_definitions_section () noexcept override = default;

            import_linked_definitions_section &
            operator= (import_linked_definitions_section const &) = delete;
            import_linked_definitions_section &
            operator= (import_linked_definitions_section &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "linked definitions section"; }
            std::error_code begin_object () override {
                return this->push<import_linked_definition<decltype (links_out_)>> (&links_out_);
            }
            std::error_code end_array () override;

        private:
            not_null<linked_definitions_container *> const ld_;
            not_null<OutputIterator *> const out_;

            std::back_insert_iterator<linked_definitions_container> links_out_;
        };

        template <typename OutputIterator>
        std::error_code import_linked_definitions_section<OutputIterator>::end_array () {
            auto const * const data = ld_->data ();
            *out_ = std::make_unique<repo::linked_definitions_creation_dispatcher> (
                data, data + ld_->size ());
            return pop ();
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_LINKED_DEFINITIONS_SECTION_HPP
