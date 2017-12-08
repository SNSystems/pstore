#include "pstore/sstring_view_archive.hpp"


namespace pstore {
    namespace serialize {

        std::size_t read_string_length (archive::database_reader & archive) {
            using namespace pstore;

            std::array<std::uint8_t, varint::max_output_length> encoded_length{{0}};
            // First read the two initial bytes. These contain the variable length value
            // but might not be enough for the entire value.
            static_assert (varint::max_output_length >= 2,
                           "maximum encoded varint length must be >= 2");
            serialize::read_uninit (archive, ::pstore::gsl::make_span (encoded_length.data (), 2));

            auto const varint_length = varint::decode_size (std::begin (encoded_length));
            assert (varint_length > 0);
            // Was that initial read of 2 bytes enough? If not get the rest of the
            // length value.
            if (varint_length > 2) {
                assert (varint_length <= encoded_length.size ());
                serialize::read_uninit (
                    archive,
                    ::pstore::gsl::make_span (encoded_length.data () + 2, varint_length - 2));
            }

            return varint::decode (encoded_length.data (), varint_length);
        }

    } // namespace serialize
} // namespace pstore
