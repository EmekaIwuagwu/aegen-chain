#pragma once
#include "types.h"
#include <vector>

namespace aegen {

class MerkleTree {
public:
    static Hash computeRoot(const std::vector<Hash>& leaves);
    static std::vector<Hash> computeProof(const std::vector<Hash>& leaves, size_t index);
    static bool verifyProof(const Hash& root, const Hash& leaf, const std::vector<Hash>& proof, size_t index);
};

}
