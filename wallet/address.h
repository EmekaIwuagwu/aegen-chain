#pragma once
#include "types.h"
#include <string>

namespace aegen {

class AddressUtils {
public:
    static Address deriveFromPublicKey(const PublicKey& pk);
    static bool isValid(const Address& addr);
};

}
