#pragma once
#include <string>
#include <vector>

namespace aegen {

class Mnemonic {
public:
    static std::string generate(int wordCount = 12); // BIP39
    static std::vector<uint8_t> toSeed(const std::string& mnemonic, const std::string& passphrase = "");
    static bool validate(const std::string& mnemonic);
};

}
