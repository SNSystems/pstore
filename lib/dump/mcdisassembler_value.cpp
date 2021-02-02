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
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/mcdisassembler_value.hpp"

#include <type_traits>

#include "pstore/config/config.hpp"

#ifdef PSTORE_IS_INSIDE_LLVM

#    include "pstore/dump/error.hpp"
#    include "pstore/dump/line_splitter.hpp"

#    include "llvm/ADT/ArrayRef.h"
#    include "llvm/ADT/Triple.h"
#    include "llvm/MC/MCAsmBackend.h"
#    include "llvm/MC/MCAsmInfo.h"
#    include "llvm/MC/MCCodeEmitter.h"
#    include "llvm/MC/MCContext.h"
#    include "llvm/MC/MCDisassembler/MCDisassembler.h"
#    include "llvm/MC/MCInst.h"
#    include "llvm/MC/MCInstPrinter.h"
#    include "llvm/MC/MCRegisterInfo.h"
#    include "llvm/MC/MCObjectFileInfo.h"
#    include "llvm/MC/MCStreamer.h"
#    include "llvm/MC/MCSubtargetInfo.h"
#    include "llvm/MC/MCTargetOptionsCommandFlags.h"
#    include "llvm/Support/Format.h"
#    include "llvm/Support/MemoryBuffer.h"
#    include "llvm/Support/SourceMgr.h"
#    include "llvm/Support/TargetRegistry.h"
#    include "llvm/MC/MCInstrInfo.h"

#    include "pstore/adt/error_or.hpp"
#    include "pstore/support/bit_count.hpp"
#    include "pstore/support/error.hpp"
#    include "pstore/support/gsl.hpp"

namespace {

    //*                              _                       *
    //*  __ _ _ _ _ _ __ _ _  _   __| |_ _ _ ___ __ _ _ __   *
    //* / _` | '_| '_/ _` | || | (_-<  _| '_/ -_) _` | '  \  *
    //* \__,_|_| |_| \__,_|\_, | /__/\__|_| \___\__,_|_|_|_| *
    //*                    |__/                              *
    class array_stream : public llvm::raw_ostream {
    public:
        explicit array_stream (pstore::dump::array::container & array)
                : splitter_{&array} {}

    private:
        void write_impl (char const * ptr, std::size_t size) override;
        std::uint64_t current_pos () const override { return pos_; }

        pstore::dump::line_splitter splitter_;
        std::size_t pos_ = 0;
    };

    // write_impl
    // ~~~~~~~~~~
    void array_stream::write_impl (char const * ptr, std::size_t size) {
        using namespace pstore;
        splitter_.append (gsl::make_span (ptr, size), &dump::trim_line);
        pos_ += size;
    }


    //*          _    _                          _     _            *
    //*  __ _ __| |__| |_ _ ___ ______  _ __ _ _(_)_ _| |_ ___ _ _  *
    //* / _` / _` / _` | '_/ -_|_-<_-< | '_ \ '_| | ' \  _/ -_) '_| *
    //* \__,_\__,_\__,_|_| \___/__/__/ | .__/_| |_|_||_\__\___|_|   *
    //*                                |_|                          *
    class address_printer {
    public:
        address_printer (pstore::gsl::span<std::uint8_t const> const & span, bool hex_mode);

        class value {
        public:
            constexpr value (address_printer const & ap, std::uint64_t v) noexcept
                    : ap_{ap}
                    , v_{v} {}
            llvm::raw_ostream & write (llvm::raw_ostream & os) const;

        private:
            address_printer const & ap_;
            std::uint64_t v_;
        };

        value make_value (std::uint64_t v) const { return {*this, v}; }

        constexpr unsigned hex_digits () const noexcept { return digits_; }
        constexpr bool hex_mode () const noexcept { return hex_mode_; }

    private:
        /// \param span  A range of bytes.
        /// \returns The number of hex digits required to represent the size of \p span.
        static unsigned hex_width (pstore::gsl::span<std::uint8_t const> const & span);

        unsigned digits_;
        bool hex_mode_;
    };

    // ctor
    // ~~~~
    address_printer::address_printer (pstore::gsl::span<std::uint8_t const> const & span,
                                      bool hex_mode)
            : digits_{hex_width (span)}
            , hex_mode_{hex_mode} {}

    // hex_width
    // ~~~~~~~~~
    unsigned address_printer::hex_width (pstore::gsl::span<std::uint8_t const> const & span) {
        auto const size = span.size ();
        if (size <= 0) {
            return 0U;
        }
        using usize = std::remove_cv_t<std::make_unsigned_t<decltype (size)>>;
        std::size_t const num_bytes =
            (sizeof (usize) * 8U - pstore::bit_count::clz (static_cast<usize> (size)) + 8U - 1U) /
            8U;
        return std::max (num_bytes, std::size_t{1}) * 2U + 2U;
    }

    // value::write
    // ~~~~~~~~~~~~
    llvm::raw_ostream & address_printer::value::write (llvm::raw_ostream & os) const {
        if (ap_.hex_mode ()) {
            return os << llvm::format_hex (v_, ap_.hex_digits (), false /*upper*/);
        }
        return os << v_;
    }

    // operator<<
    llvm::raw_ostream & operator<< (llvm::raw_ostream & os, address_printer::value const & v) {
        return v.write (os);
    }

    //*             _           _    *
    //*  __ ___ _ _| |_ _____ _| |_  *
    //* / _/ _ \ ' \  _/ -_) \ /  _| *
    //* \__\___/_||_\__\___/_\_\\__| *
    //*                              *
    /// A class to wrap the initialization of an LLVM MCContext instance.
    class context {
    public:
        context (llvm::Triple const & triple, llvm::MCAsmInfo const * const asm_info,
                 llvm::MCRegisterInfo const * const registers)
                : context_{asm_info, registers, &object_file_} {
            object_file_.InitMCObjectFileInfo (triple, false /*PIC*/, context_);
        }
        context (context const &) = delete;
        context (context &&) = delete;
        context & operator= (context const &) = delete;
        context & operator= (context &&) = delete;

        auto & get () const { return context_; }
        auto & get () { return context_; }

    private:
        llvm::MCObjectFileInfo object_file_;
        llvm::MCContext context_;
    };


    using register_ptr = std::unique_ptr<llvm::MCRegisterInfo const>;
    using asm_info_ptr = std::unique_ptr<llvm::MCAsmInfo const>;
    using subtarget_ptr = std::unique_ptr<llvm::MCSubtargetInfo const>;
    using instr_info_ptr = std::unique_ptr<llvm::MCInstrInfo const>;
    using disasm_ptr = std::unique_ptr<llvm::MCDisassembler>;
    using inst_printer_ptr = std::unique_ptr<llvm::MCInstPrinter>;
    using streamer_ptr = std::unique_ptr<llvm::MCStreamer>;

    using pstore::error_or;
    using pstore::dump::error_code;

    auto create_target (llvm::Triple triple) {
        using return_type = error_or<llvm::Target const *>;

        std::string error;
        static constexpr pstore::gsl::czstring arch_name = "";
        if (llvm::Target const * const target =
                llvm::TargetRegistry::lookupTarget (arch_name, triple, error)) {
            return return_type{target};
        }
        // unknown target: fall back to producing a hex-dump.
        return return_type{make_error_code (error_code::unknown_target)};
    }

    auto create_register_info (llvm::Target const & target, llvm::Triple const & triple) {
        using return_type = error_or<register_ptr>;
        if (register_ptr register_info{target.createMCRegInfo (triple.getTriple ())}) {
            return return_type{std::move (register_info)};
        }
        return return_type{make_error_code (error_code::no_register_info_for_target)};
    }

    auto create_asm_info (llvm::Target const & target, llvm::MCRegisterInfo const & register_info,
                          llvm::Triple const & triple) {
        using return_type = error_or<asm_info_ptr>;
        llvm::MCTargetOptions options;
        if (asm_info_ptr asm_info{
                target.createMCAsmInfo (register_info, triple.getTriple (), options)}) {
            return return_type{std::move (asm_info)};
        }
        return return_type{make_error_code (error_code::no_assembly_info_for_target)};
    }

    auto create_subtarget_info (llvm::Target const & target, llvm::Triple const & triple) {
        using return_type = error_or<subtarget_ptr>;

        std::string cpu; // triple should be enough!
        auto const features = llvm::SubtargetFeatures ();
        if (subtarget_ptr subtarget_info{target.createMCSubtargetInfo (
                triple.getTriple (), llvm::StringRef (cpu), features.getString ())}) {
            return return_type{std::move (subtarget_info)};
        }
        return return_type{make_error_code (error_code::no_subtarget_info_for_target)};
    }

    auto create_instruction_info (llvm::Target const & target) {
        using return_type = error_or<instr_info_ptr>;
        if (instr_info_ptr instruction_info{target.createMCInstrInfo ()}) {
            return return_type{std::move (instruction_info)};
        }
        return return_type{make_error_code (error_code::no_instruction_info_for_target)};
    }

    auto create_disassembler (llvm::Target const & target,
                              llvm::MCSubtargetInfo const & subtarget_info,
                              llvm::MCContext & context) {
        using return_type = error_or<disasm_ptr>;
        if (disasm_ptr disassembler{target.createMCDisassembler (subtarget_info, context)}) {
            return return_type{std::move (disassembler)};
        }
        return return_type{make_error_code (error_code::no_disassembler_for_target)};
    }

    auto create_inst_printer (llvm::MCContext const & context, llvm::Triple const & triple,
                              llvm::Target const & target, llvm::MCInstrInfo const & instructions) {
        using return_type = error_or<inst_printer_ptr>;

        llvm::MCAsmInfo const * const asm_info = context.getAsmInfo ();
        PSTORE_ASSERT (asm_info != nullptr);
        if (inst_printer_ptr instruction_printer{
                target.createMCInstPrinter (triple, asm_info->getAssemblerDialect (), *asm_info,
                                            instructions, *context.getRegisterInfo ())}) {
            return return_type{std::move (instruction_printer)};
        }
        return return_type{make_error_code (error_code::no_instruction_printer)};
    }

    auto create_asm_streamer (llvm::MCContext & context,
                              std::unique_ptr<llvm::formatted_raw_ostream> && formatted_os,
                              llvm::MCInstPrinter * const instruction_printer) {
        using return_type = error_or<streamer_ptr>;

        if (streamer_ptr streamer{llvm::createAsmStreamer (
                context, std::move (formatted_os), false /*is verbose asm*/,
                false /*use dwarf directory*/, instruction_printer,
                std::unique_ptr<llvm::MCCodeEmitter>{}, std::unique_ptr<llvm::MCAsmBackend>{},
                true /*showInst*/)}) {
            streamer->InitSections (false /*NoExecStack*/);
            return return_type{std::move (streamer)};
        }
        return return_type{make_error_code (error_code::cannot_create_asm_streamer)};
    }

    struct disassembler_state {
        llvm::MCContext & context;
        disasm_ptr const & disassembler;
        subtarget_ptr const & subtarget_info;
        inst_printer_ptr & inst_printer;
    };

    // emit_instructions
    // ~~~~~~~~~~~~~~~~~
    error_or<bool> emit_instructions (disassembler_state & state,
                                      pstore::gsl::span<std::uint8_t const> const & span,
                                      pstore::dump::array::container & array, bool hex_mode) {
        auto out = std::make_unique<array_stream> (array);
        auto os = std::make_unique<llvm::formatted_raw_ostream> (*out);
        auto osp = os.get ();

        auto const emit = [&] (streamer_ptr & streamer) {
            address_printer ap{span, hex_mode};

            auto address = std::uint64_t{0};
            auto offset = std::ptrdiff_t{0};
            for (auto sub = span.subspan (offset); !sub.empty (); sub = span.subspan (offset)) {
                (*osp) << ap.make_value (address) << ':';

                llvm::MCInst instruction;
                std::uint64_t bytes_consumed = 0;
                switch (state.disassembler->getInstruction (
                    instruction,    // out: an MCInst to populate
                    bytes_consumed, // out: number of bytes consumed
                    llvm::ArrayRef<std::uint8_t> (sub.data (),
                                                  sub.size ()), // actual bytes of the instruction
                    address,        // address of the first byte of the instruction
                    llvm::nulls ()) // stream on which to print comments and annotations)
                ) {
                case llvm::MCDisassembler::Fail:
                    (*osp) << "invalid instruction\n";
                    if (bytes_consumed == 0) {
                        bytes_consumed = 1; // skip illegible bytes
                    }
                    break;
                case llvm::MCDisassembler::SoftFail:
                    (*osp) << "potentially undefined instruction:";
                    LLVM_FALLTHROUGH;
                case llvm::MCDisassembler::Success:
                    streamer->emitInstruction (instruction, *state.subtarget_info);
                    break;
                }
                address += bytes_consumed;
                offset += bytes_consumed;
            }
            return error_or<bool>{true};
        };

        state.inst_printer->setPrintImmHex (hex_mode);
        return create_asm_streamer (state.context, std::move (os),
                                    state.inst_printer.release ()) >>= emit;
    }

    // disasm_block
    // ~~~~~~~~~~~~
    std::error_code disasm_block (pstore::gsl::span<std::uint8_t const> const & span,
                                  pstore::dump::array::container & output, // out
                                  llvm::Triple const & triple, bool hex_mode) {
        if (span.size () <= 0) {
            return {};
        }

        // clang-format off
        return (create_target (triple) >>= [&] (llvm::Target const * const target) {
        return create_register_info (*target, triple) >>= [&] (register_ptr const & registers) {
        return create_asm_info (*target, *registers, triple) >>= [&] (asm_info_ptr const & asm_info) {
        return create_subtarget_info (*target, triple) >>= [&] (subtarget_ptr const & subtarget) {
        return create_instruction_info (*target) >>= [&] (instr_info_ptr const & instr) {
        context context{triple, asm_info.get (), registers.get ()};
        return create_disassembler (*target, *subtarget, context.get ()) >>= [&] (disasm_ptr const & disasm) {
        return create_inst_printer (context.get (), triple, *target, *instr) >>= [&] (inst_printer_ptr & ip) {
            disassembler_state state{context.get (), disasm, subtarget, ip};
            return emit_instructions (state, span, output, hex_mode);
        }; }; }; }; }; }; }).get_error ();
        // clang-format on
    }

} // end anonymous namespace

#endif // PSTORE_IS_INSIDE_LLVM

namespace {

    pstore::dump::value_ptr make_hex_dump_value (std::uint8_t const * first,
                                                 std::uint8_t const * last, bool const hex_mode) {
        if (hex_mode) {
            return std::make_shared<pstore::dump::binary16> (first, last);
        }
        return std::make_shared<pstore::dump::binary> (first, last);
    }

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM

namespace pstore {
    namespace dump {

        value_ptr make_disassembled_value (std::uint8_t const * const first,
                                           std::uint8_t const * const last,
                                           gsl::czstring const triple, bool const hex_mode) {
            array::container arr;

            if (!disasm_block (gsl::make_span (first, last), arr, llvm::Triple{triple}, hex_mode)) {
                return make_value (arr);
            }
            return make_hex_dump_value (first, last, hex_mode);
        }

    } // end namespace dump
} // end namespace pstore

#else // PSTORE_IS_INSIDE_LLVM

namespace pstore {
    namespace dump {

        value_ptr make_disassembled_value (std::uint8_t const * const first,
                                           std::uint8_t const * const last,
                                           gsl::czstring const /*triple*/, bool const hex_mode) {
            return make_hex_dump_value (first, last, hex_mode);
        }

    } // end namespace dump
} // end namespace pstore

#endif // PSTORE_IS_INSIDE_LLVM
