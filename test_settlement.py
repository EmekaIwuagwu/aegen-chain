import requests
import time
import json

RPC_URL = "http://localhost:8545"

def send_rpc(method, params=[]):
    payload = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": method,
        "params": params
    }
    try:
        response = requests.post(RPC_URL, json=payload).json()
        return response
    except Exception as e:
        print(f"RPC Error: {e}")
        return None

def main():
    print("Waiting for node to start...")
    time.sleep(2)

    print("Sending Transaction 1...")
    # Using the proprietary sendTransaction for simplicity as we have valid checks there too?
    # Or start testing eth_sendRawTransaction? 
    # Let's use the simple one first to ensure block production triggers.
    # Actually, main.cpp genesis has "alice" with balance.
    
    tx1 = {
        "sender": "alice",
        "receiver": "bob",
        "amount": "100", # String to be safe
        "nonce": "0",
        "gasLimit": "21000",
        "gasPrice": "1",
        "data": "",
        "signature": "" 
    }
    
    # RPC server expects params to be a list or object. 
    # If our C++ server implementation passes the raw 'params' JSON string to the handler,
    # then passing `[tx1]` means the handler receives `[{"sender":...}]`.
    # extractJsonValue should find "sender" inside.
    
    res1 = send_rpc("sendTransaction", [tx1])
    print(f"Tx1 Response: {res1}")

    print("Waiting for Block 1 production...")
    time.sleep(11) 
    
    print("Sending Transaction 2 (Nonce 1)...")
    tx2 = tx1.copy()
    tx2["nonce"] = "1"
    res2 = send_rpc("sendTransaction", [tx2])
    print(f"Tx2 Response: {res2}")

    print("Waiting for Block 2 production and Batch Settlement...")
    time.sleep(5)
    
    # Fee Verification Logic
    print("\n--- Fee Market Verification ---")
    
    # 1. Check Sender (Alice) Balance
    res = send_rpc("getBalance", [{"account": "alice"}])
    print(f"Alice Balance: {res}")
    
    # 2. Check Receiver (Bob) Balance
    res = send_rpc("getBalance", [{"account": "bob"}])
    print(f"Bob Balance: {res}")

    # 3. Check Miner (Coinbase) - nodeAddress is generated in main.cpp, we don't know it easily via RPC unless we expose it.
    # The logs show [PROPOSER] I am Leader (node-1). Proposing...
    # The address is derived from node keys.
    # We can check if a "node-1" account exists or "k:..."
    # Main.cpp: Leader leader(..., keys.address);
    # keys generated via Wallet::generateKeyPair().
    # It's random every run!
    # We can't verify miner balance easily without knowing the random address.
    # However, Alice and Bob deterministic balances are good enough proof of fee deduction.
    
    print("\nTest complete. Check node logs for L1 SETTLEMENT.")

if __name__ == "__main__":
    main()
