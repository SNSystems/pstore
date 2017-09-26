//*                     _   _                    *
//*  _ __ ___  __ _  __| | | | ___   ___  _ __   *
//* | '__/ _ \/ _` |/ _` | | |/ _ \ / _ \| '_ \  *
//* | | |  __/ (_| | (_| | | | (_) | (_) | |_) | *
//* |_|  \___|\__,_|\__,_| |_|\___/ \___/| .__/  *
//*                                      |_|     *
//===- lib/broker/read_loop_win32.cpp -------------------------------------===//
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
/// \file read_loop_win32.cpp
/// \brief The read loop thread entry point for Windows.

#include "broker/read_loop.hpp"

#ifdef _WIN32

// Standard includes
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <utility>

// Platform includes
#include <Windows.h>

// pstore includes
#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/message_type.hpp"
#include "pstore_broker_intf/unique_handle.hpp"
#include "pstore_support/logging.hpp"
#include "pstore_support/utf.hpp"

// Local includes
#include "broker/command.hpp"
#include "broker/globals.hpp"
#include "broker/message_pool.hpp"
#include "broker/recorder.hpp"

namespace {
    //*                           _    *
    //*  _ _ ___ __ _ _  _ ___ __| |_  *
    //* | '_/ -_) _` | || / -_|_-<  _| *
    //* |_| \___\__, |\_,_\___/__/\__| *
    //*            |_|                 *
    /// Manages the process of asynchronously reading from the named pipe.
    class request {
    public:
        explicit request (command_processor * const cp, recorder * const record_file);
        void attach_pipe (pstore::broker::unique_handle && p);

    private:
        class reader {
        public:
            explicit reader (pstore::broker::unique_handle && ph, command_processor * const cp,
                             recorder * const record_file);
            ~reader ();

            bool initiate (OVERLAPPED & overlap, LPOVERLAPPED_COMPLETION_ROUTINE completion);
            void completed ();

        private:
            pstore::broker::unique_handle pipe_handle_;
            pstore::broker::message_ptr request_;

            command_processor * const command_processor_;
            recorder * const record_file_;
        };

        void initiate_read ();
        static VOID WINAPI read_completed (DWORD errcode, DWORD bytes_read, LPOVERLAPPED overlap);
        OVERLAPPED overlap_ = {0};
        std::unique_ptr<reader> pipe_;

        command_processor * const command_processor_;
        recorder * const record_file_;
    };

    // (ctor)
    // ~~~~~~
    request::request (command_processor * const cp, recorder * const record_file)
            : command_processor_{cp}
            , record_file_{record_file} {
        static_assert (offsetof (request, overlap_) == 0,
                       "OVERLAPPED must be the first member of the request structure");
    }

    // initiate_read
    // ~~~~~~~~~~~~~
    void request::initiate_read () {
        if (!pipe_->initiate (overlap_, read_completed)) {
            // Close this pipe instance.
            pipe_.reset ();
        }
    }

    // attach_pipe
    // ~~~~~~~~~~~
    /// Associates the given pipe handle with this request object and starts a read operation.
    void request::attach_pipe (pstore::broker::unique_handle && p) {
        pipe_ = std::make_unique<reader> (std::move (p), command_processor_, record_file_);
        this->initiate_read ();
    }

    // read_completed
    // ~~~~~~~~~~~~~~
    /// An I/O completion routine that's called after a read request completes.
    VOID WINAPI request::read_completed (DWORD errcode, DWORD bytes_read, LPOVERLAPPED overlap) {
        try {
            auto & req = *reinterpret_cast<request *> (overlap);
            if (errcode == ERROR_SUCCESS && bytes_read != 0) {
                if (bytes_read != pstore::broker::message_size) {
                    pstore::logging::log (pstore::logging::priority::error, "partial message received (length=",
                                  bytes_read, ")");
                } else {
                    // The read operation has finished, so process the request.
                    req.pipe_->completed ();
                }
            }
            // Try reading some more from this pipe client.
            req.initiate_read ();
        } catch (std::exception const & ex) {
            pstore::logging::log (pstore::logging::priority::error, "error: ", ex.what ());
        } catch (...) {
            pstore::logging::log (pstore::logging::priority::error, "unknown error");
        }
    }

    // (ctor)
    // ~~~~~~
    request::reader::reader (pstore::broker::unique_handle && ph, command_processor * const cp,
                             recorder * const record_file)
            : pipe_handle_{std::move (ph)}
            , command_processor_{cp}
            , record_file_{record_file} {
        assert (pipe_handle_.valid ());
    }

    // (dtor)
    // ~~~~~~
    request::reader::~reader () {
        assert (pipe_handle_.valid ());
        ::DisconnectNamedPipe (pipe_handle_.get ());
    }

    // initiate
    // ~~~~~~~~
    /// \return True if the pipe read does not return an error. If false is returned, the client has
    /// gone away and this pipe instance should be closed.
    bool request::reader::initiate (OVERLAPPED & overlap,
                                    LPOVERLAPPED_COMPLETION_ROUTINE completion) {
        assert (sizeof (*request_) <= std::numeric_limits<DWORD>::max ());
        request_ = pool.get_from_pool ();
        return ::ReadFileEx (pipe_handle_.get (), request_.get (),
                             static_cast<DWORD> (sizeof (*request_)), &overlap,
                             completion) != FALSE;
    }

    // completed
    // ~~~~~~~~~
    void request::reader::completed () {
        command_processor_->push_command (std::move (request_), record_file_);
    }

} // (anonymous namespace)

namespace {

    // connect_to_new_client
    // ~~~~~~~~~~~~~~~~~~~~~
    /// Initiates the connection between a named pipe and a client.
    bool connect_to_new_client (HANDLE pipe, OVERLAPPED & overlapped) {
        // Start an overlapped connection for this pipe instance.
        auto cnp_res = ::ConnectNamedPipe (pipe, &overlapped);

        // From MSDN: "In nonblocking mode, ConnectNamedPipe() returns a non-zero value the first
        // time it is called for a pipe instance that is disconnected from a previous client. This
        // indicates that the pipe is now available to be connected to a new client process. In all
        // other situations when the pipe handle is in nonblocking mode, ConnectNamedPipe() returns
        // zero."

        auto const errcode = ::GetLastError ();
        if (cnp_res) {
            raise (::pstore::win32_erc (errcode), "ConnectNamedPipe");
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
                raise (::pstore::win32_erc (::GetLastError ()), "SetEvent");
            }
            break;
        default:
            // An error occurred during the connect operation.
            raise (::pstore::win32_erc (errcode), "ConnectNamedPipe");
        }
        return pending_io;
    }


    // create_and_connect_instance
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    /// Creates a pipe instance and connects to the client.
    /// \param pipe_name  The name of the pipe.
    /// \param overlap  A OVERLAPPED instance which is used to track the state of the asynchronour
    /// pipe creation.
    /// \return  A pair containing both the pipe handle and a boolean which will be true if the
    /// connect operation is pending, and false if the connection has been completed.
    std::pair<pstore::broker::unique_handle, bool>
    create_and_connect_instance (std::wstring const & pipe_name, OVERLAPPED & overlap) {
        // The default time-out value, in milliseconds,
        static constexpr auto default_pipe_timeout = DWORD{5 * 1000};

        pstore::broker::unique_handle pipe =
            ::CreateNamedPipeW (pipe_name.c_str (),
                                PIPE_ACCESS_INBOUND |             // read/write access
                                    FILE_FLAG_OVERLAPPED,         // overlapped mode
                                PIPE_TYPE_BYTE |                  // message-type pipe
                                    PIPE_READMODE_BYTE |          // message-read mode
                                    PIPE_WAIT,                    // blocking mode
                                PIPE_UNLIMITED_INSTANCES,         // unlimited instances
                                0,                                // output buffer size
                                pstore::broker::message_size * 4, // input buffer size
                                default_pipe_timeout,             // client time-out
                                nullptr);                         // default security attributes
        if (pipe.get () == INVALID_HANDLE_VALUE) {
            raise (::pstore::win32_erc (::GetLastError ()), "CreateNamedPipeW");
        }

        auto const pending_io = connect_to_new_client (pipe.get (), overlap);
        return std::make_pair (std::move (pipe), pending_io);
    }


    // create_event
    // ~~~~~~~~~~~~
    /// Creates a manual-reset event which is initially signaled.
    pstore::broker::unique_handle create_event () {
        if (HANDLE h = ::CreateEvent (nullptr, true, true, nullptr)) {
            return {h};
        } else {
            raise (::pstore::win32_erc (::GetLastError ()), "CreateEvent");
        }
    }

} // (anonymous namespace)


// read_loop
// ~~~~~~~~~
void read_loop (pstore::broker::fifo_path & path, std::shared_ptr<recorder> & record_file,
                std::shared_ptr<command_processor> & cp) {
    try {
        pstore::logging::log (pstore::logging::priority::notice, "listening to named pipe (", path.get (), ")");
        auto const pipe_name = pstore::utf::win32::to16 (path.get ());

        // Create one event object for the connect operation.
        pstore::broker::unique_handle connect_event = create_event ();

        OVERLAPPED connect{0};
        connect.hEvent = connect_event.get ();

        // Create a pipe instance and and wait for a the client to connect.
        bool pending_io;
        pstore::broker::unique_handle pipe;
        std::tie (pipe, pending_io) = create_and_connect_instance (pipe_name, connect);

        request req (cp.get (), record_file.get ());

        while (!done) {
            constexpr DWORD timeout_ms = 60 * 1000; // 60 seconds // TODO: shared with POSIX
            // Wait for a client to connect, or for a read or write operation to be completed,
            // which causes a completion routine to be queued for execution.
            auto const cause = ::WaitForSingleObjectEx (connect_event.get (), timeout_ms,
                                                        true /*alertable wait?*/);
            switch (cause) {
            case WAIT_OBJECT_0:
                // A connect operation has been completed. If an operation is pending, get the
                // result of the connect operation.
                if (pending_io) {
                    auto bytes_transferred = DWORD{0};
                    if (!::GetOverlappedResult (pipe.get (), &connect, &bytes_transferred,
                                                false /*do not wait*/)) {
                        raise (::pstore::win32_erc (::GetLastError ()), "ConnectNamedPipe");
                    }
                }

                // Start the read operation for this client.
                req.attach_pipe (std::move (pipe));

                // Create new pipe instance for the next client.
                std::tie (pipe, pending_io) = create_and_connect_instance (pipe_name, connect);
                break;

            case WAIT_IO_COMPLETION:
                // The wait was satisfied by a completed read operation.
                break;
            case WAIT_TIMEOUT:
                pstore::logging::log (pstore::logging::priority::notice, "wait timeout");
                break;
            default:
                raise (::pstore::win32_erc (::GetLastError ()), "WaitForSingleObjectEx");
            }
        }
    } catch (std::exception const & ex) {
        pstore::logging::log (pstore::logging::priority::error, "error: ", ex.what ());
        throw;
    } catch (...) {
        pstore::logging::log (pstore::logging::priority::error, "unknown error");
        throw;
    }
    pstore::logging::log (pstore::logging::priority::notice, "exiting read loop");
}

#endif // _WIN32
// eof: lib/broker/read_loop_win32.cpp
