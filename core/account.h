#pragma once
#include "types.h"

namespace aegen {

struct AccountState {
    uint64_t nonce;
    uint64_t balance;
    // can add storage root for contracts later
};

// Simple in-memory representation for now
class Account {
public:
    Address address;
    AccountState state;
};

}
