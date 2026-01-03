#include "address.h"

namespace aegen {

Address AddressUtils::deriveFromPublicKey(const PublicKey& pk) {
    // TODO: Implement address derivation (e.g. SHA256(pk) -> Hex)
    return "";
}

bool AddressUtils::isValid(const Address& addr) {
    // TODO: Verify checksum/format
    return !addr.empty();
}

}
