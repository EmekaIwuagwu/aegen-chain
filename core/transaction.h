#pragma once
#include "types.h"

namespace aegen {

struct Transaction {
    Address sender;
    Address receiver;
    uint64_t amount = 0;
    uint64_t nonce = 0;
    uint64_t gasLimit = 21000;
    uint64_t gasPrice = 1;
    Bytes data;
    Signature signature;
    Hash hash;

    static Transaction deserialize(const Bytes& data);
    Bytes serialize() const;
    void calculateHash();
    bool isSignedBy(const PublicKey& pk) const;
};

}
