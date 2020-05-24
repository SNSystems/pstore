#include "digest_from_string.hpp"

using namespace pstore;

// FIXME: copied from the dump-lib command-line handler.
namespace {

    maybe<unsigned> hex_to_digit (char const digit) noexcept {
        if (digit >= 'a' && digit <= 'f') {
            return just (static_cast<unsigned> (digit) - ('a' - 10));
        }
        if (digit >= 'A' && digit <= 'F') {
            return just (static_cast<unsigned> (digit) - ('A' - 10));
        }
        if (digit >= '0' && digit <= '9') {
            return just (static_cast<unsigned> (digit) - '0');
        }
        return nothing<unsigned> ();
    }

    maybe<std::uint64_t> get64 (std::string const & str, unsigned index) {
        assert (index < str.length ());
        auto result = std::uint64_t{0};
        for (auto shift = 60; shift >= 0; shift -= 4, ++index) {
            auto const digit = hex_to_digit (str[index]);
            if (!digit) {
                return nothing<std::uint64_t> ();
            }
            result |= (static_cast<std::uint64_t> (digit.value ()) << shift);
        }
        return just (result);
    }

} // end anonymous namespace

maybe<uint128> digest_from_string (std::string const & str) {
    if (str.length () != 32) {
        return nothing<uint128> ();
    }
    return get64 (str, 0U) >>= [&] (std::uint64_t const high) {
        return get64 (str, 16U) >>= [&] (std::uint64_t const low) {
            return just (uint128{high, low});
        };
    };
}
