#pragma once
#include "kadena_client.h"
#include "batch.h"

namespace aegen {

class SettlementBridge {
    KadenaClient& kadena;

public:
    SettlementBridge(KadenaClient& k);
    
    void settleBatch(const Batch& batch);
    std::string generatePactCmd(const Batch& batch);
};

}
