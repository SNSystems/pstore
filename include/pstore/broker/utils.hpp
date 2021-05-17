//===- include/pstore/broker/utils.hpp -------------------*- mode: C++ -*-===//
//*        _   _ _      *
//*       | | (_) |     *
//*  _   _| |_ _| |___  *
//* | | | | __| | / __| *
//* | |_| | |_| | \__ \ *
//*  \__,_|\__|_|_|___/ *
//*                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_BROKER_UTILS_HPP
#define PSTORE_BROKER_UTILS_HPP

namespace pstore {
    namespace broker {
        // create thread
        // ~~~~~~~~~~~~~
        template <class Function, class... Args>
        auto create_thread (Function && f, Args &&... args)
            -> std::future<typename std::result_of<Function (Args...)>::type> {

            using return_type = typename std::result_of<Function (Args...)>::type;

            std::packaged_task<return_type (Args...)> task (f);
            auto fut = task.get_future ();

            std::thread thr (std::move (task), std::forward<Args> (args)...);
            thr.detach ();

            return fut;
        }

        // thread init
        // ~~~~~~~~~~~
        void thread_init (std::string const & name) {
            threads::set_name (name.c_str ());
            create_log_stream ("broker." + name);
        }

        // create http worker thread
        // ~~~~~~~~~~~~~~~~~~~~~~~~~
        void
        create_http_worker_thread (gsl::not_null<std::vector<std::future<void>> *> const futures,
                                   gsl::not_null<maybe<http::server_status> *> http_status,
                                   bool announce_port, romfs::romfs & fs) {
            if (!http_status->has_value ()) {
                return;
            }
            futures->push_back (create_thread (
                [announce_port, &fs] (maybe<http::server_status> * const status) {
                    thread_init ("http");
                    PSTORE_ASSERT (status->has_value ());

                    http::channel_container channels{
                        {"commits", http::channel_container_entry{&broker::commits_channel,
                                                                  &broker::commits_cv}},
                        {"uptime", http::channel_container_entry{&broker::uptime_channel,
                                                                 &broker::uptime_cv}},
                    };

                    http::server (
                        fs, &status->value (), channels, [announce_port] (in_port_t const port) {
                            if (announce_port) {
                                std::lock_guard<decltype (broker::iomut)> lock{broker::iomut};
                                std::cout << "HTTP listening on port " << port << std::endl;
                            }
                        });
                },
                http_status.get ()));
        }

        // create worker threads
        // ~~~~~~~~~~~~~~~~~~~~~
        std::vector<std::future<void>>
        create_worker_threads (std::shared_ptr<broker::command_processor> const & commands,
                               brokerface::fifo_path & fifo,
                               std::shared_ptr<broker::scavenger> const & scav,
                               std::atomic<bool> * const uptime_done) {

            std::vector<std::future<void>> futures;

            futures.push_back (create_thread ([&fifo, commands] () {
                thread_init ("command");
                commands->thread_entry (fifo);
            }));

            futures.push_back (create_thread ([scav] () {
                thread_init ("scavenger");
                scav->thread_entry ();
            }));

            futures.push_back (create_thread ([] () {
                thread_init ("gcwatch");
                broker::gc_process_watch_thread ();
            }));

            futures.push_back (create_thread (
                [] (std::atomic<bool> * const done) {
                    thread_init ("uptime");
                    broker::uptime (done);
                },
                uptime_done));

            return futures;
        }

        // make weak
        // ~~~~~~~~~
        /// Creates a weak_ptr from a shared_ptr. This can be done implicitly, but I want to make
        /// the conversion explicit.
        template <typename T>
        std::weak_ptr<T> make_weak (std::shared_ptr<T> const & p) {
            return {p};
        }

        // get http server status
        // ~~~~~~~~~~~~~~~~~~~~~~
        /// Create an HTTP server_status object which reflects the user's choice of port.
        decltype (auto) get_http_server_status (maybe<in_port_t> const & port) {
            if (port.has_value ()) {
                return maybe<http::server_status>{in_place, *port};
            }
            return nothing<http::server_status> ();
        }
    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_UTILS_HPP
