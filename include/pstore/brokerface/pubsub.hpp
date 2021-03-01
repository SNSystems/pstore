//===- include/pstore/brokerface/pubsub.hpp ---------------*- mode: C++ -*-===//
//*              _               _      *
//*  _ __  _   _| |__  ___ _   _| |__   *
//* | '_ \| | | | '_ \/ __| | | | '_ \  *
//* | |_) | |_| | |_) \__ \ |_| | |_) | *
//* | .__/ \__,_|_.__/|___/\__,_|_.__/  *
//* |_|                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file pubsub.hpp
/// \brief A simple publish-and-subscribe mechanism.
///
/// This module provides a means for one part of a program to "publish" information to which other
/// parts can subscribe. There can be multiple "channels" of information representing different
/// groups of data.
#ifndef PSTORE_BROKERFACE_PUBSUB_HPP
#define PSTORE_BROKERFACE_PUBSUB_HPP

#include <mutex>
#include <queue>
#include <unordered_set>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace brokerface {

        template <typename ConditionVariable>
        class channel;

        //*          _               _ _              *
        //*  ____  _| |__ ___ __ _ _(_) |__  ___ _ _  *
        //* (_-< || | '_ (_-</ _| '_| | '_ \/ -_) '_| *
        //* /__/\_,_|_.__/__/\__|_| |_|_.__/\___|_|   *
        //*                                           *
        /// An instance of the subscriber class represents a subscription to messages published on
        /// an associated owning channel.
        template <typename ConditionVariable>
        class subscriber {
            friend class channel<ConditionVariable>;

        public:
            ~subscriber () noexcept;

            // No copying or assignment.
            subscriber (subscriber const &) = delete;
            subscriber (subscriber &&) = delete;
            subscriber & operator= (subscriber const &) = delete;
            subscriber & operator= (subscriber &&) = delete;

            /// Blocks waiting for a message to be published on the owning channel of for the
            /// subscription to be cancelled.
            ///
            /// \returns A maybe holding a message published to the owning channel or with no value
            /// indicating that the subscription has been cancelled.
            maybe<std::string> listen ();

            /// Cancels a subscription.
            ///
            /// The subscription is marked as inactive. If waiting it is woken up.
            void cancel ();

            /// \returns A reference to the owning channel.
            channel<ConditionVariable> & owner () noexcept { return *owner_; }
            /// \returns A reference to the owning channel.
            channel<ConditionVariable> const & owner () const noexcept { return *owner_; }

            /// Removes a single message from the subscription queue if available.
            maybe<std::string> pop ();

        private:
            explicit subscriber (gsl::not_null<channel<ConditionVariable> *> c) noexcept
                    : owner_{c} {}

            /// The queue of published messages waiting to be delivered to a listening subscriber.
            ///
            /// \note This is a queue of strings. If there are multiple subscribers to this channel
            /// then the strings will be duplicated in each which could be very inefficient. An
            /// alternative would be to store shared_ptr<string>. For the moment I'm leaving it like
            /// this on the assumption that there will typically be just a single subscriber.
            std::queue<std::string> queue_;

            /// The channel with which this subscription is associated.
            channel<ConditionVariable> * const owner_;

            /// Should this subscriber continue to listen to messages?
            bool active_ = true;
        };

        //*     _                       _  *
        //*  __| |_  __ _ _ _  _ _  ___| | *
        //* / _| ' \/ _` | ' \| ' \/ -_) | *
        //* \__|_||_\__,_|_||_|_||_\___|_| *
        //*                                *
        /// Messages can be written ("published") to a channel; there can be multiple "subscribers"
        /// which will all receive notification of published messages.
        template <typename ConditionVariable>
        class channel {
            friend class subscriber<ConditionVariable>;

        public:
            using subscriber_type = subscriber<ConditionVariable>;
            using subscriber_pointer = std::unique_ptr<subscriber_type>;

            explicit channel (gsl::not_null<ConditionVariable *> cv);
            channel (channel const &) = delete;
            channel (channel &&) = delete;

            ~channel () noexcept;

            channel & operator= (channel const &) = delete;
            channel & operator= (channel &&) = delete;

            /// Broadcasts a message to all subscribers.
            ///
            /// \param message  The message to be published.
            void publish (std::string const & message);
            void publish (gsl::czstring message);

            /// \brief Broadcasts a message to all subscribers.
            ///
            /// The string to be published is the result of a function (which is passed as \p f,
            /// along with its arguments, \p args). The function will only be called if there are
            /// subscribers. This can be used to avoid unnecessarily creating the published string
            /// if it potentially expensive to do so.
            ///
            /// \param f  A function whose signature is compatible with `std::function<std::string
            /// (Args...)>` which will be called if the channel has one or more subscribers. It
            /// should produce the message to be sent.
            ///
            /// \param args  Arguments that will be passed to \p f.
            template <typename MessageFunction, typename... Args>
            void publish (MessageFunction f, Args &&... args);

            /// Creates a new subscriber instance and attaches it to this channel.
            subscriber_pointer new_subscriber ();

        private:
            maybe<std::string> listen (subscriber_type & sub);

            /// Cancels a subscription.
            ///
            /// The subscription is marked as inactive. If waiting it is woken up.
            void cancel (subscriber_type & sub) const;

            /// Called when a subscriber is destructed to remove it from the subscribers list.
            void remove (subscriber_type * sub) noexcept;

            /// Is anyone subscribed to this channel?
            bool have_listeners () const;

            mutable std::mutex mut_;
            mutable gsl::not_null<ConditionVariable *> cv_;

            /// All of the subscribers to this channel.
            std::unordered_set<subscriber_type *> subscribers_;
        };


        //*          _               _ _              *
        //*  ____  _| |__ ___ __ _ _(_) |__  ___ _ _  *
        //* (_-< || | '_ (_-</ _| '_| | '_ \/ -_) '_| *
        //* /__/\_,_|_.__/__/\__|_| |_|_.__/\___|_|   *
        //*                                           *

        // (dtor)
        // ~~~~~~
        template <typename ConditionVariable>
        subscriber<ConditionVariable>::~subscriber () noexcept {
            owner_->remove (this);
        }

        // cancel
        // ~~~~~~
        template <typename ConditionVariable>
        inline void subscriber<ConditionVariable>::cancel () {
            this->owner ().cancel (*this);
        }

        // listen
        // ~~~~~~
        template <typename ConditionVariable>
        inline maybe<std::string> subscriber<ConditionVariable>::listen () {
            return owner_->listen (*this);
        }

        // pop
        // ~~~
        template <typename ConditionVariable>
        maybe<std::string> subscriber<ConditionVariable>::pop () {
            std::unique_lock<std::mutex> const lock{this->owner ().mut_};
            if (queue_.empty ()) {
                return {};
            }
            std::string const message = std::move (queue_.front ());
            queue_.pop ();
            return just (message);
        }

        //*     _                       _  *
        //*  __| |_  __ _ _ _  _ _  ___| | *
        //* / _| ' \/ _` | ' \| ' \/ -_) | *
        //* \__|_||_\__,_|_||_|_||_\___|_| *
        //*                                *

        // (ctor)
        // ~~~~~~
        template <typename ConditionVariable>
        channel<ConditionVariable>::channel (gsl::not_null<ConditionVariable *> cv)
                : cv_{cv} {}

        // (dtor)
        // ~~~~~~
        template <typename ConditionVariable>
        channel<ConditionVariable>::~channel () noexcept {
            // Check that this channel has no subscribers.
            PSTORE_ASSERT (subscribers_.empty ());
        }

        // publish
        // ~~~~~~~
        template <typename ConditionVariable>
        void channel<ConditionVariable>::publish (std::string const & message) {
            this->publish ([&message] () { return message; });
        }
        template <typename ConditionVariable>
        void channel<ConditionVariable>::publish (gsl::czstring message) {
            this->publish ([&message] () { return std::string{message}; });
        }

        template <typename ConditionVariable>
        template <typename MessageFunction, typename... Args>
        void channel<ConditionVariable>::publish (MessageFunction f, Args &&... args) {
            if (this->have_listeners ()) {
                // Note that f() is called without the lock held.
                std::string const message = f (std::forward<Args> (args)...);

                std::lock_guard<std::mutex> const lock{mut_};
                for (auto & sub : subscribers_) {
                    sub->queue_.push (message);
                }
                cv_->notify_all ();
            }
        }

        // have listeners
        // ~~~~~~~~~~~~~~
        template <typename ConditionVariable>
        bool channel<ConditionVariable>::have_listeners () const {
            std::lock_guard<std::mutex> const lock{mut_};
            return !subscribers_.empty ();
        }

        // cancel
        // ~~~~~~
        template <typename ConditionVariable>
        void channel<ConditionVariable>::cancel (subscriber_type & sub) const {
            if (&sub.owner () == this) {
                std::unique_lock<std::mutex> const lock{mut_};
                sub.active_ = false;
                cv_->notify_all ();
            }
        }

        // listen
        // ~~~~~~
        template <typename ConditionVariable>
        maybe<std::string> channel<ConditionVariable>::listen (subscriber_type & sub) {
            std::unique_lock<std::mutex> lock{mut_};
            while (sub.active_) {
                if (sub.queue_.size () == 0) {
                    cv_->wait (lock);
                } else {
                    std::string const message = std::move (sub.queue_.front ());
                    sub.queue_.pop ();
                    return just (message);
                }
            }
            return nothing<std::string> ();
        }

        // new subscriber
        // ~~~~~~~~~~~~~~
        template <typename ConditionVariable>
        auto channel<ConditionVariable>::new_subscriber () -> subscriber_pointer {
            std::lock_guard<std::mutex> const lock{mut_};
            auto resl = subscriber_pointer{new subscriber_type (this)};
            subscribers_.insert (resl.get ());
            return resl;
        }

        // remove
        // ~~~~~~
        template <typename ConditionVariable>
        void channel<ConditionVariable>::remove (subscriber_type * sub) noexcept {
            std::lock_guard<std::mutex> const lock{mut_};
            PSTORE_ASSERT (subscribers_.find (sub) != subscribers_.end ());
            subscribers_.erase (sub);
        }

    } // end namespace brokerface
} // end namespace pstore

#endif // PSTORE_BROKERFACE_PUBSUB_HPP
