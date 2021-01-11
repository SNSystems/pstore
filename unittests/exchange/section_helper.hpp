//*                _   _               _          _                  *
//*  ___  ___  ___| |_(_) ___  _ __   | |__   ___| |_ __   ___ _ __  *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  | '_ \ / _ \ | '_ \ / _ \ '__| *
//* \__ \  __/ (__| |_| | (_) | | | | | | | |  __/ | |_) |  __/ |    *
//* |___/\___|\___|\__|_|\___/|_| |_| |_| |_|\___|_| .__/ \___|_|    *
//*                                                |_|               *
//===- unittests/exchange/section_helper.hpp ------------------------------===//
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
#ifndef PSTORE_UNITTESTS_EXCHANGE_SECTION_HELPER_HPP
#define PSTORE_UNITTESTS_EXCHANGE_SECTION_HELPER_HPP

#include "pstore/mcrepo/section.hpp"

/// placement_delete is paired with use of placement new and unique_ptr<> to enable unique_ptr<>
/// to be used on memory that is not owned. unique_ptr<> will cause the object's destructor to
/// be called when it goes out of scope, but the release of memory is a no-op.
template <typename T>
struct placement_delete {
    constexpr void operator() (T *) const noexcept {}
};

template <pstore::repo::section_kind Kind, typename Dispatcher>
decltype (auto)
dispatch_new_section (Dispatcher const & dispatcher,
                      pstore::gsl::not_null<std::vector<std::uint8_t> *> const buffer) {
    buffer->resize (dispatcher.size_bytes ());
    auto * const data = buffer->data ();
    PSTORE_ASSERT (data != nullptr);
    dispatcher.write (data);

    using section_type = typename pstore::repo::enum_to_section_t<Kind>;

    // Use unique_ptr<> to ensure that the section_type object's destructor is called. Since
    // the memory is not owned by this object (it belongs to 'buffer'), the delete operation
    // will do nothing thanks to placement_delete<>.
    return std::unique_ptr<section_type const, placement_delete<section_type const>>{
        reinterpret_cast<section_type const *> (data), placement_delete<section_type const>{}};
}



/// \tparam Kind The type of section to be built.
/// \tparam SectionType  The type used to store the properties of Kind section.
/// \tparam CreationDispatcher  The type of creation dispatcher used to instantiate sections of
///     type SectionType.
template <pstore::repo::section_kind Kind,
          typename SectionType = typename pstore::repo::enum_to_section_t<Kind>,
          typename CreationDispatcher =
              typename pstore::repo::section_to_creation_dispatcher<SectionType>::type>
decltype (auto) build_section (pstore::gsl::not_null<std::vector<std::uint8_t> *> const buffer,
                               pstore::repo::section_content const & content) {
    CreationDispatcher dispatcher{Kind, &content};
    return dispatch_new_section<Kind> (dispatcher, buffer);
}



template <pstore::repo::section_kind Kind>
std::string export_section (pstore::database const & db,
                            pstore::exchange::export_ns::name_mapping const & exported_names,
                            pstore::repo::section_content const & content, bool comments) {
    // First build the section that we want to export.
    std::vector<std::uint8_t> buffer;
    auto const section = build_section<Kind> (&buffer, content);

    // Now export it.
    std::ostringstream os;
    using namespace pstore::exchange::export_ns;
    emit_section<Kind> (os, indent{}, db, exported_names, *section, comments);
    return os.str ();
}

#endif // PSTORE_UNITTESTS_EXCHANGE_SECTION_HELPER_HPP
