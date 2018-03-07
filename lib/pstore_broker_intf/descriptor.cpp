#include "pstore_broker_intf/descriptor.hpp"

#if defined(_WIN32) && defined(_MSC_VER)

namespace pstore {
    namespace broker {
        namespace details {

            win32_pipe_descriptor_traits::type const win32_pipe_descriptor_traits::invalid =
                INVALID_HANDLE_VALUE;
            win32_pipe_descriptor_traits::type const win32_pipe_descriptor_traits::error =
                INVALID_HANDLE_VALUE;

        } // namespace details
    }     // namespace broker
} // namespace pstore

#endif
