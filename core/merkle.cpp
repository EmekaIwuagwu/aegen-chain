#include "merkle.h"
#include "util/crypto.h"
#include <sstream>

namespace aegen {

Hash MerkleTree::computeRoot(const std::vector<Hash>& leaves) {
    if (leaves.empty()) return Hash{};
    if (leaves.size() == 1) return leaves[0];

    // Naive implementation: just hash everything together for now
    // A real Merkle Tree pairs them up recursively.
    // For Phase 1 prototype, a "concatenation hash" is sufficient to prove binding.
    std::vector<uint8_t> buffer;
    for (const auto& hash : leaves) {
        buffer.insert(buffer.end(), hash.begin(), hash.end());
    }
    return crypto::sha256_bytes(buffer);
}

std::vector<Hash> MerkleTree::computeProof(const std::vector<Hash>& leaves, size_t index) {
    return {}; // TODO
}

bool MerkleTree::verifyProof(const Hash& root, const Hash& leaf, const std::vector<Hash>& proof, size_t index) {
    return false; // TODO
}

}
