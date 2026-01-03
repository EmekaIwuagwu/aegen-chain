#!/bin/bash
# Aegen L2 - DigitalOcean Deployment Script
# This script sets up the testnet on a fresh Ubuntu 22.04 droplet

set -e

echo "========================================"
echo "  AEGEN L2 - DigitalOcean Setup"
echo "========================================"

# Update system
echo "[1/6] Updating system..."
apt-get update && apt-get upgrade -y

# Install Docker
echo "[2/6] Installing Docker..."
if ! command -v docker &> /dev/null; then
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
    rm get-docker.sh
    systemctl enable docker
    systemctl start docker
fi

# Install Docker Compose
echo "[3/6] Installing Docker Compose..."
if ! command -v docker-compose &> /dev/null; then
    curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    chmod +x /usr/local/bin/docker-compose
fi

# Create project directory
echo "[4/6] Setting up project..."
mkdir -p /opt/aegen
cd /opt/aegen

# Clone or copy files (if not already present)
if [ ! -f "docker-compose.testnet.yml" ]; then
    echo "Please copy the aegen-blockchain files to /opt/aegen"
    exit 1
fi

# Build Docker image
echo "[5/6] Building Aegen Docker image..."
docker build -t aegen-node:latest .

# Start testnet
echo "[6/6] Starting 3-node testnet..."
docker-compose -f docker-compose.testnet.yml up -d

# Wait for nodes to start
echo "Waiting for nodes to start..."
sleep 10

# Health check
echo ""
echo "========================================"
echo "  TESTNET STATUS"
echo "========================================"

for port in 8545 8546 8547; do
    if curl -s -f http://localhost:$port > /dev/null 2>&1; then
        echo "✓ Validator on port $port is running"
    else
        echo "✗ Validator on port $port is not responding"
    fi
done

echo ""
echo "========================================"
echo "  ACCESS POINTS"
echo "========================================"
echo "  Validator 1 RPC: http://$(curl -s ifconfig.me):8545"
echo "  Validator 2 RPC: http://$(curl -s ifconfig.me):8546"
echo "  Validator 3 RPC: http://$(curl -s ifconfig.me):8547"
echo "  Explorer:        http://$(curl -s ifconfig.me):3000"
echo "  Prometheus:      http://$(curl -s ifconfig.me):9090"
echo "  Grafana:         http://$(curl -s ifconfig.me):3001"
echo "========================================"
echo ""
echo "To view logs: docker-compose -f docker-compose.testnet.yml logs -f"
echo "To stop:      docker-compose -f docker-compose.testnet.yml down"
