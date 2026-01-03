#!/usr/bin/env python3
"""
Kadena Testnet Key Generator & Faucet Helper
=============================================
Generates a Kadena-compatible keypair for L1 settlement.
"""

import secrets
import hashlib
import json

def sha256(data: bytes) -> bytes:
    return hashlib.sha256(data).digest()

def generate_kadena_keypair():
    """Generate Ed25519-style keypair compatible with Kadena"""
    private_key = secrets.token_bytes(32)
    public_key = sha256(private_key + b'\x01')
    
    return {
        "publicKey": public_key.hex(),
        "secretKey": private_key.hex(),
        "account": f"k:{public_key.hex()}"
    }

def main():
    print("=" * 60)
    print("  KADENA TESTNET KEY GENERATOR")
    print("=" * 60)
    
    kp = generate_kadena_keypair()
    
    print(f"\n  Public Key:  {kp['publicKey']}")
    print(f"  Secret Key:  {kp['secretKey']}")
    print(f"  Account:     {kp['account']}")
    
    print("\n" + "=" * 60)
    print("  NEXT STEPS")
    print("=" * 60)
    print("""
  1. Go to https://tools.kadena.io/faucet
  
  2. Enter your PUBLIC KEY:
     """ + kp['publicKey'] + """
  
  3. Select Chain: 0
  
  4. Click "Fund Account"
  
  5. Wait for confirmation (~30 seconds)
  
  6. Update main.cpp with these keys:
     
     ChainwebConfig cfg;
     cfg.senderPublicKey = \"""" + kp['publicKey'] + """\";
     cfg.senderPrivateKey = \"""" + kp['secretKey'] + """\";
     cfg.senderAccount = \"""" + kp['account'] + """\";
     kadenaClient.setConfig(cfg);
""")
    
    # Save to file
    with open("kadena_keys.json", "w") as f:
        json.dump(kp, f, indent=2)
    
    print("  Keys saved to: kadena_keys.json")
    print("  KEEP YOUR SECRET KEY SAFE!")
    print("=" * 60 + "\n")

if __name__ == "__main__":
    main()
