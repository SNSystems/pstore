//*  _       _                  _             _ _     _    *
//* (_)_ __ | |_ _ __ _   _ ___(_)_   _____  | (_)___| |_  *
//* | | '_ \| __| '__| | | / __| \ \ / / _ \ | | / __| __| *
//* | | | | | |_| |  | |_| \__ \ |\ V /  __/ | | \__ \ |_  *
//* |_|_| |_|\__|_|   \__,_|___/_| \_/ \___| |_|_|___/\__| *
//*                                                        *
//===- include/pstore/broker/intrusive_list.hpp ---------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_BROKER_INTRUSIVE_LIST_HPP
#define PSTORE_BROKER_INTRUSIVE_LIST_HPP

#include <iterator>
#include <memory>

#include "pstore/support/assert.hpp"

namespace pstore {
    namespace broker {

        template <typename T>
        struct list_member {
            T * prev = nullptr;
            T * next = nullptr;
        };

        template <typename T>
        class intrusive_list {
        public:
            intrusive_list ();
            ~intrusive_list () noexcept { this->check (); }

            /// The list does not own the pointers.
            class iterator {
            public:
                using iterator_category = std::bidirectional_iterator_tag;
                using value_type = T;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type *;
                using reference = value_type &;

                constexpr iterator () noexcept = default;
                explicit constexpr iterator (T * ptr) noexcept
                        : ptr_{ptr} {}

                bool operator== (iterator const & rhs) const noexcept { return ptr_ == rhs.ptr_; }
                bool operator!= (iterator const & rhs) const noexcept { return !operator== (rhs); }

                iterator & operator++ () noexcept {
                    ptr_ = ptr_->get_list_member ().next;
                    return *this;
                }
                iterator operator++ (int) noexcept {
                    auto t = *this;
                    ++(*this);
                    return t;
                }
                iterator & operator-- () noexcept {
                    ptr_ = ptr_->get_list_member ().prev;
                    return *this;
                }
                iterator operator-- (int) noexcept {
                    auto t = *this;
                    --(*this);
                    return t;
                }

                T & operator* () { return *ptr_; }
                T * operator-> () { return ptr_; }

            private:
                T * ptr_ = nullptr;
            };

            iterator begin () const { return iterator (head_->get_list_member ().next); }
            iterator end () const { return iterator (tail_.get ()); }

            void insert_before (T * element, T * before) noexcept;
            static void erase (T * element) noexcept;
            std::size_t size () const;

            void check () noexcept;
            T * tail () const noexcept { return tail_.get (); }

        private:
            std::unique_ptr<T> head_;
            std::unique_ptr<T> tail_;
        };

        // (ctor)
        // ~~~~~~
        template <typename T>
        intrusive_list<T>::intrusive_list ()
                : head_{std::unique_ptr<T> (new T)}
                , tail_{std::unique_ptr<T> (new T)} {
            head_->get_list_member ().next = tail_.get ();
            tail_->get_list_member ().prev = head_.get ();
        }

        template <typename T>
        std::size_t intrusive_list<T>::size () const {
            return static_cast<std::size_t> (std::distance (this->begin (), this->end ()));
        }

        // check
        // ~~~~~
        template <typename T>
        void intrusive_list<T>::check () noexcept {
#ifndef NDEBUG
            PSTORE_ASSERT (!head_->get_list_member ().prev);
            T * prev = nullptr;
            for (auto p = head_.get (); p; prev = p, p = p->get_list_member ().next) {
                PSTORE_ASSERT (p->get_list_member ().prev == prev);
            }
            PSTORE_ASSERT (prev == tail_.get ());
#endif
        }

        // insert_before
        // ~~~~~~~~~~~~~
        template <typename T>
        void intrusive_list<T>::insert_before (T * element, T * before) noexcept {
            auto & element_member = element->get_list_member ();
            auto & before_member = before->get_list_member ();

            element_member.prev = before_member.prev;
            before_member.prev->get_list_member ().next = element;
            before_member.prev = element;
            element_member.next = before;

            PSTORE_ASSERT (element_member.next->get_list_member ().prev == element);
            PSTORE_ASSERT (element_member.prev->get_list_member ().next == element);
            this->check ();
        }

        // erase
        // ~~~~~
        template <typename T>
        void intrusive_list<T>::erase (T * element) noexcept {
            auto & links = element->get_list_member ();
            if (links.prev) {
                links.prev->get_list_member ().next = links.next;
            }
            if (links.next) {
                links.next->get_list_member ().prev = links.prev;
            }
        }

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_INTRUSIVE_LIST_HPP
