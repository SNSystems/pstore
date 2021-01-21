//*                     _   _                    *
//*  _ __ ___  __ _  __| | | | ___   ___  _ __   *
//* | '__/ _ \/ _` |/ _` | | |/ _ \ / _ \| '_ \  *
//* | | |  __/ (_| | (_| | | | (_) | (_) | |_) | *
//* |_|  \___|\__,_|\__,_| |_|\___/ \___/| .__/  *
//*                                      |_|     *
//===- lib/broker/read_loop_win32.cpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file read_loop_win32.cpp
/// \brief The read loop thread entry point for Windows.

#include "pstore/broker/read_loop.hpp"

#ifdef _WIN32

// Standard includes
#    include <algorithm>
#    include <array>
#    include <cstdio>
#    include <cstdlib>
#    include <iterator>
#    include <memory>
#    include <sstream>
#    include <utility>

// Platform includes
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

// pstore includes
#    include "pstore/broker/command.hpp"
#    include "pstore/broker/globals.hpp"
#    include "pstore/broker/intrusive_list.hpp"
#    include "pstore/broker/message_pool.hpp"
#    include "pstore/broker/quit.hpp"
#    include "pstore/broker/recorder.hpp"
#    include "pstore/brokerface/fifo_path.hpp"
#    include "pstore/brokerface/message_type.hpp"
#    include "pstore/os/descriptor.hpp"
#    include "pstore/os/logging.hpp"
#    include "pstore/support/gsl.hpp"
#    include "pstore/support/utf.hpp"

namespace {
    using namespace pstore;
    using namespace pstore::broker;


    // error_message
    // ~~~~~~~~~~~~~
    /// Yields the description of a win32 error code.
    std::string error_message (DWORD errcode) {
        if (errcode == 0) {
            return "no error";
        }

        wchar_t * ptr = nullptr;
        constexpr DWORD flags =
            FORMAT_MESSAGE_FROM_SYSTEM       // use system message tables to retrieve error text
            | FORMAT_MESSAGE_ALLOCATE_BUFFER // allocate buffer on the local heap for error text
            | FORMAT_MESSAGE_IGNORE_INSERTS;
        std::size_t size = ::FormatMessageW (flags,
                                             nullptr, // unused with FORMAT_MESSAGE_FROM_SYSTEM
                                             errcode, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                                             reinterpret_cast<wchar_t *> (&ptr), // output
                                             0, // minimum size for output buffer
                                             nullptr);
        std::unique_ptr<wchar_t, decltype (&LocalFree)> error_text (ptr, &LocalFree);
        ptr = nullptr; // The pointer is now owned by the error_text unique_ptr.

        if (error_text) {
            // The string returned by FormatMessage probably ends with a CR. Remove it.
            auto is_space = [] (wchar_t const c) {
                return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
            };
            for (; size > 0 && is_space (error_text.get ()[size - 1]); --size) {
            }
        }
        if (!error_text || size == 0) {
            return "unknown error";
        }
        return utf::win32::to8 (error_text.get (), size);
    }

    //*                  _          *
    //*  _ _ ___ __ _ __| |___ _ _  *
    //* | '_/ -_) _` / _` / -_) '_| *
    //* |_| \___\__,_\__,_\___|_|   *
    //*                             *
    class reader {
    public:
        reader () = default;
        reader (pipe_descriptor && ph, gsl::not_null<command_processor *> cp,
                recorder * record_file);
        reader (reader const &) = delete;
        reader (reader &&) = delete;
        ~reader () noexcept;

        reader & operator= (reader const &) = delete;
        reader & operator= (reader &&) = delete;

        reader * initiate_read ();
        void cancel ();

        list_member<reader> & get_list_member () noexcept { return listm_; }

    private:
        /// Must be the first object in the structure. The address of this member is passed to the
        /// read_completed() function when the OS notifies us of the completion of a read.
        OVERLAPPED overlap_ = {0};

        list_member<reader> listm_;

        pipe_descriptor pipe_handle_;
        brokerface::message_ptr request_;

        command_processor * command_processor_ = nullptr;
        recorder * record_file_ = nullptr;

        /// Is a read using this buffer in progress? Used as a debugging check to ensure that
        /// the object is not "active" when it is being destroyed.
        bool is_in_flight_ = false;

        bool read ();
        static VOID WINAPI read_completed (DWORD errcode, DWORD bytes_read, LPOVERLAPPED overlap);

        void completed ();
        void completed_with_error ();

        /// Called when the series of reads for a connection has been completed. This function
        /// removes the reader from the list of in-flight reads and deletes the associated object.
        ///
        /// \param r  The reader instance to be removed and deleted.
        /// \returns Always nullptr
        static reader * done (reader * r) noexcept;
    };


    // (ctor)
    // ~~~~~~
    reader::reader (pipe_descriptor && ph, gsl::not_null<command_processor *> cp,
                    recorder * record_file)
            : pipe_handle_{std::move (ph)}
            , command_processor_{cp}
            , record_file_{record_file} {
        using T = std::remove_pointer<decltype (this)>::type;
        static_assert (offsetof (T, overlap_) == 0,
                       "OVERLAPPED must be the first member of the request structure");
        PSTORE_ASSERT (pipe_handle_.valid ());
    }

    // (dtor)
    // ~~~~~~
    reader::~reader () noexcept {
        PSTORE_ASSERT (!is_in_flight_);
        // Close this pipe instance.
        if (pipe_handle_.valid ()) {
            ::DisconnectNamedPipe (pipe_handle_.native_handle ());
        }
    }

    // done
    // ~~~~
    reader * reader::done (reader * r) noexcept {
        PSTORE_ASSERT (!r->is_in_flight_);
        intrusive_list<reader>::erase (r);
        delete r;
        return nullptr;
    }

    // initiate_read
    // ~~~~~~~~~~~~~
    reader * reader::initiate_read () {
        // Start an asynchronous read. This will either start to read data if it's available
        // or signal (by returning nullptr) that we've read all of the data already. In the latter
        // case we tear down this reader instance by calling done().
        return this->read () ? this : this->done (this);
    }

    // cancel
    // ~~~~~~
    void reader::cancel () { ::CancelIoEx (pipe_handle_.native_handle (), &overlap_); }

    // read
    // ~~~~
    /// \return True if the pipe read does not return an error. If false is returned, the client has
    /// gone away and this pipe instance should be closed.
    bool reader::read () {
        PSTORE_ASSERT (!is_in_flight_);

        // Pull a buffer from pool to use for storing the data read from the pipe connection.
        PSTORE_ASSERT (request_.get () == nullptr);
        request_ = pool.get_from_pool ();

        PSTORE_ASSERT (sizeof (*request_) <= std::numeric_limits<DWORD>::max ());

        // Start the read and call read_completed() when it finishes.
        is_in_flight_ =
            ::ReadFileEx (pipe_handle_.native_handle (), request_.get (),
                          static_cast<DWORD> (sizeof (*request_)), &overlap_, &read_completed);
        DWORD const erc = ::GetLastError ();
        if (erc != ERROR_SUCCESS && erc != ERROR_BROKEN_PIPE && erc != ERROR_IO_PENDING) {
            std::ostringstream message;
            message << error_message (erc) << '(' << erc << ')';
            log (logger::priority::error, "ReadFileEx: ", message.str ());
        }

        return is_in_flight_;
    }


    // read_completed
    // ~~~~~~~~~~~~~~
    /// An I/O completion routine that's called after a read request completes.
    /// If we've received a complete message, then it is queued for processing. We then try to read
    /// more from the client.
    ///
    /// \param errcode  The I/O completion status: one of the system error codes.
    /// \param bytes_read  This number of bytes transferred.
    /// \param overlap  A pointer to the OVERLAPPED structure specified by the asynchronous I/O
    ///   function.
    VOID WINAPI reader::read_completed (DWORD errcode, DWORD bytes_read, LPOVERLAPPED overlap) {
        auto * r = reinterpret_cast<reader *> (overlap);
        PSTORE_ASSERT (r != nullptr);

        try {
            PSTORE_ASSERT (r->is_in_flight_);
            r->is_in_flight_ = false;

            if (errcode == ERROR_SUCCESS && bytes_read != 0) {
                if (bytes_read != brokerface::message_size) {
                    log (logger::priority::error, "Partial message received. Length ", bytes_read);
                    r->completed_with_error ();
                } else {
                    // The read operation has finished successfully, so process the request.
                    log (logger::priority::debug, "Queueing command");
                    r->completed ();
                }
            } else if (errcode != ERROR_SUCCESS) {
                if (errcode == ERROR_BROKEN_PIPE) {
                    log (logger::priority::debug, "Pipe was broken");
                } else {
                    std::ostringstream stream;
                    stream << error_message (errcode) << " (" << errcode << ')';
                    log (logger::priority::error, "Read completed with error: ", stream.str ());
                }
                r->completed_with_error ();
            }

            // Try reading some more from this pipe client.
            r = r->initiate_read ();
        } catch (std::exception const & ex) {
            log (logger::priority::error, "error: ", ex.what ());
            // This object should now kill itself?
        } catch (...) {
            log (logger::priority::error, "unknown error");
            // This object should now kill itself?
        }
    }

    // completed
    // ~~~~~~~~~
    void reader::completed () {
        PSTORE_ASSERT (command_processor_ != nullptr);
        command_processor_->push_command (std::move (request_), record_file_);
    }

    // completed_with_error
    // ~~~~~~~~~~~~~~~~~~~~
    void reader::completed_with_error () { request_.reset (); }


    //*                           _    *
    //*  _ _ ___ __ _ _  _ ___ __| |_  *
    //* | '_/ -_) _` | || / -_|_-<  _| *
    //* |_| \___\__, |\_,_\___/__/\__| *
    //*            |_|                 *
    /// Manages the process of asynchronously reading from the named pipe.
    class request {
    public:
        request (gsl::not_null<command_processor *> cp, recorder * record_file);

        void attach_pipe (pipe_descriptor && p);

        void cancel ();

    private:
        intrusive_list<reader> list_;

        gsl::not_null<command_processor *> command_processor_;
        recorder * const record_file_;

        /// A class used to insert a reader instance into the list of extant reads and
        /// move it from that list unless ownership has been released (by a call to the
        /// release() method).
        class raii_insert {
        public:
            raii_insert (intrusive_list<reader> & list, reader * r) noexcept
                    : r_{r} {
                list.insert_before (r, list.tail ());
            }
            raii_insert (raii_insert const &) = delete;
            raii_insert (raii_insert &&) = delete;

            ~raii_insert () noexcept {
                if (r_) {
                    intrusive_list<reader>::erase (r_);
                }
            }

            raii_insert & operator= (raii_insert const &) = delete;
            raii_insert & operator= (raii_insert &&) = delete;

            void release () noexcept { r_ = nullptr; }

        private:
            reader * r_;
        };
    };

    // (ctor)
    // ~~~~~~
    request::request (gsl::not_null<command_processor *> cp, recorder * record_file)
            : command_processor_{cp}
            , record_file_{record_file} {}

    // attach_pipe
    // ~~~~~~~~~~~
    /// Associates the given pipe handle with this request object and starts a read operation.
    void request::attach_pipe (pipe_descriptor && pipe) {
        auto r = std::make_unique<reader> (std::move (pipe), command_processor_, record_file_);

        // Insert this new object into the list of active reads. We now have two pointers to it:
        // the unique_ptr and the one in the list.
        raii_insert inserter (list_, r.get ());

        r->initiate_read ();

        inserter.release ();
        r.release ();
    }

    // cancel
    // ~~~~~~
    void request::cancel () {
        list_.check ();
        for (reader & r : list_) {
            r.cancel ();
        }
    }


    // connect_to_new_client
    // ~~~~~~~~~~~~~~~~~~~~~
    /// Initiates the connection between a named pipe and a client.
    bool connect_to_new_client (HANDLE pipe, OVERLAPPED & overlapped) {
        // Start an overlapped connection for this pipe instance.
        BOOL const cnp_res = ::ConnectNamedPipe (pipe, &overlapped);

        // From MSDN: "In nonblocking mode, ConnectNamedPipe() returns a non-zero value the first
        // time it is called for a pipe instance that is disconnected from a previous client. This
        // indicates that the pipe is now available to be connected to a new client process. In all
        // other situations when the pipe handle is in nonblocking mode, ConnectNamedPipe() returns
        // zero."

        DWORD const errcode = ::GetLastError ();
        if (cnp_res) {
            raise (win32_erc (errcode), "ConnectNamedPipe");
        }

        bool pending_io = false;
        switch (errcode) {
        case ERROR_IO_PENDING:
            pending_io = true; // The overlapped connection in progress.
            break;
        case ERROR_NO_DATA:
        case ERROR_PIPE_CONNECTED:
            // The client is already connected, so signal an event.
            if (!::SetEvent (overlapped.hEvent)) {
                raise (win32_erc (::GetLastError ()), "SetEvent");
            }
            break;
        default:
            // An error occurred during the connect operation.
            raise (win32_erc (errcode), "ConnectNamedPipe");
        }
        return pending_io;
    }


    // create_and_connect_instance
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /// Creates a pipe instance and connects to the client.
    /// \param pipe_name  The name of the pipe.
    /// \param overlap  A OVERLAPPED instance which is used to track the state of the asynchronour
    ///   pipe creation.
    /// \return  A pair containing both the pipe handle and a boolean which will be true if the
    ///   connect operation is pending, and false if the connection has been completed.
    std::pair<pipe_descriptor, bool> create_and_connect_instance (std::wstring const & pipe_name,
                                                                  OVERLAPPED & overlap) {
        // The default time-out value, in milliseconds,
        static constexpr auto default_pipe_timeout =
            DWORD{5 * 1000}; // TODO: make this user-configurable.

        auto pipe =
            pipe_descriptor{::CreateNamedPipeW (pipe_name.c_str (),
                                                PIPE_ACCESS_INBOUND |         // read/write access
                                                    FILE_FLAG_OVERLAPPED,     // overlapped mode
                                                PIPE_TYPE_BYTE |              // message-type pipe
                                                    PIPE_READMODE_BYTE |      // message-read mode
                                                    PIPE_WAIT,                // blocking mode
                                                PIPE_UNLIMITED_INSTANCES,     // unlimited instances
                                                0,                            // output buffer size
                                                brokerface::message_size * 4, // input buffer size
                                                default_pipe_timeout,         // client time-out
                                                nullptr)}; // default security attributes
        if (pipe.native_handle () == INVALID_HANDLE_VALUE) {
            raise (win32_erc (::GetLastError ()), "CreateNamedPipeW");
        }

        auto const pending_io = connect_to_new_client (pipe.native_handle (), overlap);
        return std::make_pair (std::move (pipe), pending_io);
    }


    // create_event
    // ~~~~~~~~~~~~
    /// Creates a manual-reset event which is initially signaled.
    pipe_descriptor create_event () {
        if (HANDLE h = ::CreateEvent (nullptr, true, true, nullptr)) {
            return pipe_descriptor{h};
        }
        raise (win32_erc (::GetLastError ()), "CreateEvent");
    }

} // end anonymous namespace


namespace pstore {
    namespace broker {

        // read_loop
        // ~~~~~~~~~
        void read_loop (brokerface::fifo_path & path, std::shared_ptr<recorder> & record_file,
                        std::shared_ptr<command_processor> cp) {
            try {
                log (logger::priority::notice, "listening to named pipe ",
                     logger::quoted (path.get ().c_str ()));
                auto const pipe_name = utf::win32::to16 (path.get ());
                // Create one event object for the connect operation.
                unique_handle connect_event = create_event ();

                OVERLAPPED connect{0};
                connect.hEvent = connect_event.native_handle ();

                // Create a pipe instance and and wait for a the client to connect.
                bool pending_io;
                pipe_descriptor pipe;
                std::tie (pipe, pending_io) = create_and_connect_instance (pipe_name, connect);

                request req (cp.get (), record_file.get ());

                while (!done) {
                    // Wait for a client to connect, or for a read or write operation to be
                    // completed, which causes a completion routine to be queued for execution.
                    auto const cause = ::WaitForSingleObjectEx (connect_event.native_handle (),
                                                                details::timeout_seconds * 1000,
                                                                true /*alertable wait?*/);
                    switch (cause) {
                    case WAIT_OBJECT_0:
                        // A connect operation has been completed. If an operation is pending, get
                        // the result of the connect operation.
                        if (pending_io) {
                            auto bytes_transferred = DWORD{0};
                            if (!::GetOverlappedResult (pipe.native_handle (), &connect,
                                                        &bytes_transferred,
                                                        false /*do not wait*/)) {
                                raise (win32_erc (::GetLastError ()), "ConnectNamedPipe");
                            }
                        }

                        // Start the read operation for this client.
                        req.attach_pipe (std::move (pipe));

                        // Create new pipe instance for the next client.
                        std::tie (pipe, pending_io) =
                            create_and_connect_instance (pipe_name, connect);
                        break;

                    case WAIT_IO_COMPLETION:
                        // The wait was satisfied by a completed read operation.
                        break;
                    case WAIT_TIMEOUT: log (logger::priority::notice, "wait timeout"); break;
                    default: raise (win32_erc (::GetLastError ()), "WaitForSingleObjectEx");
                    }
                }

                // Try to cancel any reads that are still in-flight.
                req.cancel ();
            } catch (std::exception const & ex) {
                log (logger::priority::error, "error: ", ex.what ());
                exit_code = EXIT_FAILURE;
                notify_quit_thread ();
            } catch (...) {
                log (logger::priority::error, "unknown error");
                exit_code = EXIT_FAILURE;
                notify_quit_thread ();
            }
            log (logger::priority::notice, "exiting read loop");
        }

    } // namespace broker
} // namespace pstore

#endif // _WIN32
