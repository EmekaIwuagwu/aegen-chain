#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <cstring>

namespace aegen {

// Production-Grade UInt256 implementation
// Provides full 256-bit arithmetic for EVM compatibility
class UInt256 {
public:
    // 4 x 64-bit words, Little Endian (data[0] is least significant)
    std::array<uint64_t, 4> data;

    UInt256() { data.fill(0); }
    UInt256(uint64_t v) { data.fill(0); data[0] = v; }
    
    // Construct from big-endian bytes (EVM standard)
    static UInt256 fromBigEndianBytes(const std::vector<uint8_t>& bytes);

    // Export to big-endian bytes
    std::vector<uint8_t> toBigEndianBytes() const;

    // Hex parsing
    static UInt256 fromHex(const std::string& hex);
    std::string toHex() const;

    // Arithmetic
    UInt256 operator+(const UInt256& other) const;
    UInt256 operator-(const UInt256& other) const;
    UInt256 operator*(const UInt256& other) const;
    UInt256 operator/(const UInt256& other) const;
    UInt256 operator%(const UInt256& other) const;

    // Bitwise
    UInt256 operator&(const UInt256& other) const;
    UInt256 operator|(const UInt256& other) const;
    UInt256 operator^(const UInt256& other) const;
    UInt256 operator~() const;
    UInt256 operator<<(int shift) const;
    UInt256 operator>>(int shift) const;

    // Comparison
    bool operator==(const UInt256& other) const { return data == other.data; }
    bool operator!=(const UInt256& other) const { return !(*this == other); }
    bool operator<(const UInt256& other) const;
    bool operator>(const UInt256& other) const { return other < *this; }
    bool operator<=(const UInt256& other) const { return !(*this > other); }
    bool operator>=(const UInt256& other) const { return !(*this < other); }

    // Helpers
    uint64_t toUint64() const { return data[0]; }
    // Get the index of the highest set bit (0-255), or -1 if zero
    int getLeadingBit() const;
    // Set a specific bit
    void setBit(int bit);
};

}
