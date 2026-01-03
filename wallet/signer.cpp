#include "signer.h"
#include "util/crypto.h"

namespace aegen {

Signature Signer::sign(const Bytes& message, const PrivateKey& pk) {
    return crypto::sign_message(message, pk);
}

bool Signer::verify(const Bytes& message, const Signature& sig, const PublicKey& pk) {
    return crypto::verify_signature(message, sig, pk);
}

}
