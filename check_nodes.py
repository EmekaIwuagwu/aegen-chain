import requests
import time

ports = [8545, 8546, 8547]

print("Checking 3 nodes (Wait 15s for syncing)...")
time.sleep(15)

for p in ports:
    try:
        url = f"http://localhost:{p}"
        res = requests.post(url, json={"method":"getChainInfo", "params":{}}, timeout=2)
        if res.status_code == 200:
            data = res.json()
            print(f"Node {p}: Height {data.get('result', {}).get('blockHeight', 'N/A')}")
        else:
            print(f"Node {p}: Error {res.status_code}")
    except Exception as e:
        print(f"Node {p}: Connection Error")
