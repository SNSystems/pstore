//*              _   _              *
//*   ___  _ __ | |_(_) ___  _ __   *
//*  / _ \| '_ \| __| |/ _ \| '_ \  *
//* | (_) | |_) | |_| | (_) | | | | *
//*  \___/| .__/ \__|_|\___/|_| |_| *
//*       |_|                       *
//===- lib/cmd_util/cl/option.cpp -----------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#include "pstore/cmd_util/cl/option.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {
            //*           _   _           *
            //*  ___ _ __| |_(_)___ _ _   *
            //* / _ \ '_ \  _| / _ \ ' \  *
            //* \___/ .__/\__|_\___/_||_| *
            //*     |_|                   *
            option::option () { all ().push_back (this); }

            option::~option () {}

            void option::set_num_occurrences_flag (num_occurrences_flag n) { occurrences_ = n; }
            num_occurrences_flag option::get_num_occurrences_flag () const { return occurrences_; }
            unsigned option::getNumOccurrences () const { return num_occurrences_; }

            void option::set_description (std::string const & d) { description_ = d; }
            void option::set_positional () { positional_ = true; }
            bool option::is_positional () const { return positional_; }
            bool option::is_alias () const { return false; }

            std::string const & option::name () const { return name_; }
            void option::set_name (std::string const & name) {
                assert ((name.empty () || name[0] != '-') && "Option can't start with '-");
                name_ = name;
            }
            std::string const & option::description () const { return description_; }

            void option::add_occurrence () { ++num_occurrences_; }
            bool option::is_satisfied () const {
                bool result = true;
                switch (this->get_num_occurrences_flag ()) {
                case num_occurrences_flag::required: result = num_occurrences_ >= 1U; break;
                case num_occurrences_flag::one_or_more: result = num_occurrences_ > 1U; break;
                case num_occurrences_flag::optional:
                case num_occurrences_flag::zero_or_more: break; ;
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
            void opt<bool>::add_occurrence () {
                option::add_occurrence ();
                if (this->getNumOccurrences () == 1U) {
                    value_ = !value_;
                }
            }
            parser_base * opt<bool>::get_parser () { return nullptr; }

            //*       _ _          *
            //*  __ _| (_)__ _ ___ *
            //* / _` | | / _` (_-< *
            //* \__,_|_|_\__,_/__/ *
            //*                    *
            void alias::set_original (option * o) {
                assert (o != nullptr);
                original_ = o;
            }
            void alias::set_num_occurrences_flag (num_occurrences_flag n) {
                original_->set_num_occurrences_flag (n);
            }
            num_occurrences_flag alias::get_num_occurrences_flag () const {
                return original_->get_num_occurrences_flag ();
            }
            void alias::set_positional () { original_->set_positional (); }
            bool alias::is_positional () const { return original_->is_positional (); }
            bool alias::is_alias () const { return true; }
            unsigned alias::getNumOccurrences () const { return original_->getNumOccurrences (); }
            parser_base * alias::get_parser () { return original_->get_parser (); }
            bool alias::takes_argument () const { return original_->takes_argument (); }
            bool alias::value (std::string const & v) { return original_->value (v); }
        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore
