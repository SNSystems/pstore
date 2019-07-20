//*                       *
//* __   ____ _ _ __ ___  *
//* \ \ / / _` | '__/ __| *
//*  \ V / (_| | |  \__ \ *
//*   \_/ \__,_|_|  |___/ *
//*                       *
//===- tools/genromfs/vars.hpp --------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_GENROMFS_VARS_HPP
#define PSTORE_GENROMFS_VARS_HPP

#include <iosfwd>
#include <string>

template <typename NamePolicy>
class variable_name {
public:
    explicit constexpr variable_name (unsigned id, NamePolicy policy = NamePolicy ()) noexcept
            : id_ (id)
            , policy_ (policy) {}
    variable_name (variable_name const &) noexcept = default;
    variable_name (variable_name &&) noexcept = default;

    ~variable_name () noexcept = default;

    variable_name & operator= (variable_name const &) noexcept = default;
    variable_name & operator= (variable_name &&) noexcept = default;

    std::string as_string () const;
    std::ostream & write (std::ostream & os) const;

private:
    unsigned const id_;
    NamePolicy const policy_;
};

template <typename NamePolicy>
std::string variable_name<NamePolicy>::as_string () const {
    return policy_.name () + std::to_string (id_);
}

template <typename NamePolicy>
std::ostream & variable_name<NamePolicy>::write (std::ostream & os) const {
    return os << policy_.name () << id_;
}

template <typename NamePolicy>
std::ostream & operator<< (std::ostream & os, variable_name<NamePolicy> const & vn) {
    return vn.write (os);
}

class directory_var_policy {
public:
    static std::string const & name () noexcept { return name_; }

private:
    static std::string const name_;
};

class file_var_policy {
public:
    static std::string const & name () noexcept { return name_; }

private:
    static std::string const name_;
};

using directory_var = variable_name<directory_var_policy>;
using file_var = variable_name<file_var_policy>;

#endif // PSTORE_GENROMFS_VARS_HPP
