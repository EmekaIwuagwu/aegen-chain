import requests
import json
import time

URL = "http://localhost:8545"

def rpc(method, params):
    payload = {
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
        "id": 1
    }
    try:
        response = requests.post(URL, json=payload).json()
        return response
    except Exception as e:
        return {"error": str(e)}

def send_tx(sender, receiver, amount, nonce):
    tx_params = {
        "sender": sender,
        "receiver": receiver,
        "amount": str(amount),
        "nonce": str(nonce)
    }
    return rpc("sendTransaction", tx_params)

def main():
    print("Waiting for node to be ready...")
    time.sleep(2)
    
    print("\n[1] Sending Tx 1 (Alice -> Bob)...")
    res1 = send_tx("alice", "bob", 100, 0)
    print("Tx1 Response:", res1)
    
    print("Waiting for Block 1 (5s)...")
    time.sleep(6)
    
    print("\n[2] Sending Tx 2 (Bob -> Alice)...")
    # Bob received 100 in prev block (if executed), but let's just use Bob's initial genesis balance to be safe
    res2 = send_tx("bob", "alice", 50, 0)
    print("Tx2 Response:", res2)

    print("Waiting for Block 2 (5s)...")
    time.sleep(6)
    
    # At this point, header height should have increased by 2.
    # Batch limit is 2. So we expect a Batch Settlement log in the node console.
    
    print("\n[3] Checking Final Balances...")
    print("Alice:", rpc("getBalance", {"address": "alice"}))
    print("Bob:", rpc("getBalance", {"address": "bob"}))
    
    print("\nCheck the node console! You should see '[Bridge] Batch ... settled successfully.'")

if __name__ == "__main__":
    main()
