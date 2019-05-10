#include <cstdlib>
#include <string>
#include <system_error>

namespace std {

    // error_category::error_category () noexcept = default;

    error_category::~error_category () = default;

    error_condition error_category::default_error_condition (int ev) const noexcept {
        return error_condition (ev, *this);
    }


    bool error_category::equivalent (int code, error_condition const & condition) const noexcept {
        return default_error_condition (code) == condition;
    }
    bool error_category::equivalent (error_code const & code, int condition) const noexcept {
        return *this == code.category () && code.value () == condition;
    }

    // template <typename T>
    // std::allocator<T>::allocator() noexcept = default;

    template class allocator<char>;
    template class basic_string<char>;

    void terminate () noexcept { std::exit (EXIT_FAILURE); }

} // namespace std
