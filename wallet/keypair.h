#pragma once
#include "core/types.h"

namespace aegen {

struct KeyPair {
    PublicKey publicKey;
    PrivateKey privateKey;
    Address address;
};

class Wallet {
public:
    static KeyPair generateKeyPair();
    static Address deriveAddress(const PublicKey& pubKey);
    static bool validateAddress(const Address& addr);
};

}
