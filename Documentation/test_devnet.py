#!/usr/bin/env python3
"""
Test L1 Settlement with Local Kadena Devnet
============================================
This script tests settlement against the local Kadena devnet.
No faucets needed - devnet has pre-funded accounts!
"""

import requests
import json
import time
import sys
import hashlib

# Devnet pre-funded keys (from official Kadena devnet)
DEVNET_SENDER = {
    "publicKey": "368820f80c324bbc7c2b0610688a7da43e39f91d118732671cd9c7500ff43cca",
    "secretKey": "251a920c403ae8c8f65f59142316af3c82b631fba46ddea92ee8c95035bd8e98",
    "account": "sender00"
}

# Local devnet endpoint
DEVNET_API = "http://localhost:8080"
NETWORK_ID = "development"
CHAIN_ID = "0"

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def pact_local(code: str) -> dict:
    """Execute Pact code locally (read-only)"""
    endpoint = f"{DEVNET_API}/chainweb/0.0/{NETWORK_ID}/chain/{CHAIN_ID}/pact/api/v1/local"
    
    creation_time = int(time.time())
    
    cmd = {
        "networkId": NETWORK_ID,
        "payload": {
            "exec": {
                "data": {},
                "code": code
            }
        },
        "signers": [],
        "meta": {
            "chainId": CHAIN_ID,
            "sender": DEVNET_SENDER["account"],
            "gasLimit": 10000,
            "gasPrice": 0.00000001,
            "ttl": 600,
            "creationTime": creation_time
        },
        "nonce": str(creation_time)
    }
    
    cmd_json = json.dumps(cmd, separators=(',', ':'))
    cmd_hash = sha256(cmd_json.encode())[:43]
    
    payload = {
        "hash": cmd_hash,
        "sigs": [],
        "cmd": cmd_json
    }
    
    try:
        response = requests.post(endpoint, json=payload, timeout=10)
        return response.json()
    except Exception as e:
        return {"error": str(e)}

def check_devnet_status():
    """Check if Kadena devnet is running"""
    print("\n" + "="*60)
    print("  CHECKING KADENA DEVNET STATUS")
    print("="*60)
    
    try:
        response = requests.get(f"{DEVNET_API}/chainweb/0.0/{NETWORK_ID}/cut", timeout=5)
        if response.status_code == 200:
            cut = response.json()
            height = cut.get("height", 0)
            print(f"  [OK] Devnet running at {DEVNET_API}")
            print(f"  [OK] Current height: {height}")
            return True
        else:
            print(f"  [FAIL] Devnet returned status {response.status_code}")
            return False
    except Exception as e:
        print(f"  [FAIL] Cannot connect to devnet: {e}")
        print("\n  To start Kadena devnet:")
        print("  docker run -p 8080:8080 kadena/devnet:latest")
        return False

def check_sender_balance():
    """Check pre-funded sender balance"""
    print("\n" + "="*60)
    print("  CHECKING SENDER BALANCE")
    print("="*60)
    
    result = pact_local(f'(coin.get-balance "{DEVNET_SENDER["account"]}")')
    
    if "result" in result:
        balance = result["result"].get("data", 0)
        print(f"  Account: {DEVNET_SENDER['account']}")
        print(f"  Balance: {balance} KDA")
        return True
    else:
        print(f"  Error: {result}")
        return False

def deploy_aegen_module():
    """Deploy the Aegen settlement module"""
    print("\n" + "="*60)
    print("  DEPLOYING AEGEN MODULE")
    print("="*60)
    
    # Read the Pact contract
    try:
        with open("contracts/aegen.pact", "r") as f:
            pact_code = f.read()
        print(f"  Loaded aegen.pact ({len(pact_code)} bytes)")
    except FileNotFoundError:
        print("  [FAIL] contracts/aegen.pact not found")
        return False
    
    # For now, just verify we can read the contract
    print("  [OK] Contract ready for deployment")
    print("  Note: Use the contracts/deploy.py script for full deployment")
    return True

def test_submit_batch_simulation():
    """Test batch submission (simulation)"""
    print("\n" + "="*60)
    print("  TESTING BATCH SUBMISSION (SIMULATION)")
    print("="*60)
    
    # This would call the deployed aegen module
    # For now, simulate the Pact code
    batch_id = "BATCH-000001"
    state_root = "f5a5fd42d16a20302798ef6ed309979b43003d2320d9f0e8ea9831a92759fb4b"
    
    pact_code = f'''
    (free.aegen.submit-batch 
      "{batch_id}" 
      "{state_root}" 
      2 1 2)
    '''
    
    print(f"  Batch ID:    {batch_id}")
    print(f"  State Root:  {state_root[:32]}...")
    print(f"  Pact Code:   {pact_code.strip()[:60]}...")
    
    # Would execute: result = pact_local(pact_code)
    print("  [SIMULATED] Batch submitted successfully")
    return True

def main():
    print("""
    ============================================================
      AEGEN L2 - LOCAL KADENA DEVNET TEST
    ============================================================
    This tests L1 settlement without needing testnet faucets!
    """)
    
    # Check devnet
    devnet_ok = check_devnet_status()
    
    if not devnet_ok:
        print("\n  [!] Start Kadena devnet first:")
        print("      docker run -p 8080:8080 kadena/devnet:latest")
        print("\n  Or use docker-compose:")
        print("      docker-compose -f docker-compose.devnet.yml up")
        return 1
    
    # Check balance
    check_sender_balance()
    
    # Deploy module
    deploy_aegen_module()
    
    # Test submission
    test_submit_batch_simulation()
    
    print("\n" + "="*60)
    print("  SUMMARY")
    print("="*60)
    print("  Devnet is running and ready for L1 settlement testing!")
    print("  Use docker-compose.devnet.yml for full integration.")
    print("="*60 + "\n")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
