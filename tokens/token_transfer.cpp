#include "token_transfer.h"

namespace aegen {

bool TokenTransfer::transfer(const TokenId& token, const Address& from, const Address& to, uint64_t amount) {
    // TODO: Check balance, update state
    return false;
}

bool TokenTransfer::transferFrom(const TokenId& token, const Address& from, const Address& to, uint64_t amount, const Address& spender) {
    // TODO: Check allowance, check balance, update state
    return false;
}

}
