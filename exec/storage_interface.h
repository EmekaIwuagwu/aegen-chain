#pragma once
#include "util/uint256.h"

namespace aegen {

// Interface for persistent storage backend
class StorageInterface {
public:
    virtual ~StorageInterface() = default;
    
    // contractAddr is expected to be part of the key prefix
    virtual void setStorage(const UInt256& contractAddr, const UInt256& key, const UInt256& value) = 0;
    virtual UInt256 getStorage(const UInt256& contractAddr, const UInt256& key) const = 0;
};

}
