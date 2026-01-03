#pragma once
#include "core/types.h"
#include "token_manager.h"

namespace aegen {

class TokenTransfer {
public:
    static bool transfer(const TokenId& token, const Address& from, const Address& to, uint64_t amount);
    static bool transferFrom(const TokenId& token, const Address& from, const Address& to, uint64_t amount, const Address& spender);
};

}
