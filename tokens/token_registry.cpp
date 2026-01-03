#include "token_registry.h"

namespace aegen {

void TokenRegistry::registerToken(const TokenId& id, const TokenInfo& info) {
    registry[id] = info;
}

bool TokenRegistry::exists(const TokenId& id) {
    return registry.find(id) != registry.end();
}

std::vector<TokenId> TokenRegistry::listTokens() {
    std::vector<TokenId> ids;
    for (const auto& [id, info] : registry) {
        ids.push_back(id);
    }
    return ids;
}

TokenInfo* TokenRegistry::getToken(const TokenId& id) {
    auto it = registry.find(id);
    if (it != registry.end()) {
        return &it->second;
    }
    return nullptr;
}

}
