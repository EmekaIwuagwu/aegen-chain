#!/bin/bash
# Aegen L2 - One-Line Deployment
# Usage: curl -sSL https://raw.githubusercontent.com/YOUR_REPO/main/deploy/quick_deploy.sh | bash

set -e

echo "🔷 AEGEN L2 Quick Deploy"
echo "========================"

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash $0"
    exit 1
fi

# Install dependencies
echo "[1/5] Installing dependencies..."
apt-get update -qq
apt-get install -y -qq curl git build-essential cmake

# Install Docker if not present
if ! command -v docker &> /dev/null; then
    echo "[2/5] Installing Docker..."
    curl -fsSL https://get.docker.com | sh
    systemctl enable docker
    systemctl start docker
else
    echo "[2/5] Docker already installed ✓"
fi

# Install Docker Compose if not present
if ! command -v docker-compose &> /dev/null; then
    echo "[3/5] Installing Docker Compose..."
    curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    chmod +x /usr/local/bin/docker-compose
else
    echo "[3/5] Docker Compose already installed ✓"
fi

# Setup directory
echo "[4/5] Setting up Aegen..."
cd /opt
if [ -d "aegen" ]; then
    cd aegen
    git pull 2>/dev/null || true
else
    # If no git, just create directory
    mkdir -p aegen
    cd aegen
fi

# Check if files exist
if [ ! -f "Dockerfile" ]; then
    echo ""
    echo "⚠️  Please copy your Aegen project files to /opt/aegen"
    echo "   Required files: Dockerfile, docker-compose.testnet.yml"
    echo ""
    echo "   Example using SCP:"
    echo "   scp -r ./aegen-blockchain/* root@$(curl -s ifconfig.me):/opt/aegen/"
    exit 1
fi

# Build and start
echo "[5/5] Building and starting testnet..."
docker build -t aegen-node:latest .
docker-compose -f docker-compose.testnet.yml up -d

# Get IP
PUBLIC_IP=$(curl -s ifconfig.me)

echo ""
echo "========================================"
echo "✅ AEGEN L2 TESTNET DEPLOYED!"
echo "========================================"
echo ""
echo "Validator 1: http://$PUBLIC_IP:8545"
echo "Validator 2: http://$PUBLIC_IP:8546"
echo "Validator 3: http://$PUBLIC_IP:8547"
echo "Explorer:    http://$PUBLIC_IP:3000"
echo ""
echo "Test with:"
echo "  curl -X POST http://$PUBLIC_IP:8545 -H 'Content-Type: application/json' -d '{\"jsonrpc\":\"2.0\",\"method\":\"getChainInfo\",\"params\":{},\"id\":1}'"
echo ""
