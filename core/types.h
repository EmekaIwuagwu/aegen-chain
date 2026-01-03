#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace aegen {

using Byte = uint8_t;
using Bytes = std::vector<Byte>;
using Hash = std::array<Byte, 32>;
using PublicKey = std::vector<Byte>;  // 32 bytes for Ed25519
using PrivateKey = std::vector<Byte>; // 32 bytes for Ed25519 seed
using Signature = std::vector<Byte>;  // 64 bytes for Ed25519
using TokenId = std::string;

// Kadena-style address format:
// - Simple: "alice", "bob" (user-defined)
// - Single-key: "k:abc123..." (k: prefix + public key hex)
// - Multi-sig: "w:xyz789..." (w: prefix + keyset hash)
using Address = std::string;

// Kadena uses "guard" for access control (keysets)
struct KeySet {
    std::vector<PublicKey> keys;
    std::string pred; // "keys-all", "keys-any", "keys-2"
};

struct Amount {
    uint64_t value = 0;
    Amount() = default;
    Amount(uint64_t v) : value(v) {}
    operator uint64_t() const { return value; }
};

}
