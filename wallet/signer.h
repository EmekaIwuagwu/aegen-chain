#pragma once
#include "core/types.h"

namespace aegen {

// Forward declaration
struct Transaction;

class Signer {
public:
    static Signature sign(const Bytes& message, const PrivateKey& pk);
    static bool verify(const Bytes& message, const Signature& sig, const PublicKey& pk);
};

}
