#include "transaction.h"
#include "util/crypto.h"
#include "wallet/signer.h"
#include <sstream>
#include <cstring>
#include <stdexcept>

namespace aegen {

Bytes Transaction::serialize() const {
    Bytes buffer;
    auto append = [&](const void* input, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(input);
        buffer.insert(buffer.end(), p, p + size);
    };
    auto appendStr = [&](const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        append(&len, sizeof(len));
        append(s.data(), len);
    };
    auto appendBytes = [&](const Bytes& b) {
        uint32_t len = (uint32_t)b.size();
        append(&len, sizeof(len));
        buffer.insert(buffer.end(), b.begin(), b.end());
    };

    appendStr(sender);
    appendStr(receiver);
    append(&amount, sizeof(amount));
    append(&nonce, sizeof(nonce));
    append(&gasLimit, sizeof(gasLimit));
    append(&gasPrice, sizeof(gasPrice));
    appendBytes(data);
    appendBytes(signature);
    
    return buffer;
}

Transaction Transaction::deserialize(const Bytes& data) {
    Transaction tx;
    size_t offset = 0;
    
    auto read = [&](void* dest, size_t size) {
        if (offset + size > data.size()) throw std::runtime_error("Tx deserialization OOB");
        std::memcpy(dest, &data[offset], size);
        offset += size;
    };
    auto readStr = [&](std::string& s) {
        uint32_t len;
        read(&len, sizeof(len));
        if (offset + len > data.size()) throw std::runtime_error("Tx deserialization OOB string");
        s.assign((char*)&data[offset], len);
        offset += len;
    };
    auto readBytes = [&](Bytes& b) {
        uint32_t len;
        read(&len, sizeof(len));
        if (offset + len > data.size()) throw std::runtime_error("Tx deserialization OOB bytes");
        b.assign(data.begin() + offset, data.begin() + offset + len);
        offset += len;
    };

    readStr(tx.sender);
    readStr(tx.receiver);
    read(&tx.amount, sizeof(tx.amount));
    read(&tx.nonce, sizeof(tx.nonce));
    read(&tx.gasLimit, sizeof(tx.gasLimit));
    read(&tx.gasPrice, sizeof(tx.gasPrice));
    readBytes(tx.data);
    readBytes(tx.signature);
    
    // Recalculate hash for object consistency
    tx.calculateHash();
    
    return tx;
}

void Transaction::calculateHash() {
    Bytes serialized = serialize();
    auto h = crypto::sha256_bytes(serialized);
    std::copy(h.begin(), h.end(), hash.begin());
}

bool Transaction::isSignedBy(const PublicKey& pk) const {
    Transaction txCopy = *this;
    txCopy.signature.clear();
    Bytes txData = txCopy.serialize();
    return Signer::verify(txData, signature, pk);
}

}
