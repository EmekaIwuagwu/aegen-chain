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

def main():
    print("Waiting for node...")
    time.sleep(2)
    
    # 1. Get Balance
    print("\n[1] Checking Balances...")
    print("Alice:", rpc("getBalance", {"address": "alice"}))
    print("Bob:", rpc("getBalance", {"address": "bob"}))

    # 2. Send Transaction
    print("\n[2] Sending 500 from Alice to Bob...")
    tx_params = {
        "sender": "alice",
        "receiver": "bob",
        "amount": "500",
        "nonce": "0"
    }
    res = rpc("sendTransaction", tx_params)
    print("Response:", res)

    print("\nWaiting for block execution (5s)...")
    time.sleep(6)

    # 3. Check Balance Again
    print("\n[3] Checking Balances again...")
    print("Alice:", rpc("getBalance", {"address": "alice"}))
    print("Bob:", rpc("getBalance", {"address": "bob"}))

if __name__ == "__main__":
    main()
