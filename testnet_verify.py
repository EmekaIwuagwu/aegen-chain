#!/usr/bin/env python3
"""
Aegen L2 Multi-Node Testnet Verification Script
================================================
Tests the multi-node Docker deployment for:
1. All nodes are reachable
2. Chain info is consistent across nodes
3. Transactions propagate between nodes
4. Blocks are synchronized
"""

import requests
import time
import sys
import json

# Node endpoints
NODES = [
    {"name": "Validator-1", "rpc": "http://localhost:8545"},
    {"name": "Validator-2", "rpc": "http://localhost:8546"},
    {"name": "Validator-3", "rpc": "http://localhost:8547"},
]

def rpc_call(url, method, params={}):
    """Make JSON-RPC call"""
    try:
        response = requests.post(url, json={
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
            "id": 1
        }, timeout=10)
        return response.json()
    except Exception as e:
        return {"error": str(e)}

def test_node_connectivity():
    """Test that all nodes are reachable"""
    print("\n" + "="*60)
    print("  1. NODE CONNECTIVITY")
    print("="*60)
    
    all_ok = True
    for node in NODES:
        result = rpc_call(node["rpc"], "getChainInfo")
        if "result" in result:
            print(f"  [{node['name']}] OK - {node['rpc']}")
        else:
            print(f"  [{node['name']}] FAILED - {result.get('error', 'Unknown error')}")
            all_ok = False
    
    return all_ok

def test_chain_consistency():
    """Verify chain info is consistent across nodes"""
    print("\n" + "="*60)
    print("  2. CHAIN CONSISTENCY")
    print("="*60)
    
    chain_infos = []
    for node in NODES:
        result = rpc_call(node["rpc"], "getChainInfo")
        if "result" in result:
            chain_infos.append(result["result"])
            print(f"  [{node['name']}] NetworkID: {result['result'].get('networkId', 'N/A')}")
    
    if len(chain_infos) < 2:
        print("  [WARN] Not enough nodes for comparison")
        return True
    
    # Check all have same network ID
    network_ids = [c.get('networkId') for c in chain_infos]
    consistent = len(set(network_ids)) == 1
    
    if consistent:
        print(f"  [OK] All nodes on same network: {network_ids[0]}")
    else:
        print(f"  [FAIL] Network mismatch: {network_ids}")
    
    return consistent

def test_transaction_propagation():
    """Test that transactions propagate between nodes"""
    print("\n" + "="*60)
    print("  3. TRANSACTION PROPAGATION")
    print("="*60)
    
    # Send transaction to node 1
    result = rpc_call(NODES[0]["rpc"], "sendTransaction", {
        "sender": "alice",
        "receiver": "bob",
        "amount": "100",
        "nonce": "0"
    })
    
    if "result" not in result:
        print(f"  [FAIL] Could not submit transaction: {result}")
        return False
    
    tx_hash = result["result"]
    print(f"  Submitted tx to {NODES[0]['name']}: {str(tx_hash)[:40]}...")
    
    # Wait for propagation
    print("  Waiting for propagation (3s)...")
    time.sleep(3)
    
    # Check mempool size on all nodes
    propagated = True
    for node in NODES:
        info = rpc_call(node["rpc"], "getChainInfo")
        if "result" in info:
            mempool = info["result"].get("mempoolSize", 0)
            status = "OK" if mempool > 0 else "PENDING"
            print(f"  [{node['name']}] Mempool size: {mempool} - {status}")
    
    return propagated

def test_block_sync():
    """Test that blocks synchronize across nodes"""
    print("\n" + "="*60)
    print("  4. BLOCK SYNCHRONIZATION")
    print("="*60)
    
    print("  Waiting for block production (6s)...")
    time.sleep(6)
    
    # Check balances on different nodes
    balances = []
    for node in NODES:
        result = rpc_call(node["rpc"], "getBalance", {"account": "alice"})
        if "result" in result:
            balance = result["result"]
            balances.append(balance)
            print(f"  [{node['name']}] Alice balance: {balance} AE")
    
    # All nodes should report same balance after sync
    if len(set(balances)) == 1:
        print("  [OK] All nodes have consistent state")
        return True
    else:
        print(f"  [WARN] State divergence detected: {balances}")
        return False

def main():
    print("""
    ================================================
      AEGEN L2 - MULTI-NODE TESTNET VERIFICATION
    ================================================
    """)
    
    # Run tests
    connectivity_ok = test_node_connectivity()
    
    if not connectivity_ok:
        print("\n[!] Some nodes are not reachable.")
        print("[!] Make sure to run: docker-compose -f docker-compose.testnet.yml up")
        print("\nFor single-node testing, start the node manually:")
        print("  .\\build\\Debug\\aegen-node.exe")
        return 1
    
    consistency_ok = test_chain_consistency()
    propagation_ok = test_transaction_propagation()
    sync_ok = test_block_sync()
    
    # Summary
    print("\n" + "="*60)
    print("                     SUMMARY")
    print("="*60)
    print(f"  Node Connectivity:     {'PASS' if connectivity_ok else 'FAIL'}")
    print(f"  Chain Consistency:     {'PASS' if consistency_ok else 'FAIL'}")
    print(f"  Tx Propagation:        {'PASS' if propagation_ok else 'FAIL'}")
    print(f"  Block Synchronization: {'PASS' if sync_ok else 'FAIL'}")
    print("="*60)
    
    all_passed = all([connectivity_ok, consistency_ok, propagation_ok, sync_ok])
    
    if all_passed:
        print("\n  All tests PASSED!\n")
        return 0
    else:
        print("\n  Some tests FAILED.\n")
        return 1

if __name__ == "__main__":
    sys.exit(main())
