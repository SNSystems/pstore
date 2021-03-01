//===- include/pstore/os/wsa_startup.hpp ------------------*- mode: C++ -*-===//
//*                           _             _                *
//* __      _____  __ _   ___| |_ __ _ _ __| |_ _   _ _ __   *
//* \ \ /\ / / __|/ _` | / __| __/ _` | '__| __| | | | '_ \  *
//*  \ V  V /\__ \ (_| | \__ \ || (_| | |  | |_| |_| | |_) | *
//*   \_/\_/ |___/\__,_| |___/\__\__,_|_|   \__|\__,_| .__/  *
//*                                                  |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_OS_WSA_STARTUP_HPP
#define PSTORE_OS_WSA_STARTUP_HPP


namespace pstore {

#ifdef _WIN32

    class wsa_startup {
    public:
        wsa_startup () noexcept
                : started_{start ()} {}
        // no copy or assignment
        wsa_startup (wsa_startup const &) = delete;
        wsa_startup (wsa_startup &&) noexcept = delete;

        ~wsa_startup () noexcept;

        wsa_startup & operator= (wsa_startup const &) = delete;
        wsa_startup & operator= (wsa_startup &&) = delete;

        bool started () const noexcept { return started_; }

    private:
        static bool start () noexcept;
        bool started_;
    };

#else // _WIN32

    class wsa_startup {
    public:
        wsa_startup () noexcept = default;
        wsa_startup (wsa_startup const &) = delete;
        wsa_startup (wsa_startup &&) noexcept = delete;

        ~wsa_startup () noexcept = default;

        wsa_startup & operator= (wsa_startup const &) = delete;
        wsa_startup & operator= (wsa_startup &&) noexcept = delete;

        constexpr bool started () const noexcept { return true; }
    };

#endif
} // end namespace pstore


#endif // PSTORE_OS_WSA_STARTUP_HPP
