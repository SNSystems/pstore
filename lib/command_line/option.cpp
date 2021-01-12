//*              _   _              *
//*   ___  _ __ | |_(_) ___  _ __   *
//*  / _ \| '_ \| __| |/ _ \| '_ \  *
//* | (_) | |_) | |_| | (_) | | | | *
//*  \___/| .__/ \__|_|\___/|_| |_| *
//*       |_|                       *
//===- lib/command_line/option.cpp ----------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
#include "pstore/command_line/option.hpp"

namespace pstore {
    namespace command_line {

            gsl::czstring type_description<std::string>::value = "str";
            gsl::czstring type_description<int>::value = "int";
            gsl::czstring type_description<long>::value = "int";
            gsl::czstring type_description<long long>::value = "int";
            gsl::czstring type_description<unsigned short>::value = "uint";
            gsl::czstring type_description<unsigned int>::value = "uint";
            gsl::czstring type_description<unsigned long>::value = "uint";
            gsl::czstring type_description<unsigned long long>::value = "uint";

            //*           _   _           *
            //*  ___ _ __| |_(_)___ _ _   *
            //* / _ \ '_ \  _| / _ \ ' \  *
            //* \___/ .__/\__|_\___/_||_| *
            //*     |_|                   *
            option::option () { all ().push_back (this); }
            option::option (num_occurrences_flag const occurrences)
                    : option () {
                occurrences_ = occurrences;
            }

            option::~option () = default;

            void option::set_num_occurrences_flag (num_occurrences_flag const n) {
                occurrences_ = n;
            }
            num_occurrences_flag option::get_num_occurrences_flag () const { return occurrences_; }
            unsigned option::get_num_occurrences () const { return num_occurrences_; }

            void option::set_description (std::string const & d) { description_ = d; }
            void option::set_positional () { positional_ = true; }
            void option::set_usage (std::string const & d) { usage_ = d; }
            bool option::is_positional () const { return positional_; }
            bool option::is_alias () const { return false; }

            alias * option::as_alias () { return nullptr; }
            alias const * option::as_alias () const { return nullptr; }

            std::string const & option::name () const { return name_; }
            void option::set_name (std::string const & name) {
                PSTORE_ASSERT ((name.empty () || name[0] != '-') && "Option can't start with '-");
                name_ = name;
            }
            std::string const & option::usage () const noexcept { return usage_; }
            std::string const & option::description () const noexcept { return description_; }

            bool option::add_occurrence () {
                ++num_occurrences_;
                return true;
            }

            bool option::is_satisfied () const {
                bool result = true;
                switch (this->get_num_occurrences_flag ()) {
                case num_occurrences_flag::required: result = num_occurrences_ >= 1U; break;
                case num_occurrences_flag::one_or_more: result = num_occurrences_ > 1U; break;
                case num_occurrences_flag::optional:
                case num_occurrences_flag::zero_or_more: break;
                }
                return result;
            }

            bool option::can_accept_another_occurrence () const {
                bool result = true;
                switch (this->get_num_occurrences_flag ()) {
                case num_occurrences_flag::optional:
                case num_occurrences_flag::required: result = num_occurrences_ == 0U; break;
                case num_occurrences_flag::zero_or_more:
                case num_occurrences_flag::one_or_more: break;
                }
                return result;
            }

            gsl::czstring option::arg_description () const noexcept { return nullptr; }

            option::options_container & option::all () {
                static options_container all_options;
                return all_options;
            }

            option::options_container & option::reset_container () {
                auto & a = option::all ();
                a.clear ();
                return a;
            }


            //*           _     _              _  *
            //*  ___ _ __| |_  | |__  ___  ___| | *
            //* / _ \ '_ \  _| | '_ \/ _ \/ _ \ | *
            //* \___/ .__/\__| |_.__/\___/\___/_| *
            //*     |_|                           *
            bool opt<bool>::value (std::string const &) { return false; }
            bool opt<bool>::add_occurrence () {
                option::add_occurrence ();
                if (this->get_num_occurrences () == 1U) {
                    value_ = !value_;
                }
                return true;
            }
            parser_base * opt<bool>::get_parser () { return nullptr; }

            //*       _ _          *
            //*  __ _| (_)__ _ ___ *
            //* / _` | | / _` (_-< *
            //* \__,_|_|_\__,_/__/ *
            //*                    *
            void alias::set_original (option * const o) {
                PSTORE_ASSERT (o != nullptr);
                original_ = o;
            }
            bool alias::add_occurrence () { return original_->add_occurrence (); }
            void alias::set_num_occurrences_flag (num_occurrences_flag const n) {
                original_->set_num_occurrences_flag (n);
            }
            num_occurrences_flag alias::get_num_occurrences_flag () const {
                return original_->get_num_occurrences_flag ();
            }
            void alias::set_positional () { original_->set_positional (); }
            bool alias::is_positional () const { return original_->is_positional (); }
            bool alias::is_alias () const { return true; }
            unsigned alias::get_num_occurrences () const {
                return original_->get_num_occurrences ();
            }
            parser_base * alias::get_parser () { return original_->get_parser (); }
            bool alias::takes_argument () const { return original_->takes_argument (); }
            bool alias::value (std::string const & v) { return original_->value (v); }

    }     // end namespace command_line
} // end namespace pstore
