#!/usr/bin/env python3
"""
Aegen L2 - Kadena Contract Deployment Script
=============================================
Deploys the aegen.pact settlement contract to Kadena testnet.
"""

import requests
import json
import time
import hashlib
import sys
from datetime import datetime

# Configuration
CHAINWEB_API = "https://api.testnet.chainweb.com"
NETWORK_ID = "testnet04"
CHAIN_ID = "0"

# Keys (replace with your Kadena testnet keys)
ADMIN_PUBLIC_KEY = "YOUR_ADMIN_PUBLIC_KEY"
ADMIN_SECRET_KEY = "YOUR_ADMIN_SECRET_KEY"
OPERATOR_PUBLIC_KEY = "YOUR_OPERATOR_PUBLIC_KEY"

def create_command(code: str, data: dict, caps: list = []):
    """Create Pact command payload"""
    creation_time = int(time.time())
    
    cmd = {
        "networkId": NETWORK_ID,
        "payload": {
            "exec": {
                "data": data,
                "code": code
            }
        },
        "signers": [
            {
                "pubKey": ADMIN_PUBLIC_KEY,
                "clist": caps if caps else [{"name": "coin.GAS", "args": []}]
            }
        ],
        "meta": {
            "chainId": CHAIN_ID,
            "sender": f"k:{ADMIN_PUBLIC_KEY}",
            "gasLimit": 100000,
            "gasPrice": 0.00000001,
            "ttl": 600,
            "creationTime": creation_time
        },
        "nonce": str(creation_time)
    }
    
    return json.dumps(cmd, separators=(',', ':'))

def hash_command(cmd: str) -> str:
    """Hash command for signing (base64url of blake2b)"""
    # Simplified - in production use proper blake2b
    return hashlib.sha256(cmd.encode()).hexdigest()[:43]

def send_command(cmd: str, sig: str) -> dict:
    """Send command to Chainweb"""
    cmd_hash = hash_command(cmd)
    
    payload = {
        "cmds": [{
            "hash": cmd_hash,
            "sigs": [{"sig": sig}],
            "cmd": cmd
        }]
    }
    
    url = f"{CHAINWEB_API}/chainweb/0.0/{NETWORK_ID}/chain/{CHAIN_ID}/pact/api/v1/send"
    
    print(f"Sending to: {url}")
    response = requests.post(url, json=payload, timeout=30)
    return response.json()

def poll_result(request_key: str) -> dict:
    """Poll for transaction result"""
    url = f"{CHAINWEB_API}/chainweb/0.0/{NETWORK_ID}/chain/{CHAIN_ID}/pact/api/v1/poll"
    
    for _ in range(30):
        response = requests.post(url, json={"requestKeys": [request_key]}, timeout=30)
        result = response.json()
        
        if request_key in result:
            return result[request_key]
        
        print("Waiting for confirmation...")
        time.sleep(2)
    
    return {"error": "Timeout"}

def deploy_keysets():
    """Deploy keysets"""
    print("\n=== Deploying Keysets ===")
    
    with open("contracts/keysets.pact", "r") as f:
        code = f.read()
    
    data = {
        "aegen-admin": {
            "keys": [ADMIN_PUBLIC_KEY],
            "pred": "keys-all"
        },
        "aegen-operators": {
            "keys": [ADMIN_PUBLIC_KEY, OPERATOR_PUBLIC_KEY],
            "pred": "keys-any"
        }
    }
    
    cmd = create_command(code, data)
    print(f"Command created")
    print(f"Data: {json.dumps(data, indent=2)}")
    
    # In production, sign with actual key
    sig = "placeholder_signature"
    
    return cmd

def deploy_module():
    """Deploy aegen module"""
    print("\n=== Deploying Aegen Module ===")
    
    with open("contracts/aegen.pact", "r") as f:
        code = f.read()
    
    data = {"upgrade": False}
    
    caps = [
        {"name": "coin.GAS", "args": []},
        {"name": "free.aegen.GOVERNANCE", "args": []}
    ]
    
    cmd = create_command(code, data, caps)
    print(f"Module code loaded ({len(code)} bytes)")
    
    return cmd

def initialize_module():
    """Initialize the module state"""
    print("\n=== Initializing Module ===")
    
    code = "(free.aegen.init)"
    data = {}
    
    caps = [
        {"name": "coin.GAS", "args": []},
        {"name": "free.aegen.GOVERNANCE", "args": []}
    ]
    
    cmd = create_command(code, data, caps)
    return cmd

def test_submit_batch():
    """Test batch submission"""
    print("\n=== Testing Batch Submission ===")
    
    code = '''
    (free.aegen.submit-batch 
      "BATCH-000001" 
      "abc123def456789012345678901234567890123456789012345678901234" 
      2 
      1 
      2)
    '''
    
    data = {}
    caps = [
        {"name": "coin.GAS", "args": []},
        {"name": "free.aegen.OPERATOR", "args": []}
    ]
    
    cmd = create_command(code.strip(), data, caps)
    return cmd

def main():
    print("="*60)
    print("  AEGEN L2 - KADENA CONTRACT DEPLOYMENT")
    print("="*60)
    print(f"Network: {NETWORK_ID}")
    print(f"Chain:   {CHAIN_ID}")
    print(f"API:     {CHAINWEB_API}")
    print("="*60)
    
    print("\n[!] This script generates deployment commands.")
    print("[!] In production, sign with actual Kadena keys.")
    print("[!] Use Chainweaver wallet for manual deployment.\n")
    
    # Generate deployment commands
    print("\n--- STEP 1: Deploy Keysets ---")
    keysets_cmd = deploy_keysets()
    
    print("\n--- STEP 2: Deploy Module ---")
    module_cmd = deploy_module()
    
    print("\n--- STEP 3: Initialize ---")
    init_cmd = initialize_module()
    
    print("\n--- STEP 4: Test Batch ---")
    test_cmd = test_submit_batch()
    
    # Save commands to files
    with open("contracts/deploy_keysets.json", "w") as f:
        f.write(keysets_cmd)
    
    with open("contracts/deploy_module.json", "w") as f:
        f.write(module_cmd)
    
    with open("contracts/deploy_init.json", "w") as f:
        f.write(init_cmd)
    
    print("\n" + "="*60)
    print("  DEPLOYMENT FILES GENERATED")
    print("="*60)
    print("  contracts/deploy_keysets.json")
    print("  contracts/deploy_module.json")
    print("  contracts/deploy_init.json")
    print("="*60)
    print("\nSign these commands with your Kadena keys and submit to testnet.")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
