# Business Requirements Document (BRD)
## Aegen: Layer-2 Blockchain on Kadena

**Version:** 1.0  
**Date:** January 2, 2026  
**Status:** Draft  
**Document Owner:** Engineering Team

---

## 1. Executive Summary

### 1.1 Project Overview
Aegen is a production-grade Layer-2 (L2) blockchain solution designed to scale transaction throughput while maintaining security guarantees through settlement on the Kadena Layer-1 blockchain. The system executes transactions off-chain, batches state changes, and periodically settles proofs back to Kadena's mainnet.

### 1.2 Business Objectives
- **Scalability**: Increase transaction throughput beyond L1 limitations (target: 1000+ TPS)
- **Cost Efficiency**: Reduce per-transaction costs by 90%+ through batching
- **Security**: Maintain cryptographic security guarantees via L1 settlement
- **User Experience**: Provide sub-second transaction finality for end users
- **Transparency**: Offer real-time blockchain exploration and analytics

### 1.3 Success Criteria
- System processes ≥1,000 transactions per second under load
- Settlement batches submitted to Kadena every 10-60 minutes
- 99.9% uptime for validator nodes
- Block explorer loads pages in <2 seconds
- Complete test coverage (≥80% code coverage)

---

## 2. System Architecture

### 2.1 Architectural Principles
The system adheres to a clear engineering hierarchy:
1. **Correctness** - Deterministic state transitions, no data corruption
2. **Security** - Signature validation, replay protection, safe state management
3. **Performance** - Optimized execution, efficient batching
4. **User Experience** - Fast finality, intuitive explorer
5. **Extensibility** - Modular design for future enhancements

### 2.2 High-Level Design
```
┌─────────────────────────────────────────┐
│         User Applications               │
└──────────────┬──────────────────────────┘
               │ RPC Calls
┌──────────────▼──────────────────────────┐
│         Aegen L2 Network                │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐ │
│  │ Node 1  │  │ Node 2  │  │ Node 3  │ │
│  └────┬────┘  └────┬────┘  └────┬────┘ │
│       └────────────┼────────────┘       │
│              Consensus                   │
│         (PBFT/Leader-based)              │
│                                          │
│  ┌─────────────────────────────────┐    │
│  │   Execution Engine              │    │
│  │   State Management (RocksDB)    │    │
│  └─────────────────────────────────┘    │
└──────────────┬──────────────────────────┘
               │ Batch Proofs
┌──────────────▼──────────────────────────┐
│      Kadena L1 Blockchain               │
│      (Settlement Layer)                 │
└─────────────────────────────────────────┘
```

### 2.3 Core Components

#### 2.3.1 Networking Layer
**Purpose**: Enable peer-to-peer communication between validator nodes

**Requirements**:
- RPC-based communication (no REST endpoints for node-to-node)
- Transaction gossip protocol for mempool synchronization
- Block propagation mechanism
- Peer discovery and connection management
- Support for 10-100 validator nodes

**Key Interfaces**:
```cpp
class RPCServer {
    void registerEndpoint(name, handler);
    void start();
    void stop();
};

Endpoints:
- sendTransaction(tx) → txHash
- getTransaction(txHash) → Transaction
- getBlock(blockHeight) → Block
- getState(address) → AccountState
- getBatchProof(batchId) → ProofData
- getBalance(address, tokenId?) → Balance
- getTokenInfo(tokenId) → TokenInfo
- listTokens() → TokenList
- createWallet() → {address, publicKey} (client-side only)
- getTransactionHistory(address, tokenId?) → TxList
```

#### 2.3.2 Transaction Mempool
**Purpose**: Queue and prioritize pending transactions before execution

**Requirements**:
- Priority-based ordering (by fee/gas price)
- Signature validation before acceptance
- Nonce checking for replay protection
- Configurable size limits (10,000+ transactions)
- Efficient pop operations for block production

**Data Structure**:
```cpp
class Mempool {
    bool validate(Transaction);
    void add(Transaction);
    Transaction pop();
    size_t size();
};
```

#### 2.3.3 Wallet & Key Management
**Purpose**: Enable users to generate and manage cryptographic identities

**Requirements**:
- **Key Generation**: Ed25519 or secp256k1 keypair generation
- **Address Derivation**: Generate blockchain addresses from public keys
- **Private Key Security**: Encrypted storage with user password/passphrase
- **Mnemonic Support**: BIP39-compatible seed phrase generation (12/24 words)
- **Key Import/Export**: Support for external wallet integration
- **Multi-signature**: Support for multi-sig wallet creation (future)

**Key Interfaces**:
```cpp
class Wallet {
    KeyPair generateKeyPair();
    Address deriveAddress(PublicKey);
    string generateMnemonic();
    KeyPair restoreFromMnemonic(string);
    Transaction signTransaction(Transaction, PrivateKey);
    bool verifySignature(Transaction, PublicKey);
};
```

**Security Requirements**:
- Private keys never leave user's device (client-side generation)
- Encrypted storage using AES-256-GCM
- Key derivation using PBKDF2 or Argon2
- Secure random number generation (entropy from OS)

#### 2.3.4 Token Management System
**Purpose**: Enable creation, issuance, and transfer of custom tokens on Aegen L2

**Requirements**:
- **Token Creation**: Users can deploy new token contracts
- **Token Standards**: Support for fungible tokens (ERC-20 style)
- **Token Metadata**: Name, symbol, decimals, total supply
- **Minting/Burning**: Configurable mint/burn permissions
- **Token Registry**: On-chain registry of all deployed tokens
- **Balance Tracking**: Efficient multi-token balance queries per address

**Token Operations**:
```cpp
class TokenManager {
    TokenId createToken(string name, string symbol, uint8 decimals, uint256 supply);
    bool transfer(TokenId, Address from, Address to, uint256 amount);
    bool mint(TokenId, Address to, uint256 amount);
    bool burn(TokenId, Address from, uint256 amount);
    uint256 balanceOf(TokenId, Address);
    TokenInfo getTokenInfo(TokenId);
};
```

**Token Features**:
- **Atomic Swaps**: Trade tokens peer-to-peer without intermediaries
- **Batch Transfers**: Send tokens to multiple recipients in one transaction
- **Allowances**: Approve third parties to spend tokens on your behalf
- **Token Freezing**: Optional freeze mechanism for compliance tokens
- **Supply Management**: Fixed supply, capped supply, or unlimited minting

**Storage Schema**:
- `token:{tokenId}` → TokenInfo (name, symbol, supply, creator)
- `balance:{tokenId}:{address}` → uint256 (token balance)
- `allowance:{tokenId}:{owner}:{spender}` → uint256 (approved amount)
- `token:registry` → List of all TokenIds

#### 2.3.5 Execution Engine
**Purpose**: Apply transactions deterministically and maintain state

**Requirements**:
- Deterministic state transitions (same input → same output)
- Account balance management (native token + custom tokens)
- Token transfer execution
- Smart contract execution (future enhancement)
- State root computed (Merkle tree)
- Rollback capability for failed transactions
- Integration with RocksDB for persistence

**Core Operations**:
```cpp
class ExecutionEngine {
    State applyTransaction(Transaction);
    State applyTokenTransaction(TokenTransaction);
    StateRoot computeStateRoot();
    void rollback(Block);
    bool validateBalance(Address, TokenId, Amount);
};
```

#### 2.3.6 Consensus Layer
**Purpose**: Achieve agreement on block ordering among validators

**Requirements**:
- Leader-based rotation or PBFT consensus
- Block proposal mechanism
- Block validation (signature, state root, token transfers)
- Fork resolution
- Finality guarantees within 3-10 seconds
- Byzantine fault tolerance (support up to 33% malicious nodes)

**Key Functions**:
```cpp
class Consensus {
    Block proposeBlock(vector<Transaction>);
    bool validateBlock(Block);
    void finalizeBlock(Block);
    BatchRoot generateBatchRoot(vector<Block>);
};
```

#### 2.3.5 Settlement Bridge
**Purpose**: Submit batch proofs to Kadena L1 for final settlement

**Requirements**:
- Batch aggregation (100-1000 blocks per batch)
- State root computation for batches
- Pact smart contract interaction
- Transaction proof generation
- Settlement confirmation monitoring
- Retry logic for failed submissions

**Settlement Flow**:
```cpp
class SettlementBridge {
    ProofPayload prepareProof(BatchRoot);
    bool submitToKadena(ProofPayload);
    bool verifyResponse(Response);
    void handleFailure(Error);
};
```

**Kadena Integration**:
- Deploy Pact bridge contract on Kadena mainnet
- Contract functions: `submit-batch`, `verify-proof`, `challenge-fraud`
- Gas management for L1 transactions

#### 2.3.6 Proof Framework
**Purpose**: Enable fraud detection and validity verification

**Requirements**:
- Fraud proof submission mechanism
- Proof verification logic
- Challenge period (7-day window)
- Slashing mechanism for invalid proofs
- Support for future ZK-proof integration

```cpp
class ProofEngine {
    virtual bool verifyFraudProof(FraudProof) = 0;
    virtual bool verifyZKProof(ZKProof) = 0;
    void handleChallenge(Challenge);
};
```

#### 2.3.7 Database Layer
**Purpose**: Persistent storage for blockchain state

**Requirements**:
- RocksDB as storage engine
- Atomic commits for state changes
- Crash recovery support
- Efficient key-value lookups
- Snapshot capability for state exports

**Storage Schema**:
- `account:{address}` → AccountState (balance, nonce)
- `token_balance:{tokenId}:{address}` → uint256 (token balance)
- `token:{tokenId}` → TokenInfo (name, symbol, supply, creator)
- `allowance:{tokenId}:{owner}:{spender}` → uint256 (approved amount)
- `block:{height}` → Block (transactions, state root)
- `tx:{hash}` → Transaction (sender, receiver, amount, tokenId)
- `state:{height}` → StateRoot (Merkle root)
- `wallet:{address}:encrypted_key` → EncryptedPrivateKey (client-side only)

```cpp
class RocksDBWrapper {
    void put(key, value);
    string get(key);
    void commit();
    void rollback();
    void createSnapshot();
    vector<string> listKeys(prefix);
};
```

---

## 3. Block Explorer Requirements

### 3.1 Functional Requirements

#### 3.1.1 Core Features
- **Block List**: Display recent blocks with height, timestamp, transaction count
- **Transaction List**: Show pending and confirmed transactions (native + token transfers)
- **Account Lookup**: View balance (native + all token balances), transaction history by address
- **Token Explorer**: Browse all deployed tokens with metadata and holders
- **Token Details Page**: Show token info, total supply, holder distribution, transfer history
- **Wallet Dashboard**: User's portfolio view with all token balances
- **Search**: Global search for blocks, transactions, addresses, tokens
- **Real-time Updates**: WebSocket-based live data streaming

#### 3.1.2 Analytics Dashboard
- **TPS Graph**: Transactions per second over time (1h, 24h, 7d views)
- **Token Activity**: Most active tokens by transaction volume
- **Top Holders**: Leaderboard for token holders
- **Mempool Monitor**: Pending transaction count and average wait time
- **Settlement Tracker**: L1 batch submissions with status
- **Network Stats**: Active nodes, consensus round time
- **Gas Metrics**: Average fees, fee distribution
- **Token Metrics**: Total tokens deployed, total transfers, market cap (if price oracle)

#### 3.1.3 Settlement Visualization
- **Batch Timeline**: Visual flow of L2 blocks → batches → L1 settlement
- **Proof Details**: State roots, transaction counts per batch
- **Kadena Links**: Direct links to L1 settlement transactions
- **Challenge Window**: Display remaining time for fraud proof submission

### 3.2 Technical Requirements

**Backend**:
- REST API adapter wrapping RPC endpoints
- WebSocket server for real-time updates
- PostgreSQL for indexed data (transactions, blocks)
- Caching layer (Redis) for frequently accessed data

**Frontend**:
- Modern framework (React/Vue/Svelte)
- Responsive design (mobile, tablet, desktop)
- Dark/light theme support
- Loading states and error handling
- Accessibility compliance (WCAG 2.1 AA)

**Performance**:
- Page load time <2 seconds
- API response time <200ms (p95)
- Real-time updates with <1 second latency

---

## 4. Project Structure

```plaintext
/aegen
├── CMakeLists.txt
├── README.md
├── BRD.md
├── /core
│   ├── transaction.h/cpp
│   ├── block.h/cpp
│   ├── account.h/cpp
│   ├── token.h/cpp
│   └── merkle.h/cpp
├── /wallet
│   ├── keypair.h/cpp
│   ├── address.h/cpp
│   ├── mnemonic.h/cpp
│   └── signer.h/cpp
├── /tokens
│   ├── token_manager.h/cpp
│   ├── token_registry.h/cpp
│   └── token_transfer.h/cpp
├── /consensus
│   ├── pbft.h/cpp
│   ├── leader.h/cpp
│   └── validator.h/cpp
├── /network
│   ├── peer.h/cpp
│   ├── gossip.h/cpp
│   └── rpc_server.h/cpp
├── /rpc
│   ├── endpoints.h/cpp
│   └── handler.h/cpp
├── /db
│   ├── rocksdb_wrapper.h/cpp
│   └── state_manager.h/cpp
├── /exec
│   ├── execution_engine.h/cpp
│   └── vm.h/cpp
├── /settlement
│   ├── bridge.h/cpp
│   ├── batch.h/cpp
│   └── kadena_client.h/cpp
├── /proofs
│   ├── fraud_proof.h/cpp
│   └── zk_proof.h/cpp (future)
├── /explorer
│   ├── /backend
│   │   ├── api_server.cpp
│   │   └── indexer.cpp
│   └── /frontend
│       ├── package.json
│       └── /src
│           ├── /components
│           │   ├── WalletConnect.tsx
│           │   ├── TokenList.tsx
│           │   └── BalanceDisplay.tsx
│           └── /pages
│               ├── Tokens.tsx
│               └── Wallet.tsx
├── /tests
│   ├── /unit
│   │   ├── wallet_test.cpp
│   │   └── token_test.cpp
│   └── /integration
└── /docs
    ├── architecture.md
    ├── setup.md
    ├── rpc_reference.md
    ├── wallet_guide.md
    ├── token_creation.md
    └── settlement_flow.md
```

---

## 5. Testing Requirements

### 5.1 Unit Testing
- **Coverage Target**: ≥80% code coverage
- **Framework**: Google Test (C++)
- **Scope**: Test each module independently
  - Wallet key generation and signing
  - Address derivation from public keys
  - Token creation and transfer logic
  - Balance tracking for multiple tokens
  - Transaction validation
  - Block production
  - State root computation
  - RocksDB operations
  - RPC endpoint handlers

### 5.2 Integration Testing
**Test Scenarios**:
1. **Multi-Node Setup**: Spin up 3 validator nodes
2. **Wallet Creation**: Generate 10 wallets → verify unique addresses
3. **Token Deployment**: Create 5 different tokens → verify registry
4. **Transaction Flow**: Submit 1000 transactions (native + token transfers) → verify all processed
5. **Balance Verification**: Check balances across multiple tokens → verify correctness
6. **Consensus**: Ensure all nodes agree on block order and token states
7. **State Consistency**: Verify RocksDB state matches across nodes (including token balances)
8. **Batch Generation**: Create batch proof → validate format
9. **Settlement Simulation**: Mock Kadena submission → verify success response

### 5.3 Performance Testing
- **Load Test**: Sustain 1000 TPS for 1 hour
- **Stress Test**: Find breaking point (max TPS)
- **Endurance Test**: Run 7 days continuously
- **Recovery Test**: Node crash → restart → verify state recovery

### 5.4 CI/CD Pipeline
- Automated builds on every commit
- Run all tests before merge
- Code coverage reports
- Performance regression checks

---

## 6. Security Requirements

### 6.1 Cryptographic Security
- **Signature Scheme**: Ed25519 or secp256k1
- **Hash Function**: SHA-256 or Blake2b
- **State Root**: Merkle tree with collision-resistant hash
- **Random Number Generation**: Cryptographically secure RNG

### 6.2 Transaction Security
- **Replay Protection**: Nonce-based system per account
- **Signature Validation**: Verify all incoming transactions (native + token)
- **Double-Spend Prevention**: Check account balance before execution (multi-token)
- **Token Authorization**: Verify sender owns tokens before transfer
- **Allowance Validation**: Check approved amount for third-party transfers
- **Gas Limits**: Prevent resource exhaustion attacks
- **Integer Overflow Protection**: Safe arithmetic for token amounts

### 6.3 Network Security
- **DDoS Protection**: Rate limiting on RPC endpoints
- **Peer Authentication**: Validate peer identity before connection
- **Message Validation**: Reject malformed or oversized messages
- **Sybil Resistance**: Stake-weighted validator selection

### 6.4 State Management Security
- **Atomic Commits**: All-or-nothing state updates
- **Crash Safety**: RocksDB write-ahead log (WAL)
- **Rollback Capability**: Revert invalid blocks
- **State Snapshots**: Periodic backups for disaster recovery

### 6.5 Wallet Security
- **Client-Side Generation**: Private keys never transmitted to servers
- **Encrypted Storage**: AES-256-GCM encryption for stored keys
- **Password Protection**: PBKDF2 or Argon2 key derivation (100k+ iterations)
- **Mnemonic Security**: BIP39-compliant seed phrases with checksums
- **Secure Deletion**: Overwrite memory containing private keys after use
- **Hardware Wallet Support**: Interface for external hardware wallets (future)

### 6.6 Token Security
- **Mint Authority**: Only authorized addresses can mint new tokens
- **Burn Validation**: Prevent burning tokens the sender doesn't own
- **Supply Caps**: Enforce maximum supply limits for capped tokens
- **Transfer Validation**: Atomic token transfers with rollback on failure
- **Allowance Limits**: Prevent unlimited allowance exploits

### 6.7 Settlement Security
- **Fraud Proof Window**: 7-day challenge period
- **Proof Verification**: Cryptographic validation on L1
- **Slashing**: Penalize validators for invalid submissions
- **Emergency Pause**: Ability to halt settlements if vulnerability detected

---

## 7. Documentation Requirements

### 7.1 Developer Documentation
- **Architecture Overview**: System design, component interactions
- **Setup Guide**: Build instructions, dependencies, configuration
- **RPC Reference**: Complete API documentation with examples
- **Wallet Integration Guide**: How to integrate wallet functionality
- **Token Creation Guide**: Step-by-step token deployment tutorial
- **Database Schema**: Key-value structure, indexing strategy
- **Testing Guide**: How to run tests, add new tests

### 7.2 Operator Documentation
- **Node Setup**: Validator node installation and configuration
- **Monitoring**: Metrics to track, alerting setup
- **Maintenance**: Backup procedures, upgrade process
- **Troubleshooting**: Common issues and solutions

### 7.3 User Documentation
- **Wallet Setup Guide**: Creating and securing wallets
- **Token Guide**: How to create, send, and receive tokens
- **Explorer Guide**: How to use the block explorer
- **Transaction Submission**: How to send transactions
- **Settlement Tracking**: Understanding L1 finality
- **Security Best Practices**: Protecting private keys and assets

---

## 8. Deliverables

### 8.1 Phase 1: Core Infrastructure (Months 1-3)
- [ ] Project structure and build system (CMake)
- [ ] Cryptographic libraries integration (key generation, signing)
- [ ] Wallet module (keypair generation, address derivation, mnemonics)
- [ ] Networking layer with RPC
- [ ] Transaction mempool
- [ ] Basic execution engine
- [ ] Token management system (creation, transfer, balances)
- [ ] RocksDB integration (with token storage schema)
- [ ] Unit tests for core modules (wallet, tokens, transactions)

### 8.2 Phase 2: Consensus & Settlement (Months 4-6)
- [ ] Consensus implementation (PBFT or leader-based)
- [ ] Multi-node testing
- [ ] Settlement bridge to Kadena
- [ ] Batch proof generation
- [ ] Pact bridge contract deployment
- [ ] Integration tests

### 8.3 Phase 3: Explorer & Production (Months 7-9)
- [ ] Block explorer backend (REST API + indexer)
- [ ] Token indexing service
- [ ] Block explorer frontend (UI with wallet and token features)
- [ ] Wallet interface (create, import, view balances)
- [ ] Token explorer pages (list, details, holders)
- [ ] Real-time updates (WebSocket)
- [ ] Analytics dashboard (including token metrics)
- [ ] Performance testing and optimization
- [ ] Security audit (including wallet and token security)
- [ ] Documentation completion (wallet and token guides)

### 8.4 Phase 4: Launch (Month 10)
- [ ] Testnet deployment
- [ ] Community testing
- [ ] Bug fixes and refinements
- [ ] Mainnet launch

---

## 9. Technology Stack

### 9.1 Backend
- **Language**: C++20
- **Build System**: CMake 3.20+
- **Database**: RocksDB 7.0+
- **Networking**: gRPC or custom RPC over TCP
- **Testing**: Google Test, Google Benchmark
- **Cryptography**: libsodium or OpenSSL

### 9.2 Explorer
- **Backend**: Node.js + Express or Python + FastAPI
- **Database**: PostgreSQL 14+ (indexer), Redis (cache)
- **Frontend**: React 18+ or Vue 3+
- **Styling**: Tailwind CSS
- **Charts**: Recharts or D3.js
- **WebSocket**: Socket.IO

### 9.3 Infrastructure
- **Containerization**: Docker
- **Orchestration**: Kubernetes (optional)
- **CI/CD**: GitHub Actions or GitLab CI
- **Monitoring**: Prometheus + Grafana

---

## 10. Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Private key theft/exposure | Critical | Medium | Client-side encryption, security audits, user education |
| Token contract bugs | High | Medium | Extensive testing, formal verification, audit |
| Consensus bugs leading to forks | High | Medium | Extensive testing, formal verification |
| Settlement failures on Kadena | High | Low | Retry logic, manual intervention process |
| Performance below 1000 TPS | Medium | Medium | Early benchmarking, optimization sprints |
| Token double-spend attack | Critical | Low | Atomic state updates, rollback mechanisms |
| Security vulnerability | High | Low | Security audit, bug bounty program |
| RocksDB corruption | High | Low | Regular backups, write-ahead logging |
| Wallet UX confusion | Medium | Medium | User testing, clear documentation, tutorials |

---

## 11. Open Questions

1. **Consensus Mechanism**: PBFT vs. leader-based? (Decision: PBFT for Byzantine fault tolerance)
2. **Batch Size**: How many blocks per batch? (Recommendation: 100-500 based on gas costs)
3. **Challenge Period**: 7 days optimal? (Alternative: 3 days for faster finality)
4. **Gas Model**: Should L2 have its own gas token or use KDA? (TBD)
5. **Smart Contract Support**: Timeline for VM integration? (Phase 5, post-launch)

---

## 12. Approval & Sign-off

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Engineering Lead | | | |
| Product Manager | | | |
| Security Lead | | | |
| CTO | | | |

---

**Next Steps**:
1. Review and approve this BRD
2. Create detailed technical specifications for each component
3. Set up development environment and repository
4. Begin Phase 1 implementation

---

*This document is version-controlled. All changes require approval from the Engineering Lead.*
