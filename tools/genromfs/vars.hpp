//*                       *
//* __   ____ _ _ __ ___  *
//* \ \ / / _` | '__/ __| *
//*  \ V / (_| | |  \__ \ *
//*   \_/ \__,_|_|  |___/ *
//*                       *
//===- tools/genromfs/vars.hpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
