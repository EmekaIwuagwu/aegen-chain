#!/usr/bin/env python3
"""
Aegen Blockchain - Kadena L2 CLI Wallet
========================================
Commands:
  wallet new           - Generate a new Kadena-compatible account
  wallet balance <acc> - Get account balance
  wallet send          - Send tokens
  wallet info          - Get chain info
"""

import argparse
import secrets
import hashlib
import json
import requests
import sys

RPC_URL = "http://localhost:8545"

def sha256(data: bytes) -> bytes:
    return hashlib.sha256(data).digest()

def generate_keypair():
    """Generate Kadena-compatible keypair"""
    private_key = secrets.token_bytes(32)
    # Derive public key (SHA256 of private + domain separator)
    public_key = sha256(private_key + b'\x01')
    # Kadena address format: k:<public-key-hex>
    address = "k:" + public_key.hex()
    
    return {
        "private_key": private_key.hex(),
        "public_key": public_key.hex(),
        "address": address
    }

def sign_message(message: bytes, private_key: bytes) -> bytes:
    """Deterministic signature compatible with Aegen node"""
    r = sha256(private_key + message)
    s = sha256(r + message)
    return r + s

def rpc_call(method: str, params: dict):
    """Make JSON-RPC call to node"""
    payload = {
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
        "id": 1
    }
    try:
        response = requests.post(RPC_URL, json=payload, timeout=10)
        return response.json()
    except requests.exceptions.ConnectionError:
        print("Error: Cannot connect to Aegen node at", RPC_URL)
        sys.exit(1)

def cmd_new():
    """Generate new Kadena-compatible account"""
    kp = generate_keypair()
    print("\n" + "="*70)
    print("          NEW KADENA-COMPATIBLE ACCOUNT GENERATED")
    print("="*70)
    print(f"\n  Account:     {kp['address']}")
    print(f"  Public Key:  {kp['public_key']}")
    print(f"  Private Key: {kp['private_key']}")
    print("\n" + "="*70)
    print("  SAVE YOUR PRIVATE KEY SECURELY!")
    print("  This is a k: prefixed Kadena-style account.")
    print("="*70 + "\n")

def cmd_balance(account: str):
    """Get balance for account"""
    result = rpc_call("getBalance", {"account": account})
    if "result" in result:
        print(f"\nBalance for {account}: {result['result']} AE\n")
    else:
        print(f"Error: {result}")

def cmd_send(sender: str, receiver: str, amount: int, nonce: int, private_key: str):
    """Send tokens"""
    # Create transaction data for signing
    tx_data = f"{sender}{receiver}{amount}{nonce}210001".encode()
    
    # Sign the transaction
    pk_bytes = bytes.fromhex(private_key)
    signature = sign_message(tx_data, pk_bytes)
    
    params = {
        "sender": sender,
        "receiver": receiver,
        "amount": str(amount),
        "nonce": str(nonce),
        "signature": signature.hex()
    }
    
    print(f"\nSending {amount} AE from {sender} to {receiver}...")
    result = rpc_call("sendTransaction", params)
    
    if "result" in result:
        req_key = result["result"].get("requestKey", result["result"])
        print(f"Transaction submitted!")
        print(f"Request Key: {req_key}\n")
    else:
        print(f"Error: {result}")

def cmd_info():
    """Get chain info"""
    result = rpc_call("getChainInfo", {})
    if "result" in result:
        info = result["result"]
        print("\n" + "="*50)
        print("       AEGEN L2 (KADENA) CHAIN STATUS")
        print("="*50)
        print(f"  Network ID:   {info.get('networkId', 'aegen-l2')}")
        print(f"  Chain ID:     {info.get('chainId', '0')}")
        print(f"  L1 Network:   {info.get('l1Network', 'kadena-mainnet')}")
        print(f"  Mempool Size: {info.get('mempoolSize', 0)} txs")
        print(f"  Token Count:  {info.get('tokenCount', 0)}")
        print("="*50 + "\n")
    else:
        print(f"Error: {result}")

def cmd_create_token(name: str, symbol: str, precision: int, supply: int, creator: str):
    """Create fungible token"""
    params = {
        "name": name,
        "symbol": symbol,
        "precision": str(precision),
        "initialSupply": str(supply),
        "creator": creator
    }
    
    print(f"\nDeploying fungible token: {name} ({symbol})...")
    result = rpc_call("createFungible", params)
    
    if "result" in result:
        module = result["result"].get("module", "unknown")
        print(f"Token deployed!")
        print(f"Module: {module}\n")
    else:
        print(f"Error: {result}")

def main():
    parser = argparse.ArgumentParser(
        description="Aegen L2 (Kadena) CLI Wallet",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # new
    subparsers.add_parser("new", help="Generate new Kadena-compatible account")
    
    # balance
    bal_parser = subparsers.add_parser("balance", help="Get account balance")
    bal_parser.add_argument("account", help="Account name (k:... or simple name)")
    
    # send
    send_parser = subparsers.add_parser("send", help="Send tokens")
    send_parser.add_argument("--from", dest="sender", required=True, help="Sender account")
    send_parser.add_argument("--to", dest="receiver", required=True, help="Receiver account")
    send_parser.add_argument("--amount", type=int, required=True, help="Amount to send")
    send_parser.add_argument("--nonce", type=int, required=True, help="Transaction nonce")
    send_parser.add_argument("--key", required=True, help="Private key (hex)")
    
    # info
    subparsers.add_parser("info", help="Get chain info")
    
    # create-token
    token_parser = subparsers.add_parser("create-token", help="Create fungible token")
    token_parser.add_argument("--name", required=True, help="Token name")
    token_parser.add_argument("--symbol", required=True, help="Token symbol")
    token_parser.add_argument("--precision", type=int, default=12, help="Precision (default: 12)")
    token_parser.add_argument("--supply", type=int, default=0, help="Initial supply")
    token_parser.add_argument("--creator", required=True, help="Creator account")
    
    args = parser.parse_args()
    
    if args.command == "new":
        cmd_new()
    elif args.command == "balance":
        cmd_balance(args.account)
    elif args.command == "send":
        cmd_send(args.sender, args.receiver, args.amount, args.nonce, args.key)
    elif args.command == "info":
        cmd_info()
    elif args.command == "create-token":
        cmd_create_token(args.name, args.symbol, args.precision, args.supply, args.creator)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
