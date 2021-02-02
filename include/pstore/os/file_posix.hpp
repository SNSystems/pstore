//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- include/pstore/os/file_posix.hpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file file_posix.hpp
/// \brief Posix-specific implementations of file APIs.

#ifndef PSTORE_OS_FILE_POSIX_HPP
#define PSTORE_OS_FILE_POSIX_HPP

#if !defined(_WIN32)

#    include <unistd.h>

namespace pstore {
    namespace file {

        /// \brief A namespace to hold POSIX-specific file interfaces.
        namespace posix {
            class deleter final : public deleter_base {
            public:
                explicit deleter (std::string const & path)
                        : deleter_base (path, &platform_unlink) {}
                // No copying, moving, or assignment
                deleter (deleter const &) = delete;
                deleter (deleter &&) noexcept = delete;
                deleter & operator= (deleter const &) = delete;
                deleter & operator= (deleter &&) noexcept = delete;

                ~deleter () noexcept override;

            private:
                /// The platform-specific file deletion function. file_deleter_base will
                /// call this function when it wants to delete a file.
                /// \param path The UTF-8 encoded path to the file to be deleted.

                static void platform_unlink (std::string const & path);
            };
        } // namespace posix

        /// \brief The cross-platform name for the deleter class.
        /// This should always be preferred to the platform-specific variation.
        using deleter = posix::deleter;

    } // namespace file
} // namespace pstore
#endif //! defined (_WIN32)

#endif // PSTORE_OS_FILE_POSIX_HPP
