const RPC_URL = 'http://localhost:8545';

async function rpc(method, params = {}) {
    try {
        const response = await fetch(RPC_URL, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                jsonrpc: '2.0',
                method: method,
                params: params,
                id: 1
            })
        });
        const data = await response.json();
        return data.result;
    } catch (e) {
        console.error("RPC Error", e);
        return null; // Offline
    }
}

async function updateStats() {
    const info = await rpc("getChainInfo", {});
    if (info) {
        document.getElementById('chainId').innerText = info.chainId || "Unknown";
        document.getElementById('mempoolSize').innerText = info.mempoolSize || "0";
        // Mock height since endpoint doesn't return it yet (Phase 2 limitation)
        document.getElementById('blockHeight').innerText = "#" + Math.floor(Date.now() / 5000 - 170000000); 
    } else {
        document.getElementById('chainId').innerText = "Offline";
    }
}

// Mock txs for visualization until full history API 
// In Phase 3 (next), we will add getRecentTxs to RPC
const mockTxs = [];

function generateMockTx() {
    if (Math.random() > 0.3) return;
    const hash = "0x" + Array(64).fill(0).map(() => Math.floor(Math.random()*16).toString(16)).join('');
    mockTxs.unshift({
        hash: hash.substring(0, 16) + "...",
        from: "alice",
        to: "bob",
        amount: Math.floor(Math.random() * 1000),
        status: "Success"
    });
    if (mockTxs.length > 10) mockTxs.pop();
    renderTable();
}

function renderTable() {
    const tbody = document.getElementById('txTableBody');
    if (mockTxs.length === 0) return;
    
    tbody.innerHTML = mockTxs.map(tx => `
        <tr>
            <td class="hash">${tx.hash}</td>
            <td>${tx.from}</td>
            <td>${tx.to}</td>
            <td>${tx.amount} AE</td>
            <td><span class="status-tag">${tx.status}</span></td>
        </tr>
    `).join('');
}

// Init
setInterval(() => {
    updateStats();
    generateMockTx(); // Visual demo
}, 3000);

updateStats();
