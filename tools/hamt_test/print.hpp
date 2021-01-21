//*             _       _    *
//*  _ __  _ __(_)_ __ | |_  *
//* | '_ \| '__| | '_ \| __| *
//* | |_) | |  | | | | | |_  *
//* | .__/|_|  |_|_| |_|\__| *
//* |_|                      *
//===- tools/hamt_test/print.hpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

/// \file print.hpp

#ifndef PRINT_HPP
#define PRINT_HPP

#include <atomic>
#include <iostream>
#include <mutex>

#include "pstore/core/database.hpp"

namespace details {
    class ios_printer {
    public:
        explicit ios_printer (std::ostream & os)
                : os_ (os) {}
        ios_printer (ios_printer const &) = delete;
        ios_printer & operator= (ios_printer const &) = delete;

        /// Writes one or more values to the output stream followed by a newline. If the preceding
        /// operation was a print_flush() then the output will also be prefixed by newline.
        template <class... Args>
        std::ostream & print (Args const &... args);

        /// Writes one or more values to the output stream and flushes the stream.
        template <class... Args>
        std::ostream & print_flush (Args const &... args);

    private:
        std::ostream & print_one () { return os_; }

        template <class A0, class... Args>
        std::ostream & print_one (A0 const & a0, Args const &... args);

        std::ostream & os_;
        std::mutex mutex_;
        std::atomic<bool> cr_{false};
    };

    template <class... Args>
    std::ostream & ios_printer::print (Args const &... args) {
        std::lock_guard<std::mutex> guard (mutex_);
        if (!cr_.exchange (true)) {
            this->print_one ('\n');
        }
        return this->print_one (args...) << '\n';
    }

    template <class... Args>
    std::ostream & ios_printer::print_flush (Args const &... args) {
        std::lock_guard<std::mutex> guard (mutex_);
        cr_ = false;
        return print_one (args...) << std::flush;
    }

    template <class A0, class... Args>
    std::ostream & ios_printer::print_one (A0 const & a0, Args const &... args) {
        os_ << a0;
        return print_one (args...);
    }

    extern ios_printer cout;
    extern ios_printer cerr;
} // namespace details


// print_cout/flush
// ~~~~~~~~~~~~~~~~
template <class... Args>
std::ostream & print_cout (Args const &... args) {
    return ::details::cout.print (args...);
}
template <class... Args>
std::ostream & print_cout_flush (Args const &... args) {
    return ::details::cout.print_flush (args...);
}


// print_cerr/flush
// ~~~~~~~~~~~~~~~~
template <class... Args>
std::ostream & print_cerr (Args const &... args) {
    return ::details::cerr.print (args...);
}
template <class... Args>
std::ostream & print_cerr_flush (Args const &... args) {
    return ::details::cerr.print_flush (args...);
}

#endif // PRINT_HPP
