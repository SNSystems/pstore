//*      _                              *
//*  ___| |_ ___  _ __ __ _  __ _  ___  *
//* / __| __/ _ \| '__/ _` |/ _` |/ _ \ *
//* \__ \ || (_) | | | (_| | (_| |  __/ *
//* |___/\__\___/|_|  \__,_|\__, |\___| *
//*                         |___/       *
//===- lib/core/storage.cpp -----------------------------------------------===//
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
/// \file storage.cpp

#include "pstore/core/storage.hpp"
#include "pstore/core/file_header.hpp"

namespace {

    ///@{
    /// Rounds the value 'v' down the next lowest multiple of 'boundary'.
    /// The result is equivalent to a function such as:
    ///     round_down x b = x - x `mod` b

    /// \param x  The value to be rounded.
    /// \param b  The boundary to which 'x' is to be rounded. It must be a power of 2.
    /// \return  The value x2 such as x2 <= x and x2 `mod` b == 0
    std::uint64_t round_down (std::uint64_t x, std::uint64_t b) {
        assert (pstore::is_power_of_two (b));
        std::uint64_t const r = x & ~(b - 1U);
        assert (r <= x && r % b == 0);
        return r;
    }

    /// \param x  The value to be rounded.
    /// \param b  The boundary to which 'x' is to be rounded. It must be a power of 2.
    /// \return  The value x2 such as x2 <= x and x2 `mod` b == 0
    pstore::address round_down (pstore::address x, std::uint64_t b) {
        return {round_down (x.absolute (), b)};
    }
    ///@}

} // end anonymous namespace

namespace pstore {

    constexpr std::uint64_t storage::full_region_size;
    constexpr std::uint64_t storage::min_region_size;

    // map_bytes
    // ~~~~~~~~~
    void storage::map_bytes (std::uint64_t new_size) {
        // Get the file offset of the end of the last memory mapped region.
        std::uint64_t const old_size = regions_.size () == 0 ? 0 : regions_.back ()->end ();
        if (new_size > old_size) {
            auto const old_num_regions = regions_.size ();
            // Allocate new memory region(s) to accommodate the additional bytes requested.
            region_factory_->add (&regions_, old_size, new_size);
            this->update_master_pointers (old_num_regions);
        }
    }

    // update_master_pointers
    // ~~~~~~~~~~~~~~~~~~~~~~
    void storage::update_master_pointers (std::size_t old_length) {

        std::uint64_t last_sat_entry = 0;
        if (old_length > 0) {
            assert (old_length < regions_.size ());
            region::memory_mapper_ptr const & region = regions_[old_length - 1];
            last_sat_entry = (region->offset () + region->size ()) / address::segment_size;
            assert (sat_->at (last_sat_entry - 1).value != nullptr);
        }

        auto segment_it = std::begin (*sat_);
        auto segment_end = std::end (*sat_);
        using segment_difference_type =
            std::iterator_traits<decltype (segment_it)>::difference_type;

        assert (last_sat_entry <= static_cast<std::make_unsigned<segment_difference_type>::type> (
                                      std::numeric_limits<segment_difference_type>::max ()));
        std::advance (segment_it, static_cast<segment_difference_type> (last_sat_entry));

        auto region_it = std::begin (regions_);
        auto region_end = std::end (regions_);

        using region_difference_type = std::iterator_traits<decltype (region_it)>::difference_type;
        assert (old_length <= static_cast<std::make_unsigned<region_difference_type>::type> (
                                  std::numeric_limits<region_difference_type>::max ()));
        std::advance (region_it, static_cast<region_difference_type> (old_length));

        for (; region_it != region_end; ++region_it) {
            segment_it = storage::slice_region_into_segments (*region_it, segment_it, segment_end);
        }

#ifndef NDEBUG
        // Guarantee that segments beyond the mapped memory blocks are null.
        for (; segment_it != segment_end; ++segment_it) {
            assert (segment_it->value == nullptr && segment_it->region == nullptr);
        }
#endif
    }

    // slice_region_into_segments
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto storage::slice_region_into_segments (std::shared_ptr<memory_mapper_base> region,
                                              sat_iterator segment_it, sat_iterator segment_end)
        -> sat_iterator {

        (void) segment_end; // silence unused argument warning in release build.
        std::shared_ptr<void> data = region->data ();
        for (auto ptr = std::static_pointer_cast<std::uint8_t> (data).get (),
                  end = ptr + region->size ();
             ptr < end; ptr += address::segment_size) {

            assert (segment_it != segment_end);
            sat_entry & segment = *segment_it;
            assert (segment.value == nullptr && segment.region == nullptr);

            // The segment's memory (at 'ptr') is managed by the 'data' shared_ptr.
            segment.value = std::shared_ptr<void> (data, ptr);
            segment.region = region;

            ++segment_it;
        }
        return segment_it;
    }

    // request_spans_regions
    // ~~~~~~~~~~~~~~~~~~~~~
    bool storage::request_spans_regions (address const & addr, std::size_t size) const {
#if PSTORE_ALWAYS_SPANNING
        return true;
#else
        bool resl = false;
        if (addr.offset () + size > UINT64_C (1) << address::offset_number_bits) {
            address::segment_type const segment = addr.segment ();
            // FIXME: might be max segment number!
            resl = (*sat_)[segment].region != (*sat_)[segment + 1].region;
        }
        return resl;
#endif
    }

    // protect
    // ~~~~~~~
    void storage::protect (address first, address last) {

        std::uint64_t const page_size = memory_mapper::page_size (*page_size_);
        assert (page_size > 0 && is_power_of_two (page_size));

        first = std::max (round_down (first, page_size),
                          address::make (round_down (sizeof (header) + page_size - 1U, page_size)));
        last = round_down (last, page_size);

        for (auto region_it = regions_.rbegin (), end = regions_.rend (); region_it != end;
             ++region_it) {

            std::shared_ptr<memory_mapper_base> & region = *region_it;

            assert (region->offset () % page_size == 0);
            std::uint64_t const first_offset = std::max (region->offset (), first.absolute ());
            std::uint64_t const last_offset =
                std::min (region->offset () + region->size (), last.absolute ());

            if (region->offset () + region->size () < first_offset) {
                break;
            }

            if (first_offset >= first.absolute () && last_offset > first_offset) {
                auto pfirst = std::static_pointer_cast<std::uint8_t> (
                    this->address_to_pointer (address::make (first_offset)));

                assert (pfirst >= region->data ());
                assert (last_offset - region->offset () <= region->size ());

                region->read_only (pfirst.get (), last_offset - first_offset);
            }
        }
    }

} // namespace pstore
// eof: lib/core/storage.cpp
