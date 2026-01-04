#pragma once
#include <vector>
#include <string>
#include <array>
#include "util/uint256.h"

namespace aegen {

// Structured ZK Proof types based on Groth16
// We define the data structures required for a real pairing-based proof system.
// In a full implementation, these would map to elliptic curve points (G1, G2).

struct G1Point {
    UInt256 x;
    UInt256 y;
    
    // Check if point is on curve (simulated check for now)
    bool isValid() const { return true; } 
    
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out;
        auto xb = x.toBigEndianBytes();
        auto yb = y.toBigEndianBytes();
        out.insert(out.end(), xb.begin(), xb.end());
        out.insert(out.end(), yb.begin(), yb.end());
        return out;
    }
};

struct G2Point {
    // G2 points are over extension field Fp2, so they have 2 coordinates each being a pair (c0, c1)
    UInt256 x0, x1;
    UInt256 y0, y1;
    
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out;
        auto x0b = x0.toBigEndianBytes();
        auto x1b = x1.toBigEndianBytes();
        auto y0b = y0.toBigEndianBytes();
        auto y1b = y1.toBigEndianBytes();
        out.insert(out.end(), x0b.begin(), x0b.end());
        out.insert(out.end(), x1b.begin(), x1b.end());
        out.insert(out.end(), y0b.begin(), y0b.end());
        out.insert(out.end(), y1b.begin(), y1b.end());
        return out;
    }
};

struct Groth16Proof {
    G1Point a;
    G2Point b;
    G1Point c;
};

struct VerificationKey {
    G1Point alpha;
    G2Point beta;
    G2Point gamma;
    G2Point delta;
    std::vector<G1Point> gamma_abc; // IC
};

class ZKVerifier {
public:
    // Verify a Groth16 proof
    // Returns true if the pairing equation holds:
    // e(A, B) = e(alpha, beta) * e(IC, gamma) * e(C, delta)
    static bool verifyGroth16(
        const VerificationKey& vk, 
        const Groth16Proof& proof, 
        const std::vector<UInt256>& publicInputs
    );
};

}
