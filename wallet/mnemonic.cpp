#include "mnemonic.h"

namespace aegen {

std::string Mnemonic::generate(int wordCount) {
    // TODO: secure random + wordlist
    return "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
}

std::vector<uint8_t> Mnemonic::toSeed(const std::string& mnemonic, const std::string& passphrase) {
    // TODO: PBKDF2
    return {};
}

bool Mnemonic::validate(const std::string& mnemonic) {
    // TODO: Check checksum
    return true;
}

}
