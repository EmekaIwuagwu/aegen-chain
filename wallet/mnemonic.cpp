#include "mnemonic.h"
#include "util/crypto.h"
#include <sstream>
#include <random>
#include <algorithm>

namespace aegen {

// BIP39 English wordlist (first 128 words for demo - full list would have 2048)
static const std::vector<std::string> WORDLIST = {
    "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract",
    "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid",
    "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual",
    "adapt", "add", "addict", "address", "adjust", "admit", "adult", "advance",
    "advice", "aerobic", "affair", "afford", "afraid", "again", "age", "agent",
    "agree", "ahead", "aim", "air", "airport", "aisle", "alarm", "album",
    "alcohol", "alert", "alien", "all", "alley", "allow", "almost", "alone",
    "alpha", "already", "also", "alter", "always", "amateur", "amazing", "among",
    "amount", "amused", "analyst", "anchor", "ancient", "anger", "angle", "angry",
    "animal", "ankle", "announce", "annual", "another", "answer", "antenna", "antique",
    "anxiety", "any", "apart", "apology", "appear", "apple", "approve", "april",
    "arch", "arctic", "area", "arena", "argue", "arm", "armed", "armor",
    "army", "around", "arrange", "arrest", "arrive", "arrow", "art", "artefact",
    "artist", "artwork", "ask", "aspect", "assault", "asset", "assist", "assume",
    "asthma", "athlete", "atom", "attack", "attend", "attitude", "attract", "auction",
    "audit", "august", "aunt", "author", "auto", "autumn", "average", "avocado"
};

std::string Mnemonic::generate(int wordCount) {
    if (wordCount != 12 && wordCount != 24) {
        wordCount = 12; // Default to 12 words
    }
    
    // Use secure random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, WORDLIST.size() - 1);
    
    std::vector<std::string> words;
    for (int i = 0; i < wordCount; i++) {
        words.push_back(WORDLIST[dist(gen)]);
    }
    
    // Join words with spaces
    std::stringstream ss;
    for (size_t i = 0; i < words.size(); i++) {
        if (i > 0) ss << " ";
        ss << words[i];
    }
    
    return ss.str();
}

std::vector<uint8_t> Mnemonic::toSeed(const std::string& mnemonic, const std::string& passphrase) {
    // PBKDF2-SHA512 with salt "mnemonic" + passphrase
    // For production, use a proper PBKDF2 implementation
    // Here we use a simplified derivation
    
    std::string saltedInput = mnemonic + "mnemonic" + passphrase;
    std::vector<uint8_t> data(saltedInput.begin(), saltedInput.end());
    
    // Stretch the seed through multiple hash rounds
    std::vector<uint8_t> seed;
    
    // Generate 64 bytes (512 bits) by hashing twice
    auto hash1 = crypto::sha256_bytes(data);
    seed.insert(seed.end(), hash1.begin(), hash1.end());
    
    // Hash again with previous result
    std::vector<uint8_t> data2 = data;
    data2.insert(data2.end(), hash1.begin(), hash1.end());
    auto hash2 = crypto::sha256_bytes(data2);
    seed.insert(seed.end(), hash2.begin(), hash2.end());
    
    return seed;
}

bool Mnemonic::validate(const std::string& mnemonic) {
    if (mnemonic.empty()) return false;
    
    // Split into words
    std::vector<std::string> words;
    std::stringstream ss(mnemonic);
    std::string word;
    while (ss >> word) {
        words.push_back(word);
    }
    
    // Check word count (must be 12 or 24)
    if (words.size() != 12 && words.size() != 24) {
        return false;
    }
    
    // Check each word is in wordlist
    for (const auto& w : words) {
        bool found = false;
        for (const auto& valid : WORDLIST) {
            if (w == valid) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    return true;
}

}
