#pragma once
#include "core/types.h"
#include "token_manager.h"
#include <vector>
#include <map>

namespace aegen {

class TokenRegistry {
private:
    std::map<TokenId, TokenInfo> registry;

public:
    void registerToken(const TokenId& id, const TokenInfo& info);
    bool exists(const TokenId& id);
    std::vector<TokenId> listTokens();
    TokenInfo* getToken(const TokenId& id);
};

}
