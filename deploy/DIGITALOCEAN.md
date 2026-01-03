# Aegen L2 - DigitalOcean Deployment Guide

## Quick Start

### 1. Create a Droplet

Create a DigitalOcean droplet with:
- **Image**: Ubuntu 22.04 LTS
- **Size**: 4GB RAM / 2 vCPUs minimum (or $24/month)
- **Datacenter**: Any region

### 2. Connect to the Droplet

```bash
ssh root@YOUR_DROPLET_IP
```

### 3. Clone/Upload the Project

Option A - Clone from Git (if hosted):
```bash
git clone https://github.com/YOUR_REPO/aegen-blockchain.git /opt/aegen
cd /opt/aegen
```

Option B - Upload via SCP:
```bash
# From your local machine:
scp -r ./aegen-blockchain root@YOUR_DROPLET_IP:/opt/aegen
```

### 4. Run the Setup Script

```bash
cd /opt/aegen
chmod +x deploy/digitalocean_setup.sh
./deploy/digitalocean_setup.sh
```

This will:
- Install Docker & Docker Compose
- Build the Aegen node image
- Start a 3-validator testnet
- Display access URLs

### 5. Verify the Deployment

```bash
chmod +x deploy/verify_testnet.sh
./deploy/verify_testnet.sh YOUR_DROPLET_IP
```

---

## Manual Docker Commands

### Build the image
```bash
docker build -t aegen-node:latest .
```

### Start testnet
```bash
docker-compose -f docker-compose.testnet.yml up -d
```

### View logs
```bash
docker-compose -f docker-compose.testnet.yml logs -f
```

### Restart a specific validator
```bash
docker-compose -f docker-compose.testnet.yml restart validator-1
```

### Stop everything
```bash
docker-compose -f docker-compose.testnet.yml down
```

### Rebuild and restart
```bash
docker-compose -f docker-compose.testnet.yml down
docker build -t aegen-node:latest . --no-cache
docker-compose -f docker-compose.testnet.yml up -d
```

---

## Access Points

| Service | Port | URL |
|---------|------|-----|
| Validator 1 RPC | 8545 | `http://IP:8545` |
| Validator 2 RPC | 8546 | `http://IP:8546` |
| Validator 3 RPC | 8547 | `http://IP:8547` |
| Block Explorer | 3000 | `http://IP:3000` |
| Prometheus | 9090 | `http://IP:9090` |
| Grafana | 3001 | `http://IP:3001` (admin/admin) |

---

## Firewall Setup

Open the required ports:
```bash
ufw allow 8545/tcp  # Validator 1 RPC
ufw allow 8546/tcp  # Validator 2 RPC
ufw allow 8547/tcp  # Validator 3 RPC
ufw allow 3000/tcp  # Explorer
ufw allow 9090/tcp  # Prometheus
ufw allow 3001/tcp  # Grafana
ufw enable
```

---

## Test RPC from Local Machine

```bash
# Get chain info
curl -X POST http://YOUR_IP:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getChainInfo","params":{},"id":1}'

# Generate wallet
curl -X POST http://YOUR_IP:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"generateWallet","params":{},"id":1}'

# Send transaction
curl -X POST http://YOUR_IP:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"sendTransaction","params":{"from":"alice","to":"bob","amount":100},"id":1}'
```

---

## Troubleshooting

### Container won't start
```bash
docker logs aegen-validator-1
```

### Port already in use
```bash
lsof -i :8545
kill -9 PID
```

### Out of memory
Use a larger droplet (8GB RAM recommended for production)

### Build fails
```bash
docker build -t aegen-node:latest . 2>&1 | tee build.log
```
