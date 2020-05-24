//
//  digest_from_string.hpp
//  gmock
//
//  Created by Paul Bowen-Huggett on 22/05/2020.
//

#ifndef digest_from_string_hpp
#define digest_from_string_hpp

#include <string>
#include "pstore/support/maybe.hpp"
#include "pstore/support/uint128.hpp"

pstore::maybe<pstore::uint128> digest_from_string (std::string const & str);

#endif /* digest_from_string_hpp */
