#ifndef PSTORE_SUPPORT_ROUND2_HPP
#define PSTORE_SUPPORT_ROUND2_HPP

#include <cstdint>

namespace pstore {

    inline std::uint8_t round_to_power_of_2 (std::uint8_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        ++v;
        return v;
    }

    inline std::uint16_t round_to_power_of_2 (std::uint16_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        ++v;
        return v;
    }

    inline std::uint32_t round_to_power_of_2 (std::uint32_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        ++v;
        return v;
    }

    inline std::uint64_t round_to_power_of_2 (std::uint64_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        ++v;
        return v;
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_ROUND2_HPP
