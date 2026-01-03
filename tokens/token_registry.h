#pragma once
#include "core/types.h"
#include "token_manager.h"
#include <vector>

namespace aegen {

class TokenRegistry {
public:
    void registerToken(const TokenId& id, const TokenInfo& info);
    bool exists(const TokenId& id);
    std::vector<TokenId> listTokens();
};

}
