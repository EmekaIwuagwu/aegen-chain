// Aegen Blockchain Explorer - Data Layer
// Fetches real data from the blockchain RPC

// Auto-detect RPC URL: Use same host as explorer but port 8545
const RPC_URL = window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'
    ? 'http://localhost:8545'
    : `http://${window.location.hostname}:8545`;

// RPC Helper
async function rpcCall(method, params = {}) {
    try {
        const response = await fetch(RPC_URL, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ jsonrpc: '2.0', method, params, id: Date.now() })
        });
        const data = await response.json();
        if (data.error) {
            console.error('RPC Error:', data.error);
            return null;
        }
        return data.result;
    } catch (e) {
        console.error('RPC Error:', e);
        return null;
    }
}

// Cache for chain info (refreshes every 5 seconds)
let cachedChainInfo = null;
let lastChainInfoFetch = 0;

async function getChainInfo() {
    const now = Date.now();
    if (cachedChainInfo && now - lastChainInfoFetch < 5000) {
        return cachedChainInfo;
    }

    const result = await rpcCall('getChainInfo', {});
    if (result) {
        cachedChainInfo = result;
        lastChainInfoFetch = now;
        return result;
    }

    // Fallback if RPC fails
    return {
        blockHeight: 0,
        mempoolSize: 0,
        peerCount: 0,
        networkId: 'aegen-l2',
        chainId: '0',
        totalTransactions: 0
    };
}

// Fetch blocks from RPC
async function fetchBlocks(page = 1, limit = 10) {
    const result = await rpcCall('getBlocks', { page, limit });
    if (result && result.blocks) {
        return {
            blocks: result.blocks.map(formatBlock),
            total: result.total,
            page: result.page,
            limit: result.limit
        };
    }
    return { blocks: [], total: 0, page: 1, limit: 10 };
}

// Fetch single block
async function fetchBlock(height) {
    const result = await rpcCall('getBlock', { height: String(height) });
    if (result) {
        return formatBlock(result);
    }
    return null;
}

// Format block for display
function formatBlock(block) {
    return {
        height: block.height,
        hash: block.hash,
        parentHash: block.parentHash,
        stateRoot: block.stateRoot,
        txCount: block.txCount || (block.transactions ? block.transactions.length : 0),
        timestamp: formatTimestamp(block.timestamp),
        timestampFull: block.timestamp,
        validator: block.validator || 'validator-1',
        gasUsed: block.gasUsed || 0,
        gasLimit: block.gasLimit || 100000,
        transactions: block.transactions || []
    };
}

// Fetch transactions from RPC
async function fetchTransactions(page = 1, limit = 10) {
    const result = await rpcCall('getTransactions', { page, limit });
    if (result && result.transactions) {
        return {
            transactions: result.transactions.map(formatTransaction),
            total: result.total,
            page: result.page,
            limit: result.limit
        };
    }
    return { transactions: [], total: 0, page: 1, limit: 10 };
}

// Fetch single transaction
async function fetchTransaction(hash) {
    const result = await rpcCall('getTransaction', { hash });
    if (result) {
        return formatTransaction(result);
    }
    return null;
}

// Format transaction for display
function formatTransaction(tx) {
    return {
        hash: tx.hash,
        blockHeight: tx.blockHeight,
        from: tx.from,
        to: tx.to,
        amount: tx.amount,
        nonce: tx.nonce,
        gasUsed: tx.gasUsed || 21000,
        gasPrice: tx.gasPrice || 0.00000001,
        fee: tx.fee || (tx.gasUsed * tx.gasPrice) || 0.00021,
        status: tx.status || 'Success',
        type: tx.type || 'Native Transfer',
        timestamp: formatTimestamp(tx.timestamp),
        timestampFull: tx.timestamp
    };
}

// Format timestamp for display
function formatTimestamp(ts) {
    if (!ts) return new Date().toLocaleTimeString();
    try {
        return new Date(ts).toLocaleTimeString();
    } catch (e) {
        return ts;
    }
}

// Generate wallet
async function generateWallet() {
    const result = await rpcCall('generateWallet', {});
    return result;
}

// Get balance
async function getBalance(account) {
    const result = await rpcCall('getBalance', { account });
    return result;
}

// URL parameter helper
function getUrlParam(param) {
    const urlParams = new URLSearchParams(window.location.search);
    return urlParams.get(param);
}

// ===== Fallback functions for when RPC is unavailable =====

function generateHash() {
    const chars = 'abcdef0123456789';
    let hash = '';
    for (let i = 0; i < 64; i++) {
        hash += chars[Math.floor(Math.random() * chars.length)];
    }
    return hash;
}

// Fallback: Generate mock blocks when RPC unavailable
function getBlocks(count = 20) {
    const blocks = [];
    const now = Date.now();

    for (let i = 0; i < count; i++) {
        const height = 100 - i;
        if (height < 1) break;

        blocks.push({
            height: height,
            hash: generateHash(),
            parentHash: generateHash(),
            stateRoot: 'f5a5fd42d16a20302798ef6ed309979b43003d2320d9f0e8ea9831a92759fb4b',
            txCount: Math.floor(Math.random() * 5) + 1,
            timestamp: new Date(now - i * 5000).toLocaleTimeString(),
            timestampFull: new Date(now - i * 5000).toISOString(),
            validator: 'validator-1',
            gasUsed: Math.floor(Math.random() * 50000) + 10000,
            gasLimit: 100000
        });
    }

    return blocks;
}

function getBlockByHeight(height) {
    return {
        height: height,
        hash: generateHash(),
        parentHash: generateHash(),
        stateRoot: 'f5a5fd42d16a20302798ef6ed309979b43003d2320d9f0e8ea9831a92759fb4b',
        txCount: Math.floor(Math.random() * 5) + 1,
        timestamp: new Date().toLocaleTimeString(),
        timestampFull: new Date().toISOString(),
        validator: 'validator-1',
        gasUsed: Math.floor(Math.random() * 50000) + 10000,
        gasLimit: 100000,
        transactions: []
    };
}

function getTransactions(count = 20) {
    const accounts = ['alice', 'bob', 'k:96b66bbddfec967660eb3fe727360b8b2697d516a9cb1733a3ccf949b524596b'];
    const transactions = [];
    const now = Date.now();

    for (let i = 0; i < count; i++) {
        const fromIdx = Math.floor(Math.random() * accounts.length);
        let toIdx = (fromIdx + 1) % accounts.length;

        transactions.push({
            hash: generateHash(),
            blockHeight: 100 - Math.floor(i / 3),
            from: accounts[fromIdx],
            to: accounts[toIdx],
            amount: Math.floor(Math.random() * 1000) + 100,
            nonce: i,
            gasUsed: 21000,
            gasPrice: 0.00000001,
            status: 'Success',
            timestamp: new Date(now - i * 2000).toLocaleTimeString(),
            timestampFull: new Date(now - i * 2000).toISOString(),
            type: Math.random() > 0.7 ? 'Token Transfer' : 'Native Transfer'
        });
    }

    return transactions;
}

function getTransactionByHash(hash) {
    return {
        hash: hash,
        blockHeight: 100,
        from: 'alice',
        to: 'bob',
        amount: 500,
        nonce: 0,
        gasUsed: 21000,
        gasPrice: 0.00000001,
        fee: 0.00021,
        status: 'Success',
        timestamp: new Date().toLocaleTimeString(),
        timestampFull: new Date().toISOString(),
        type: 'Native Transfer'
    };
}

function getBlockTransactions(height) {
    return getTransactions(3).map(tx => ({ ...tx, blockHeight: parseInt(height) }));
}
