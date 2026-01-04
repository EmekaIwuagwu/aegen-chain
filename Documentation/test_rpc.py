import requests
import json

url = "http://localhost:8545"

print("Testing RPC...")
try:
    response = requests.post(url, json={"method": "health", "params": {}}, timeout=5)
    print(f"Status Code: {response.status_code}")
    print(f"Response: {response.text}")
except Exception as e:
    print(f"Error: {e}")
