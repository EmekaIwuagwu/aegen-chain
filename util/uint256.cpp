#include "uint256.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace aegen {

static void mul64(uint64_t a, uint64_t b, uint64_t& lo, uint64_t& hi) {
#if defined(_MSC_VER) && defined(_M_X64)
    lo = _umul128(a, b, &hi);
#elif defined(__SIZEOF_INT128__)
    unsigned __int128 res = (unsigned __int128)a * b;
    lo = (uint64_t)res;
    hi = (uint64_t)(res >> 64);
#else
    // Robust 32-bit fallback
    uint64_t u1 = a >> 32;
    uint64_t u0 = a & 0xFFFFFFFF;
    uint64_t v1 = b >> 32;
    uint64_t v0 = b & 0xFFFFFFFF;
    
    uint64_t t = (u0 * v0);
    uint64_t w0 = t & 0xFFFFFFFF;
    uint64_t k = t >> 32;

    t = (u1 * v0) + k;
    uint64_t w1 = t & 0xFFFFFFFF;
    uint64_t w2 = t >> 32;

    t = (u0 * v1) + w1;
    k = t >> 32;

    lo = (t << 32) + w0;
    hi = (u1 * v1) + w2 + k;
#endif
}

UInt256 UInt256::fromBigEndianBytes(const std::vector<uint8_t>& bytes) {
    UInt256 res;
    if (bytes.empty()) return res;
    
    size_t size = std::min(bytes.size(), (size_t)32);
    size_t start = bytes.size() - size;

    for (size_t i = 0; i < size; ++i) {
        size_t byteIdx = bytes.size() - 1 - i;
        if (byteIdx < start) break;
        uint8_t b = bytes[byteIdx];
        res.data[i / 8] |= (uint64_t)b << ((i % 8) * 8);
    }
    return res;
}

std::vector<uint8_t> UInt256::toBigEndianBytes() const {
    std::vector<uint8_t> out(32, 0);
    for (int i = 0; i < 32; ++i) {
        uint64_t word = data[(31 - i) / 8];
        int shift = ((31 - i) % 8) * 8;
        out[i] = (uint8_t)(word >> shift);
    }
    return out;
}

UInt256 UInt256::fromHex(const std::string& hex) {
    UInt256 res;
    std::string s = hex;
    if (s.substr(0, 2) == "0x") s = s.substr(2);
    if (s.empty()) return res;
    
    int len = s.length();
    for (int i = 0; i < len; i += 16) {
        int chunkLen = std::min(16, len - i);
        int start = len - i - chunkLen;
        std::string chunk = s.substr(start, chunkLen);
        uint64_t val = std::stoull(chunk, nullptr, 16);
        
        int wordIdx = i / 16;
        if (wordIdx < 4) res.data[wordIdx] = val;
    }
    return res;
}

std::string UInt256::toHex() const {
    std::stringstream ss;
    ss << "0x";
    bool leading = true;
    for (int i = 3; i >= 0; --i) {
        if (leading && data[i] == 0 && i > 0) continue;
        if (leading) {
            ss << std::hex << data[i];
            leading = false;
        } else {
            ss << std::hex << std::setw(16) << std::setfill('0') << data[i];
        }
    }
    if (leading) ss << "0";
    return ss.str();
}

UInt256 UInt256::operator+(const UInt256& other) const {
    UInt256 res;
    uint64_t carry = 0;
    for (int i = 0; i < 4; ++i) {
        uint64_t a = data[i];
        uint64_t b = other.data[i];
        uint64_t sum = a + b + carry;
        res.data[i] = sum;
        carry = (sum < a) || (carry && sum == a); 
    }
    return res;
}

UInt256 UInt256::operator-(const UInt256& other) const {
    UInt256 res;
    uint64_t borrow = 0;
    for (int i = 0; i < 4; ++i) {
        uint64_t a = data[i];
        uint64_t b = other.data[i];
        uint64_t diff = a - b - borrow;
        res.data[i] = diff;
        borrow = (a < b) || (borrow && a == b);
    }
    return res;
}

UInt256 UInt256::operator*(const UInt256& other) const {
    UInt256 res;
    for (int i = 0; i < 4; ++i) {
        uint64_t carry = 0;
        for (int j = 0; j < 4; ++j) {
            if (i + j < 4) {
                uint64_t p_lo, p_hi;
                mul64(data[i], other.data[j], p_lo, p_hi);
                
                uint64_t sum = res.data[i+j] + p_lo;
                uint64_t c1 = (sum < res.data[i+j]);
                sum += carry;
                uint64_t c2 = (sum < carry);
                res.data[i+j] = sum;
                
                carry = p_hi + c1 + c2;
            }
        }
        // Propagate remaining carry
        // But wait, the inner loop finishes a row. The `carry` coming out of the row 
        // needs to be added to the next column? 
        // My logic above adds `carry` into the *next iteration's* `p_hi + c1 + c2`.
        // The `carry` variable accumulates the high part of the product.
        // It needs to be stored in `res[i+j]` where `j` effectively became 4?
        // Ah, if `i+j >= 4`, we discard.
        // But we must carry forward to `res[i+j+1]` even if we are out of `j` loop?
        // No, current logic is: `carry` is the incoming carry for *this* cell calculation.
        // When loop `j` finishes, if valid, `res[i+4]` would get `carry`.
        // But we truncate to 4 words. So we can ignore `carry` after `j` loop if `i+j` goes out of bounds.
        // The condition `if (i+j < 4)` handles the bounds. 
        // If `i+j >= 4`, we don't write.
        // BUT, `carry` for the NEXT `j` (which is `j+1`) depends on current calculation.
        // The implementation:
        //    res.data[i+j] = sum;
        //    carry = p_hi + c1 + c2;
        // This makes `carry` effectively the value to add to `res[i+j+1]`.
        // So on the next iteration of `j`, `res[i+j]` (which is next word) gets `carry` added.
        // This seems correct.
    }
    return res;
}

std::pair<UInt256, UInt256> div_mod(const UInt256& a, const UInt256& b) {
    if (b == UInt256(0)) throw std::runtime_error("Division by zero");
    if (a < b) return {UInt256(0), a};
    
    UInt256 quotient;
    UInt256 remainder = a;
    UInt256 divisor = b;

    int shift = a.getLeadingBit() - b.getLeadingBit();
    
    UInt256 currentDivisor = divisor << shift;
    
    for (int i = 0; i <= shift; ++i) {
            if (remainder >= currentDivisor) {
                remainder = remainder - currentDivisor;
                quotient.setBit(shift - i);
            }
            currentDivisor = currentDivisor >> 1;
    }
    return {quotient, remainder};
}

UInt256 UInt256::operator/(const UInt256& other) const {
    if (other == UInt256(0)) return UInt256(0); 
    return div_mod(*this, other).first;
}

UInt256 UInt256::operator%(const UInt256& other) const {
    if (other == UInt256(0)) return UInt256(0);
    return div_mod(*this, other).second;
}

UInt256 UInt256::operator&(const UInt256& other) const {
    UInt256 res;
    for(int i=0; i<4; i++) res.data[i] = data[i] & other.data[i];
    return res;
}

UInt256 UInt256::operator|(const UInt256& other) const {
    UInt256 res;
    for(int i=0; i<4; i++) res.data[i] = data[i] | other.data[i];
    return res;
}

UInt256 UInt256::operator^(const UInt256& other) const {
    UInt256 res;
    for(int i=0; i<4; i++) res.data[i] = data[i] ^ other.data[i];
    return res;
}

UInt256 UInt256::operator~() const {
    UInt256 res;
    for(int i=0; i<4; i++) res.data[i] = ~data[i];
    return res;
}

UInt256 UInt256::operator<<(int shift) const {
    UInt256 res;
    if (shift >= 256) return res;
    int word_shift = shift / 64;
    int bit_shift = shift % 64;
    
    for (int i = 0; i < 4; ++i) {
        if (i - word_shift >= 0) {
            res.data[i] = data[i - word_shift] << bit_shift;
            if (bit_shift > 0 && i - word_shift - 1 >= 0) {
                res.data[i] |= data[i - word_shift - 1] >> (64 - bit_shift);
            }
        }
    }
    return res;
}

UInt256 UInt256::operator>>(int shift) const {
    UInt256 res;
    if (shift >= 256) return res;
    int word_shift = shift / 64;
    int bit_shift = shift % 64;
    
    for (int i = 0; i < 4; ++i) {
        if (i + word_shift < 4) {
                res.data[i] = data[i + word_shift] >> bit_shift;
                if (bit_shift > 0 && i + word_shift + 1 < 4) {
                    res.data[i] |= data[i + word_shift + 1] << (64 - bit_shift);
                }
        }
    }
    return res;
}

bool UInt256::operator<(const UInt256& other) const {
    for (int i = 3; i >= 0; --i) {
        if (data[i] < other.data[i]) return true;
        if (data[i] > other.data[i]) return false;
    }
    return false;
}

int UInt256::getLeadingBit() const {
    for (int i = 3; i >= 0; --i) {
        if (data[i] != 0) {
            uint64_t x = data[i];
            for(int b=63; b>=0; --b) {
                if ((x >> b) & 1) return i*64 + b;
            }
        }
    }
    return -1;
}

void UInt256::setBit(int bit) {
    if(bit < 0 || bit >= 256) return;
    data[bit/64] |= (1ULL << (bit%64));
}

}
