#!/bin/bash
# Aegen L2 - Testnet Verification Script
# Run this after deployment to verify all nodes are working

set -e

RPC_PORTS=(8545 8546 8547)
SERVER_IP="${1:-localhost}"

echo "========================================"
echo "  AEGEN L2 TESTNET VERIFICATION"
echo "========================================"
echo "Server: $SERVER_IP"
echo ""

# Test each validator
for port in "${RPC_PORTS[@]}"; do
    echo ""
    echo "--- Testing Validator on port $port ---"
    
    # Chain Info
    result=$(curl -s -X POST "http://$SERVER_IP:$port" \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getChainInfo","params":{},"id":1}')
    
    if echo "$result" | grep -q "aegen-l2"; then
        echo "✓ Chain Info: OK"
        blockHeight=$(echo "$result" | grep -o '"blockHeight":[0-9]*' | cut -d':' -f2)
        echo "  Block Height: $blockHeight"
    else
        echo "✗ Chain Info: FAILED"
        echo "  Response: $result"
    fi
    
    # Generate Wallet
    result=$(curl -s -X POST "http://$SERVER_IP:$port" \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"generateWallet","params":{},"id":2}')
    
    if echo "$result" | grep -q '"address":"k:'; then
        echo "✓ Wallet Generation: OK"
    else
        echo "✗ Wallet Generation: FAILED"
    fi
    
    # Get Balance
    result=$(curl -s -X POST "http://$SERVER_IP:$port" \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getBalance","params":{"account":"alice"},"id":3}')
    
    if echo "$result" | grep -q '"balance":'; then
        balance=$(echo "$result" | grep -o '"balance":[0-9]*' | cut -d':' -f2)
        echo "✓ Balance Query: OK (alice: $balance)"
    else
        echo "✗ Balance Query: FAILED"
    fi
done

echo ""
echo "========================================"
echo "  CROSS-NODE TRANSACTION TEST"
echo "========================================"

# Send transaction via node 1
echo "Sending transaction via Validator 1..."
result=$(curl -s -X POST "http://$SERVER_IP:8545" \
    -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","method":"sendTransaction","params":{"from":"alice","to":"bob","amount":100},"id":4}')

if echo "$result" | grep -q '"requestKey"'; then
    txHash=$(echo "$result" | grep -o '"requestKey":"[^"]*"' | cut -d'"' -f4)
    echo "✓ Transaction sent: ${txHash:0:32}..."
else
    echo "✗ Transaction failed: $result"
fi

# Wait for block
echo "Waiting for block production (6s)..."
sleep 6

# Check balance on node 2
echo "Checking Bob's balance via Validator 2..."
result=$(curl -s -X POST "http://$SERVER_IP:8546" \
    -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","method":"getBalance","params":{"account":"bob"},"id":5}')

if echo "$result" | grep -q '"balance":'; then
    balance=$(echo "$result" | grep -o '"balance":[0-9]*' | cut -d':' -f2)
    echo "✓ Bob's balance on Node 2: $balance"
else
    echo "✗ Balance check failed"
fi

echo ""
echo "========================================"
echo "  EXPLORER & MONITORING"
echo "========================================"

# Check explorer
if curl -s -f "http://$SERVER_IP:3000" > /dev/null 2>&1; then
    echo "✓ Block Explorer: http://$SERVER_IP:3000"
else
    echo "✗ Block Explorer: Not responding"
fi

# Check Prometheus
if curl -s -f "http://$SERVER_IP:9090" > /dev/null 2>&1; then
    echo "✓ Prometheus: http://$SERVER_IP:9090"
else
    echo "○ Prometheus: Not configured"
fi

# Check Grafana
if curl -s -f "http://$SERVER_IP:3001" > /dev/null 2>&1; then
    echo "✓ Grafana: http://$SERVER_IP:3001"
else
    echo "○ Grafana: Not configured"
fi

echo ""
echo "========================================"
echo "  VERIFICATION COMPLETE"
echo "========================================"
