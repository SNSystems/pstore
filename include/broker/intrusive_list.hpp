//*  _       _                  _             _ _     _    *
//* (_)_ __ | |_ _ __ _   _ ___(_)_   _____  | (_)___| |_  *
//* | | '_ \| __| '__| | | / __| \ \ / / _ \ | | / __| __| *
//* | | | | | |_| |  | |_| \__ \ |\ V /  __/ | | \__ \ |_  *
//* |_|_| |_|\__|_|   \__,_|___/_| \_/ \___| |_|_|___/\__| *
//*                                                        *
//===- include/broker/intrusive_list.hpp ----------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_BROKER_INTRUSIVE_LIST_HPP
#define PSTORE_BROKER_INTRUSIVE_LIST_HPP (1)

#include <cassert>
#include <iterator>
#include <memory>

template <typename T>
struct list_member {
    T * prev = nullptr;
    T * next = nullptr;
};

template <typename T>
struct intrusive_list {
    intrusive_list ();
    ~intrusive_list () noexcept {
        this->check ();
    }

    /// The list does not own the pointers.
    class iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
    public:
        iterator () = default;
        explicit iterator (T * ptr)
                : ptr_{ptr} {}
        iterator & operator= (iterator const & rhs) = default;

        bool operator== (iterator const & rhs) const noexcept {
            return ptr_ == rhs.ptr_;
        }
        bool operator!= (iterator const & rhs) const noexcept {
            return !operator== (rhs);
        }

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

        T & operator* () {
            return *ptr_;
        }
        T * operator-> () {
            return ptr_;
        }

    private:
        T * ptr_ = nullptr;
    };

    iterator begin () const {
        return iterator (head_->get_list_member ().next);
    }
    iterator end () const {
        return iterator (tail_.get ());
    }

    void insert_before (T * element, T * before);
    static void erase (T * element) noexcept;
    std::size_t size () const;

    void check () noexcept;
    T * tail () const {
        return tail_.get ();
    }

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
    assert (!head_->get_list_member ().prev);
    T * prev = nullptr;
    for (auto p = head_.get (); p; prev = p, p = p->get_list_member ().next) {
        assert (p->get_list_member ().prev == prev);
    }
    assert (prev == tail_.get ());
#endif
}

// insert_before
// ~~~~~~~~~~~~~
template <typename T>
void intrusive_list<T>::insert_before (T * element, T * before) {
    auto & element_member = element->get_list_member ();
    auto & before_member = before->get_list_member ();

    element_member.prev = before_member.prev;
    before_member.prev->get_list_member ().next = element;
    before_member.prev = element;
    element_member.next = before;

    assert (element_member.next->get_list_member ().prev == element);
    assert (element_member.prev->get_list_member ().next == element);
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

#endif // PSTORE_BROKER_INTRUSIVE_LIST_HPP
// eof: include/broker/intrusive_list.hpp
