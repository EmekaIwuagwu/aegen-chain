#include "keypair.h"
#include "util/crypto.h"

namespace aegen {

KeyPair Wallet::generateKeyPair() {
    KeyPair kp;
    kp.privateKey = crypto::generate_private_key();
    kp.publicKey = crypto::derive_public_key(kp.privateKey);
    kp.address = crypto::derive_kadena_address(kp.publicKey);
    return kp;
}

Address Wallet::deriveAddress(const PublicKey& pubKey) {
    return crypto::derive_kadena_address(pubKey);
}

bool Wallet::validateAddress(const Address& addr) {
    return crypto::validate_kadena_address(addr);
}

}
