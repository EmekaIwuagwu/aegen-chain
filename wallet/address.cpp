#include "address.h"
#include "util/crypto.h"

namespace aegen {

Address AddressUtils::deriveFromPublicKey(const PublicKey& pk) {
    // Kadena-style k: address format
    // Address = "k:" + hex(publicKey)
    if (pk.empty()) return "";
    
    // Convert public key to hex
    std::string hexPk = crypto::to_hex(pk);
    
    // Return as Kadena k: format
    return "k:" + hexPk;
}

bool AddressUtils::isValid(const Address& addr) {
    if (addr.empty()) return false;
    
    // Check for k: prefix (Kadena format)
    if (addr.length() > 2 && addr[0] == 'k' && addr[1] == ':') {
        std::string hexPart = addr.substr(2);
        
        // Verify it's valid hex and correct length (64 chars = 32 bytes)
        if (hexPart.length() != 64) return false;
        
        for (char c : hexPart) {
            if (!((c >= '0' && c <= '9') || 
                  (c >= 'a' && c <= 'f') || 
                  (c >= 'A' && c <= 'F'))) {
                return false;
            }
        }
        return true;
    }
    
    // Also allow simple account names (alice, bob, etc.)
    if (addr.length() < 64) {
        for (char c : addr) {
            if (!((c >= 'a' && c <= 'z') || 
                  (c >= 'A' && c <= 'Z') || 
                  (c >= '0' && c <= '9') ||
                  c == '-' || c == '_')) {
                return false;
            }
        }
        return true;
    }
    
    return false;
}

}
