//*                                                _  *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| *
//*                                                   *
//===- unittests/broker/test_command.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "pstore/broker/command.hpp"

#include <chrono>
#include <functional>

#include "gmock/gmock.h"

#include "pstore/brokerface/fifo_path.hpp"
#include "pstore/http/server_status.hpp"

using namespace std::chrono_literals;

namespace {

    class mock_cp : public pstore::broker::command_processor {
    public:
        explicit mock_cp (unsigned const num_read_threads,
                          pstore::maybe<pstore::http::server_status> * const http_status,
                          std::atomic<bool> * const uptime_done)
                : command_processor (num_read_threads, http_status, uptime_done, 4h) {}

        MOCK_METHOD2 (suicide, void (pstore::brokerface::fifo_path const &,
                                     pstore::broker::broker_command const &));
        MOCK_METHOD2 (quit, void (pstore::brokerface::fifo_path const &,
                                  pstore::broker::broker_command const &));
        MOCK_METHOD2 (cquit, void (pstore::brokerface::fifo_path const &,
                                   pstore::broker::broker_command const &));
        MOCK_METHOD2 (gc, void (pstore::brokerface::fifo_path const &,
                                pstore::broker::broker_command const &));
        MOCK_METHOD2 (echo, void (pstore::brokerface::fifo_path const &,
                                  pstore::broker::broker_command const &));
        MOCK_METHOD2 (nop, void (pstore::brokerface::fifo_path const &,
                                 pstore::broker::broker_command const &));
        MOCK_CONST_METHOD1 (unknown, void (pstore::broker::broker_command const &));

        // Replace the log message with an implementation that does nothing at all. We don't really
        // want to be writing logs from the unit tests.
        virtual void log (pstore::broker::broker_command const &) const {}
        virtual void log (pstore::gsl::czstring) const {}
    };

    class Command : public ::testing::Test {
    public:
        Command ()
                : http_status_{pstore::in_place, in_port_t{8080}}
                , uptime_done_{false}
                , cp_{1U, &http_status_, &uptime_done_}
                , fifo_{nullptr} {}

        mock_cp & cp () { return cp_; }
        pstore::brokerface::fifo_path & fifo () { return fifo_; }

        static constexpr std::uint32_t message_id = 0;
        static constexpr std::uint16_t part_no = 0;
        static constexpr std::uint16_t num_parts = 1;

    private:
        pstore::maybe<pstore::http::server_status> http_status_;
        std::atomic<bool> uptime_done_;

        mock_cp cp_;
        pstore::brokerface::fifo_path fifo_;
    };
} // namespace

// Not a particularly useful test as such, but suppresses warnings from clang about unused member
// functions in mock_cp.
TEST_F (Command, NoCalls) {
    using ::testing::_;
    EXPECT_CALL (cp (), suicide (_, _)).Times (0);
    EXPECT_CALL (cp (), quit (_, _)).Times (0);
    EXPECT_CALL (cp (), cquit (_, _)).Times (0);
    EXPECT_CALL (cp (), gc (_, _)).Times (0);
    EXPECT_CALL (cp (), echo (_, _)).Times (0);
    EXPECT_CALL (cp (), nop (_, _)).Times (0);
}

TEST_F (Command, Nop) {
    using ::testing::_;

    EXPECT_CALL (cp (), nop (_, _)).Times (1);
    pstore::brokerface::message_type msg{message_id, part_no, num_parts, "NOP"};
    cp ().process_command (fifo (), msg);
}

TEST_F (Command, Bad) {
    EXPECT_CALL (cp (), unknown (pstore::broker::broker_command{"bad", "command"})).Times (1);

    pstore::brokerface::message_type msg{message_id, part_no, num_parts, "bad command"};
    cp ().process_command (fifo (), msg);
}
