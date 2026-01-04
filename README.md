<p align="center">
  <img src="https://img.shields.io/badge/Language-C++20-blue.svg" alt="C++20"/>
  <img src="https://img.shields.io/badge/L1-Kadena-purple.svg" alt="Kadena"/>
  <img src="https://img.shields.io/badge/Consensus-PBFT-orange.svg" alt="PBFT"/>
  <img src="https://img.shields.io/badge/Status-Production--Ready-success.svg" alt="Production Ready"/>
</p>

# ⬡ Aegen L2 for Kadena

**Aegen** is a high-performance **Layer-2 blockchain** for the **Kadena** ecosystem, featuring optimistic rollup settlement to Kadena L1, Pact-compatible token operations, fraud proofs, data availability, and Docker-based multi-validator deployment.

---

## ✨ Key Features

| Feature | Description |
|---------|-------------|
| **10,000+ TPS** | High throughput with 5-second block finality |
| **Kadena k: Addresses** | Native Kadena account format |
| **fungible-v2 Tokens** | Pact-compatible token standard |
| **EVM Execution** | Full EVM with opcodes, storage, logs, receipts |
| **PBFT Consensus** | Byzantine fault tolerant consensus |
| **L1 Settlement** | Pact contract for batch submissions with fraud proofs |
| **Fee Market** | Gas-based transaction fees paid to validators |
| **ZK Precompile** | Groth16 proof verification at address 0x09 |
| **Data Availability** | Erasure coding and DAS (Data Availability Sampling) |
| **Cross-Chain Bridge** | UI for depositing/withdrawing assets |
| **Production Monitoring** | Prometheus + Grafana dashboards |
| **Docker Testnet** | 3-node validator deployment |

---

## 🚀 Quick Start

### Prerequisites
- CMake 3.20+
- C++20 compatible compiler (MSVC 2022, GCC 11+, Clang 14+)
- Python 3.8+ (for tests)
- Docker & Docker Compose (for testnet)

### Build (Windows)
```powershell
mkdir build && cd build
cmake ..
cmake --build . --config Debug
```

### Build (Linux/macOS)
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### Run Single Node
```bash
./build/Debug/aegen-node.exe   # Windows
./build/aegen-node             # Linux
```

### Run Integration Tests
```bash
python integration_test.py
```

---

## 🐳 Docker Deployment

### Single Node
```bash
docker build -t aegen-node .
docker run -p 8545:8545 -p 30303:30303 aegen-node
```

### Multi-Validator Testnet (3 Nodes)
```bash
docker-compose -f docker-compose.testnet.yml up -d
```

### Services

| Service | Port | Description |
|---------|------|-------------|
| Validator 1 | 8545 | Primary validator RPC |
| Validator 2 | 8546 | Validator RPC |
| Validator 3 | 8547 | Validator RPC |
| Explorer | 3000 | Block explorer UI |
| Prometheus | 9090 | Metrics collection |
| Grafana | 3001 | Monitoring dashboards |

---

## 🔗 Kadena L1 Settlement

### Settlement Flow
```
L2 Blocks → BatchManager → SettlementBridge → KadenaClient → Chainweb API
                                                    ↓
                                         free.aegen.submit-batch
                                                    ↓
                                         Kadena L1 (Testnet/Mainnet)
```

### Pact Contract Features
- `submit-batch` - Submit L2 state roots with DA commitments
- `challenge-batch` - Initiate fraud proof challenge
- `resolve-challenge` - Resolve fraud proof (slash/reward)
- `finalize-batch` - Finalize after challenge period
- `get-batch` - Query batch details

---

## 📡 RPC API

### Native Operations
| Method | Description |
|--------|-------------|
| `getChainInfo` | Get network status |
| `sendTransaction` | Transfer native tokens |
| `getBalance` | Get account balance |
| `getNonce` | Get account nonce |
| `generateWallet` | Generate new keypair |

### fungible-v2 Token Operations
| Method | Pact Equivalent |
|--------|-----------------|
| `createFungible` | Deploy new token |
| `transfer` | `(coin.transfer)` |
| `get-balance` | `(coin.get-balance)` |
| `details` | `(coin.details)` |
| `mint` | `(coin.coinbase)` |

### Explorer Endpoints
| Method | Description |
|--------|-------------|
| `getBlocks` | Paginated block list |
| `getBlock` | Block details by height |
| `getTransactions` | Paginated transaction list |
| `getTransaction` | Transaction details by hash |
| `getMetrics` | Prometheus metrics |

### Ethereum JSON-RPC (EVM Compatibility)
| Method | Description |
|--------|-------------|
| `eth_chainId` | Returns chain ID (0x1e) |
| `eth_blockNumber` | Returns current block height |
| `eth_getBalance` | Get EVM account balance |
| `eth_call` | Execute read-only contract call |
| `eth_sendRawTransaction` | Submit signed transaction |
| `eth_getTransactionReceipt` | Get transaction receipt with logs |

### Example Request
```bash
curl -X POST http://localhost:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getChainInfo","params":{},"id":1}'
```

### Example Responses

**getChainInfo**
```json
{
  "result": {
    "name": "aegen-l2",
    "height": 42,
    "chainId": "30",
    "mempoolSize": 0,
    "peerCount": 3,
    "totalTransactions": 156,
    "tokenCount": 2,
    "l1Network": "kadena-mainnet"
  }
}
```

**sendTransaction**
```bash
curl -X POST http://localhost:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"sendTransaction","params":[{"sender":"alice","receiver":"bob","amount":"1000","nonce":"0"}],"id":1}'
```
```json
{
  "result": {
    "requestKey": "fa8dd885c457e0faa4d5cfb38dab4e04012c9b8a1e..."
  }
}
```

**getBalance**
```bash
curl -X POST http://localhost:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getBalance","params":[{"account":"alice"}],"id":1}'
```
```json
{
  "result": 9957800
}
```

**eth_getTransactionReceipt**
```bash
curl -X POST http://localhost:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"eth_getTransactionReceipt","params":["0xfa8dd885c457e0faa4d5cfb38dab4e04012c9b8a1e..."],"id":1}'
```
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "transactionHash": "0xfa8dd885c457e0faa4d5cfb38dab4e04...",
    "transactionIndex": "0x0",
    "blockHash": "0x000...000",
    "blockNumber": "0x2a",
    "from": "alice",
    "to": "bob",
    "cumulativeGasUsed": "0x5208",
    "gasUsed": "0x5208",
    "contractAddress": null,
    "logs": [],
    "status": "0x1"
  }
}
```

**generateWallet**
```bash
curl -X POST http://localhost:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"generateWallet","params":{},"id":1}'
```
```json
{
  "result": {
    "address": "k:96b66bbddfec967660eb3fe727360b8b2697d516a9cb1733a3ccf949b524596b",
    "publicKey": "96b66bbddfec967660eb3fe727360b8b2697d516a9cb1733a3ccf949b524596b",
    "privateKey": "a1b2c3d4e5f6..."
  }
}
```

**createFungible (Token Creation)**
```bash
curl -X POST http://localhost:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"createFungible","params":[{"name":"TestToken","symbol":"TST","precision":"18","initialSupply":"1000000","creator":"alice"}],"id":1}'
```
```json
{
  "result": {
    "module": "coin.62e8c2dcd1ff9562",
    "status": "deployed"
  }
}
```

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│                           AEGEN L2 NODE                                   │
├──────────────────────────────────────────────────────────────────────────┤
│  CLI Wallet │ Block Explorer │ RPC Server (8545) │ P2P Gossip (30303)    │
├──────────────────────────────────────────────────────────────────────────┤
│  PBFT Consensus │ Mempool │ Execution Engine │ Token Manager             │
├──────────────────────────────────────────────────────────────────────────┤
│  State Manager │ RocksDB Wrapper │ Merkle Trees │ Fraud Proof Verifier   │
├──────────────────────────────────────────────────────────────────────────┤
│                         DATA AVAILABILITY LAYER                           │
│  Erasure Coding │ DAS Sampling │ Blob Storage │ Merkle Proofs            │
├──────────────────────────────────────────────────────────────────────────┤
│                         SETTLEMENT LAYER                                  │
│  BatchManager → SettlementBridge → KadenaClient (HTTPS)                  │
└──────────────────────────────────────────────────────────────────────────┘
                                   ↓
┌──────────────────────────────────────────────────────────────────────────┐
│                         KADENA LAYER-1                                    │
│  Chainweb API │ free.aegen module │ Fraud Proofs │ DA Commitments        │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
aegen-blockchain/
├── core/           # Blocks, Transactions, Merkle trees
├── wallet/         # Keys, Signing, k: Addresses
├── tokens/         # fungible-v2 Token Manager
├── consensus/      # PBFT, Leader election, Validators
├── exec/           # Execution Engine, VM
├── db/             # State Manager, RocksDB Wrapper
├── network/        # RPC Server, P2P Gossip
├── rpc/            # RPC Endpoints
├── settlement/     # BatchManager, KadenaClient
├── proofs/         # Fraud Proof Verifier
├── da/             # Data Availability Layer
├── util/           # Crypto, Logging, Metrics
├── contracts/      # Pact L1 contract
├── explorer/       # Web Block Explorer + Bridge UI
├── monitoring/     # Prometheus & Grafana configs
├── deploy/         # DigitalOcean deployment scripts
└── tests/          # Unit & Integration tests
```

---

## ✅ Testing

### Unit Tests
```bash
./build/tests/Debug/unit_block_test.exe
./build/tests/Debug/unit_consensus_test.exe
./build/tests/Debug/unit_token_test.exe
./build/tests/Debug/unit_wallet_test.exe
./build/tests/Debug/unit_vm_test.exe
./build/tests/Debug/unit_execution_test.exe
```

### End-to-End Test
```bash
# Start node first
./build/Debug/aegen-node.exe --node node-1

# In another terminal
python tests/e2e_test.py
```

### Integration Test
```bash
python integration_test.py
```

### Test Coverage
- ✅ Block production & validation
- ✅ PBFT consensus (single and multi-node)
- ✅ Transaction execution with fee market
- ✅ EVM opcodes (arithmetic, storage, logs)
- ✅ fungible-v2 token operations
- ✅ Wallet key generation & signing
- ✅ Address validation (k: format)
- ✅ RPC endpoints (native + eth_*)
- ✅ L1 settlement batching
- ✅ Transaction receipts with logs

---

## 🔐 Cryptography

The crypto module provides a **libsodium-compatible API**:

```cpp
// Key generation
crypto::crypto_sign_keypair(pk, sk);

// Signing
crypto::crypto_sign_detached(sig, msg, msglen, sk);

// Verification
crypto::crypto_sign_verify_detached(sig, msg, msglen, pk);
```

**Features:**
- NIST FIPS 180-4 compliant SHA-256
- Deterministic Ed25519-compatible signatures
- Secure memory cleanup
- Drop-in libsodium replacement

---

## 📊 Monitoring

### Prometheus Metrics
- `aegen_blocks_produced_total`
- `aegen_transactions_processed_total`
- `aegen_peers_connected`
- `aegen_rpc_latency_ms`
- `aegen_settlement_batches_total`

### Grafana Dashboard
Pre-configured dashboard at `http://localhost:3001` (admin/admin)

---

## 🌐 DigitalOcean Deployment

See [`deploy/DIGITALOCEAN.md`](deploy/DIGITALOCEAN.md) for full instructions.

```bash
# Quick deploy on Ubuntu 22.04
chmod +x deploy/quick_deploy.sh
./deploy/quick_deploy.sh
```

---

## 📄 License

MIT License

---

<p align="center">
  <strong>Built for Kadena. Powered by C++20.</strong>
</p>