//*                        _                                             *
//* __      _____  _ __ __| | __      ___ __ __ _ _ __  _ __   ___ _ __  *
//* \ \ /\ / / _ \| '__/ _` | \ \ /\ / / '__/ _` | '_ \| '_ \ / _ \ '__| *
//*  \ V  V / (_) | | | (_| |  \ V  V /| | | (_| | |_) | |_) |  __/ |    *
//*   \_/\_/ \___/|_|  \__,_|   \_/\_/ |_|  \__,_| .__/| .__/ \___|_|    *
//*                                              |_|   |_|               *
//===- include/pstore/command_line/word_wrapper.hpp -----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_WORD_WRAPPER_HPP
#define PSTORE_COMMAND_LINE_WORD_WRAPPER_HPP

#include <cstdlib>
#include <iterator>
#include <string>

namespace pstore {
    namespace command_line {

        class word_wrapper {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::string const;
            using difference_type = std::ptrdiff_t;
            using pointer = std::string const *;
            using reference = std::string const &;

            static constexpr std::size_t default_width = 79;

            explicit word_wrapper (std::string const & text,
                                   std::size_t const max_width = default_width)
                    : word_wrapper (text, max_width, 0U) {}
            static word_wrapper end (std::string const & text,
                                     std::size_t max_width = default_width);

            bool operator== (word_wrapper const & rhs) const;
            bool operator!= (word_wrapper const & rhs) const { return !operator== (rhs); }

            reference operator* () const { return substr_; }
            pointer operator-> () const { return &substr_; }

            word_wrapper & operator++ ();
            word_wrapper operator++ (int);

        private:
            word_wrapper (std::string const & text, std::size_t max_width, std::size_t pos);
            void next ();

            std::string const & text_;
            std::size_t const max_width_;
            std::size_t start_pos_;
            std::string substr_;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_WORD_WRAPPER_HPP
