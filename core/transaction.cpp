#include "transaction.h"
#include "util/crypto.h"
#include "wallet/signer.h"
#include <sstream>

namespace aegen {

Bytes Transaction::serialize() const {
    std::stringstream ss;
    ss << sender << receiver << amount << nonce << gasLimit << gasPrice;
    for (auto b : data) ss << (char)b;
    std::string s = ss.str();
    return Bytes(s.begin(), s.end());
}

void Transaction::calculateHash() {
    Bytes serialized = serialize();
    auto h = crypto::sha256_bytes(serialized);
    std::copy(h.begin(), h.end(), hash.begin());
}

bool Transaction::isSignedBy(const PublicKey& pk) const {
    Transaction txCopy = *this;
    txCopy.signature.clear();
    Bytes txData = txCopy.serialize();
    return Signer::verify(txData, signature, pk);
}

}
