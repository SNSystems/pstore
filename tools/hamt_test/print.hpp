//===- tools/hamt_test/print.hpp --------------------------*- mode: C++ -*-===//
//*             _       _    *
//*  _ __  _ __(_)_ __ | |_  *
//* | '_ \| '__| | '_ \| __| *
//* | |_) | |  | | | | | |_  *
//* | .__/|_|  |_|_| |_|\__| *
//* |_|                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file print.hpp

#ifndef PRINT_HPP
#define PRINT_HPP

#include <iostream>
#include <mutex>

#include "pstore/core/database.hpp"

namespace details {

    class ios_printer {
    public:
        explicit constexpr ios_printer (std::ostream & os) noexcept
                : os_{os} {}
        ios_printer (ios_printer const &) = delete;
        ios_printer (ios_printer &&) noexcept = delete;

        ~ios_printer () noexcept = default;

        ios_printer & operator= (ios_printer const &) = delete;
        ios_printer & operator= (ios_printer &&) noexcept = delete;

        /// Writes one or more values to the output stream followed by a newline. If the preceding
        /// operation was a print_flush() then the output will also be prefixed by newline.
        template <typename... Args>
        std::ostream & print (Args &&... args);

        /// Writes one or more values to the output stream and flushes the stream.
        template <typename... Args>
        std::ostream & print_flush (Args &&... args);

    private:
        std::ostream & print_one () noexcept { return os_; }

        template <typename A0, typename... Args>
        std::ostream & print_one (A0 && a0, Args &&... args);

        std::ostream & os_;
        std::mutex mutex_;
        bool cr_ = false;
    };

    template <typename... Args>
    std::ostream & ios_printer::print (Args &&... args) {
        std::lock_guard<std::mutex> _{mutex_};
        if (!cr_) {
            cr_ = true;
            this->print_one ('\n');
        }
        return this->print_one (std::forward<Args> (args)...) << '\n';
    }

    template <typename... Args>
    std::ostream & ios_printer::print_flush (Args &&... args) {
        std::lock_guard<std::mutex> _{mutex_};
        cr_ = false;
        return print_one (std::forward<Args> (args)...) << std::flush;
    }

    template <typename A0, typename... Args>
    std::ostream & ios_printer::print_one (A0 && a0, Args &&... args) {
        os_ << a0;
        return print_one (std::forward<Args> (args)...);
    }

    extern ios_printer cout;
    extern ios_printer cerr;

} // end namespace details


// print cout/flush
// ~~~~~~~~~~~~~~~~
template <typename... Args>
std::ostream & print_cout (Args &&... args) {
    return details::cout.print (std::forward<Args> (args)...);
}
template <typename... Args>
std::ostream & print_cout_flush (Args &&... args) {
    return details::cout.print_flush (std::forward<Args> (args)...);
}


// print cerr/flush
// ~~~~~~~~~~~~~~~~
template <typename... Args>
std::ostream & print_cerr (Args &&... args) {
    return details::cerr.print (std::forward<Args> (args)...);
}
template <typename... Args>
std::ostream & print_cerr_flush (Args &&... args) {
    return details::cerr.print_flush (std::forward<Args> (args)...);
}

#endif // PRINT_HPP
