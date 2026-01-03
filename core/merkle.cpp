#include "merkle.h"
#include "util/crypto.h"
#include <sstream>

namespace aegen {

Hash MerkleTree::computeRoot(const std::vector<Hash>& leaves) {
    if (leaves.empty()) return Hash{};
    if (leaves.size() == 1) return leaves[0];

    // Build proper Merkle tree by pairing leaves
    std::vector<Hash> level = leaves;
    
    while (level.size() > 1) {
        std::vector<Hash> nextLevel;
        
        for (size_t i = 0; i < level.size(); i += 2) {
            std::vector<uint8_t> combined;
            combined.insert(combined.end(), level[i].begin(), level[i].end());
            
            // If odd number of nodes, duplicate the last one
            if (i + 1 < level.size()) {
                combined.insert(combined.end(), level[i + 1].begin(), level[i + 1].end());
            } else {
                combined.insert(combined.end(), level[i].begin(), level[i].end());
            }
            
            nextLevel.push_back(crypto::sha256_bytes(combined));
        }
        
        level = nextLevel;
    }
    
    return level[0];
}

std::vector<Hash> MerkleTree::computeProof(const std::vector<Hash>& leaves, size_t index) {
    if (leaves.empty() || index >= leaves.size()) return {};
    
    std::vector<Hash> proof;
    std::vector<Hash> level = leaves;
    size_t idx = index;
    
    while (level.size() > 1) {
        std::vector<Hash> nextLevel;
        
        for (size_t i = 0; i < level.size(); i += 2) {
            // Determine sibling
            if (i == idx || i + 1 == idx) {
                // Add sibling to proof
                if (idx % 2 == 0) {
                    // Left node, sibling is right
                    if (idx + 1 < level.size()) {
                        proof.push_back(level[idx + 1]);
                    } else {
                        proof.push_back(level[idx]); // Duplicate if odd
                    }
                } else {
                    // Right node, sibling is left
                    proof.push_back(level[idx - 1]);
                }
            }
            
            std::vector<uint8_t> combined;
            combined.insert(combined.end(), level[i].begin(), level[i].end());
            
            if (i + 1 < level.size()) {
                combined.insert(combined.end(), level[i + 1].begin(), level[i + 1].end());
            } else {
                combined.insert(combined.end(), level[i].begin(), level[i].end());
            }
            
            nextLevel.push_back(crypto::sha256_bytes(combined));
        }
        
        idx = idx / 2;
        level = nextLevel;
    }
    
    return proof;
}

bool MerkleTree::verifyProof(const Hash& root, const Hash& leaf, const std::vector<Hash>& proof, size_t index) {
    if (proof.empty() && root == leaf) return true;
    
    Hash current = leaf;
    size_t idx = index;
    
    for (const auto& sibling : proof) {
        std::vector<uint8_t> combined;
        
        if (idx % 2 == 0) {
            // Current is left, sibling is right
            combined.insert(combined.end(), current.begin(), current.end());
            combined.insert(combined.end(), sibling.begin(), sibling.end());
        } else {
            // Current is right, sibling is left
            combined.insert(combined.end(), sibling.begin(), sibling.end());
            combined.insert(combined.end(), current.begin(), current.end());
        }
        
        current = crypto::sha256_bytes(combined);
        idx = idx / 2;
    }
    
    return current == root;
}

}
