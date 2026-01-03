const http = require('http');

const AEGEN_RPC = process.env.AEGEN_RPC || 'http://138.68.65.46:8545';
const RELAYER_PORT = 3001;

async function rpcCall(method, params) {
    try {
        const body = JSON.stringify({ jsonrpc: '2.0', method, params, id: Date.now() });
        const response = await fetch(AEGEN_RPC, {
            method: 'POST',
            body,
            headers: { 'Content-Type': 'application/json' }
        });
        const json = await response.json();
        return json.result ? json.result : { error: json.error };
    } catch (e) {
        return { error: 'Node Connection Failed' };
    }
}

const server = http.createServer(async (req, res) => {
    // Enable CORS for Wallet
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') { res.statusCode = 200; return res.end(); }

    if (req.url === '/bridge' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', async () => {
            try {
                const data = JSON.parse(body);
                console.log(`\n[Relayer] Incoming Deposit: ${data.amount} KDA -> ${data.receiver}`);

                // 1. Verify L1 Transaction (Mocking Chainweb Verification)
                process.stdout.write(`[Relayer] Verifying Kadena Chain 1 Proof... `);
                await new Promise(r => setTimeout(r, 1500)); // Simulate Network Latency
                console.log(`Verified! [OK]`);

                // 2. Submit to Aegen L2 Node
                console.log(`[Relayer] Submitting Proof to Aegen L2 Node...`);

                // Try 'bridgeDeposit' (New Protocol)
                let result = await rpcCall('bridgeDeposit', {
                    l1Hash: data.l1Hash,
                    amount: data.amount,
                    receiver: data.receiver,
                    token: data.tokenModule
                });

                // Fallback for Development (if node not updated)
                if (result.error) {
                    console.log(`[Relayer] Protocol Update Pending. Using fallback minting.`);
                    result = await rpcCall('mint', {
                        token: data.tokenModule,
                        account: data.receiver,
                        minter: data.receiver,
                        amount: data.amount
                    });
                }

                if (result && result.status === 'success') {
                    console.log(`[Relayer] Bridge Success! L2 Tx: ${result.tx || result.requestKey || 'Confirmed'}`);
                    res.statusCode = 200;
                    res.end(JSON.stringify({ success: true, tx: result.tx }));
                } else {
                    console.log(`[Relayer] Failed:`, result);
                    res.statusCode = 500;
                    res.end(JSON.stringify(result));
                }

            } catch (e) {
                console.error(e);
                res.statusCode = 500;
                res.end(JSON.stringify({ error: e.message }));
            }
        });
        return;
    }

    res.statusCode = 404;
    res.end();
});

server.listen(RELAYER_PORT, () => {
    console.log(`\n=================================================`);
    console.log(`   KADENA <> AEGEN RELAYER SERVICE (v1.0)`);
    console.log(`=================================================`);
    console.log(`Listening on port ${RELAYER_PORT}`);
    console.log(`Mode: Local Development (Simulated L1)`);
    console.log(`Target L2: ${AEGEN_RPC}`);
});
