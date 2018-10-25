//*                    _ _                                  _     _            *
//*  _ __ ___   ___ __| (_)___  __ _ ___ ___  ___ _ __ ___ | |__ | | ___ _ __  *
//* | '_ ` _ \ / __/ _` | / __|/ _` / __/ __|/ _ \ '_ ` _ \| '_ \| |/ _ \ '__| *
//* | | | | | | (_| (_| | \__ \ (_| \__ \__ \  __/ | | | | | |_) | |  __/ |    *
//* |_| |_| |_|\___\__,_|_|___/\__,_|___/___/\___|_| |_| |_|_.__/|_|\___|_|    *
//*                                                                            *
//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===- lib/dump/mcdisassembler_value.cpp ----------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#include "pstore/dump/mcdisassembler_value.hpp"

#include "pstore/config/config.hpp"

#if PSTORE_IS_INSIDE_LLVM

#include "pstore/dump/error.hpp"
#include "pstore/dump/line_splitter.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/MC/MCInstrInfo.h"

#include "pstore/support/bit_count.hpp"
#include "pstore/support/error.hpp"

namespace {

    class array_stream : public llvm::raw_ostream {
    public:
        array_stream (pstore::dump::array::container & array)
                : splitter_ (&array) {}

    private:
        void write_impl (char const * ptr, std::size_t size) override {
            using namespace pstore;
            splitter_.append (gsl::make_span (ptr, size), &dump::trim_line);
            pos_ += size;
        }
        std::uint64_t current_pos () const override { return pos_; }

        pstore::dump::line_splitter splitter_;
        std::size_t pos_ = 0;
    };


    unsigned hex_width (std::uint8_t const * first, std::uint8_t const * last) {
        assert (last >= first);
        auto const power = 64U - pstore::bit_count::clz (last - first);
        auto const num_bytes = (power + 8U - 1U) / 8U;
        return std::max (num_bytes, 1U) * 2U + 2U;
    }

    bool disasm_block (std::uint8_t const * first, std::uint8_t const * last,
                       pstore::dump::array::container & array, pstore::gsl::czstring triple_str,
                       bool hex_mode) {
        if (first == last) {
            return true;
        }

        // Get the target specific parser.
        std::string error;
        llvm::Triple triple{triple_str};
        llvm::Target const * const target =
            llvm::TargetRegistry::lookupTarget ("" /*arch-name*/, triple, error);
        if (target == nullptr) {
            return false; // unknown target: fall back to producing a hex-dump.
        }

        // Update the triple name and return the found target.
        std::string const & triple_name = triple.getTriple ();

        auto const features = llvm::SubtargetFeatures ();
        std::unique_ptr<llvm::MCRegisterInfo const> register_info (
            target->createMCRegInfo (triple_name));
        if (!register_info) {
            pstore::raise (pstore::dump::error_code::no_register_info_for_target, triple_name);
        }

        std::unique_ptr<llvm::MCAsmInfo const> asm_info (
            target->createMCAsmInfo (*register_info, triple_name));
        if (asm_info == nullptr) {
            pstore::raise (pstore::dump::error_code::no_assembly_info_for_target, triple_name);
        }

        std::string cpu; // triple should be enough!
        std::unique_ptr<llvm::MCSubtargetInfo const> subtarget_info (target->createMCSubtargetInfo (
            triple_name, llvm::StringRef (cpu), features.getString ()));
        if (subtarget_info == nullptr) {
            pstore::raise (pstore::dump::error_code::no_subtarget_info_for_target, triple_name);
        }

        std::unique_ptr<llvm::MCInstrInfo const> instruction_info (target->createMCInstrInfo ());
        if (instruction_info == nullptr) {
            pstore::raise (pstore::dump::error_code::no_instruction_info_for_target, triple_name);
        }

        llvm::MCObjectFileInfo object_file_info;
        llvm::MCContext context (asm_info.get (), register_info.get (), &object_file_info);
        object_file_info.InitMCObjectFileInfo (triple, false /*PIC*/, context);

        std::unique_ptr<llvm::MCDisassembler> disassembler (
            target->createMCDisassembler (*subtarget_info, context));
        if (disassembler == nullptr) {
            pstore::raise (pstore::dump::error_code::no_disassembler_for_target, triple_name);
        }

        std::unique_ptr<llvm::MCInstPrinter> instruction_printer (
            target->createMCInstPrinter (triple, asm_info->getAssemblerDialect (), *asm_info,
                                         *instruction_info, *register_info));

        instruction_printer->setPrintImmHex (hex_mode);


        auto os = llvm::make_unique<array_stream> (array);
        auto formatted_os = llvm::make_unique<llvm::formatted_raw_ostream> (*os);
        auto raw_formatted_os = formatted_os.get ();
        std::unique_ptr<llvm::MCCodeEmitter> MCE;
        std::unique_ptr<llvm::MCAsmBackend> MAB;
        std::unique_ptr<llvm::MCStreamer> streamer (
            llvm::createAsmStreamer (context, std::move (formatted_os), false /*is verbose asm*/,
                                     false /*use dwarf directory*/, instruction_printer.release (),
                                     std::move (MCE) /*MCCodeEmitter*/,
                                     std::move (MAB) /*MCAsmBackend*/, true /*showInst*/));
        streamer->InitSections (false /*NoExecStack*/);


        auto address = std::uint64_t{0};
        auto const hex_digits = hex_width (first, last);

        while (first < last) {
            if (hex_mode) {
                (*raw_formatted_os) << llvm::format_hex (address, hex_digits, false /*upper*/);
            } else {
                (*raw_formatted_os) << address;
            }
            (*raw_formatted_os) << ':';

            llvm::MCInst instruction;
            std::uint64_t bytes_consumed = 0;
            llvm::MCDisassembler::DecodeStatus const S = disassembler->getInstruction (
                instruction,                                // an MCInst to populate
                bytes_consumed,                             // out: number of bytes consumed
                llvm::ArrayRef<std::uint8_t> (first, last), // actual bytes of the instruction
                address,         // address of the first byte of the instruction
                llvm::nulls (),  // stream on which to print warnings and messages
                llvm::nulls ()); // stream on which to print comments and annotations
            switch (S) {
            case llvm::MCDisassembler::Fail:
                (*raw_formatted_os) << "invalid instruction\n";
                if (bytes_consumed == 0) {
                    bytes_consumed = 1; // skip illegible bytes
                }
                break;
            case llvm::MCDisassembler::SoftFail:
                (*raw_formatted_os) << "potentially undefined instruction:";
                LLVM_FALLTHROUGH;
            case llvm::MCDisassembler::Success:
                streamer->EmitInstruction (instruction, *subtarget_info);
                break;
            }
            address += bytes_consumed;
            first += bytes_consumed;
        }
        return true;
    }

} // anonymous namespace

#endif // PSTORE_IS_INSIDE_LLVM

namespace {

    pstore::dump::value_ptr make_hex_dump_value (std::uint8_t const * first,
                                                 std::uint8_t const * last, bool hex_mode) {
        pstore::dump::value_ptr v;
        if (hex_mode) {
            v = std::make_shared<pstore::dump::binary16> (first, last);
        } else {
            v = std::make_shared<pstore::dump::binary> (first, last);
        }
        return v;
    }

} // end anonymous namespace

#if PSTORE_IS_INSIDE_LLVM

namespace pstore {
    namespace dump {

        value_ptr make_disassembled_value (std::uint8_t const * first, std::uint8_t const * last,
                                           gsl::czstring triple, bool hex_mode) {

            array::container arr;
            if (disasm_block (first, last, arr, triple, hex_mode)) {
                return make_value (arr);
            }
            return make_hex_dump_value (first, last, hex_mode);
        }

    } // namespace dump
} // namespace pstore

#else // PSTORE_IS_INSIDE_LLVM

namespace pstore {
    namespace dump {

        value_ptr make_disassembled_value (std::uint8_t const * first, std::uint8_t const * last,
                                           gsl::czstring /*triple*/, bool hex_mode) {
            return make_hex_dump_value (first, last, hex_mode);
        }

    } // namespace dump
} // namespace pstore

#endif // PSTORE_IS_INSIDE_LLVM
