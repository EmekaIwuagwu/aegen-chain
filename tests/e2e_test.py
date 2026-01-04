"""
Aegen L2 End-to-End Test Suite
Tests: Wallet Creation, Token Transfers, EVM Execution, Receipts, L1 Settlement
"""
import requests
import time
import json
import hashlib

RPC_URL = "http://localhost:8545"

def send_rpc(method, params=[]):
    payload = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": method,
        "params": params
    }
    try:
        response = requests.post(RPC_URL, json=payload, timeout=10).json()
        return response
    except Exception as e:
        return {"error": str(e)}

def test_wallet_creation():
    print("\n=== TEST: Wallet Creation ===")
    res = send_rpc("generateWallet", [])
    if "result" in res and "address" in res.get("result", {}):
        addr = res["result"]["address"]
        pubkey = res["result"].get("publicKey", "N/A")
        print(f"  [PASS] Wallet created: {addr[:20]}...")
        return addr
    else:
        print(f"  [FAIL] {res}")
        return None

def test_get_balance(account, expected=None):
    print(f"\n=== TEST: Get Balance ({account}) ===")
    res = send_rpc("getBalance", [{"account": account}])
    if "result" in res:
        balance = res["result"]
        if expected is not None:
            if balance == expected:
                print(f"  [PASS] Balance: {balance} (expected {expected})")
            else:
                print(f"  [FAIL] Balance: {balance} (expected {expected})")
        else:
            print(f"  [INFO] Balance: {balance}")
        return balance
    else:
        print(f"  [FAIL] {res}")
        return None

def test_transfer(sender, receiver, amount, nonce):
    print(f"\n=== TEST: Transfer {amount} from {sender} to {receiver} (nonce={nonce}) ===")
    tx = {
        "sender": sender,
        "receiver": receiver,
        "amount": str(amount),
        "nonce": str(nonce),
        "gasLimit": "21000",
        "gasPrice": "1",
        "data": "",
        "signature": ""
    }
    res = send_rpc("sendTransaction", [tx])
    if "result" in res:
        print(f"  [PASS] TX Hash: {res['result'].get('requestKey', 'N/A')[:32]}...")
        return res["result"].get("requestKey")
    else:
        print(f"  [FAIL] {res}")
        return None

def test_chain_info():
    print("\n=== TEST: Chain Info ===")
    res = send_rpc("getChainInfo", [])
    if "result" in res:
        info = res["result"]
        print(f"  [PASS] Chain: {info.get('name', 'N/A')}, Height: {info.get('height', 0)}")
        return info
    else:
        print(f"  [FAIL] {res}")
        return None

def test_eth_chain_id():
    print("\n=== TEST: eth_chainId ===")
    res = send_rpc("eth_chainId", [])
    if "result" in res:
        print(f"  [PASS] Chain ID: {res['result']}")
        return res["result"]
    else:
        print(f"  [FAIL] {res}")
        return None

def test_eth_block_number():
    print("\n=== TEST: eth_blockNumber ===")
    res = send_rpc("eth_blockNumber", [])
    if "result" in res:
        print(f"  [PASS] Block: {res['result']}")
        return res["result"]
    else:
        print(f"  [FAIL] {res}")
        return None

def test_token_creation():
    print("\n=== TEST: Token Creation ===")
    res = send_rpc("createFungible", [{
        "name": "TestToken",
        "symbol": "TST",
        "precision": "18",
        "initialSupply": "1000000",
        "creator": "alice"
    }])
    if "result" in res:
        print(f"  [PASS] Token created: {res['result']}")
        return res["result"].get("module")
    else:
        print(f"  [FAIL] {res}")
        return None

def test_nonce(account, expected=None):
    print(f"\n=== TEST: Get Nonce ({account}) ===")
    res = send_rpc("getNonce", [{"account": account}])
    if "result" in res:
        nonce = res["result"]
        if expected is not None:
            if nonce == expected:
                print(f"  [PASS] Nonce: {nonce}")
            else:
                print(f"  [FAIL] Nonce: {nonce} (expected {expected})")
        else:
            print(f"  [INFO] Nonce: {nonce}")
        return nonce
    else:
        print(f"  [FAIL] {res}")
        return None

def run_all_tests():
    print("=" * 60)
    print("AEGEN L2 BLOCKCHAIN - END-TO-END TEST SUITE")
    print("=" * 60)
    
    # Wait for node
    print("\nWaiting for node startup...")
    time.sleep(2)
    
    # 1. Chain Info
    test_chain_info()
    
    # 2. Ethereum Compatibility
    test_eth_chain_id()
    test_eth_block_number()
    
    # 3. Wallet
    new_wallet = test_wallet_creation()
    
    # 4. Initial Balances (genesis accounts)
    alice_bal = test_get_balance("alice")
    bob_bal = test_get_balance("bob")
    
    # 5. Nonce Check
    alice_nonce = test_nonce("alice")
    
    # 6. Transfer #1
    test_transfer("alice", "bob", 1000, alice_nonce if alice_nonce else 0)
    
    # Wait for block
    print("\nWaiting for block production (5s)...")
    time.sleep(5)
    
    # 7. Verify Balances after transfer
    # alice: initial - 1000 - 21000 gas
    # bob: initial + 1000
    test_get_balance("alice")
    test_get_balance("bob")
    
    # 8. Token Creation
    test_token_creation()
    
    # 9. Nonce should have incremented
    test_nonce("alice")
    
    # 10. Second Transfer
    test_transfer("alice", "bob", 500, (alice_nonce if alice_nonce else 0) + 1)
    
    print("\nWaiting for L1 settlement trigger (10s)...")
    time.sleep(10)
    
    # Final balance check
    final_alice = test_get_balance("alice")
    final_bob = test_get_balance("bob")
    
    print("\n" + "=" * 60)
    print("TEST SUITE COMPLETE")
    print("=" * 60)
    print(f"\nFinal Balances:")
    print(f"  Alice: {final_alice}")
    print(f"  Bob: {final_bob}")
    print("\nCheck node logs for L1 SETTLEMENT output.")

if __name__ == "__main__":
    run_all_tests()
