# 🚀 Aegen L2 Deployment Guide for Novices

This guide will walk you through deploying the Aegen Layer-2 Blockchain on a DigitalOcean droplet (virtual server). You don't need to be an expert! Just follow these steps carefully.

---

## 📋 Prerequisites

1.  **DigitalOcean Account**: If you don't have one, sign up at [digitalocean.com](https://digitalocean.com).
2.  **Terminal/Command Prompt**:
    *   **Windows**: Use PowerShell or PuTTY.
    *   **Mac/Linux**: Use the built-in Terminal.

---

## 🛠️ Step 1: Create a DigitalOcean Droplet (Server)

1.  Log in to DigitalOcean.
2.  Click **Create** -> **Droplets**.
3.  **Choose Region**: Select the one closest to you (e.g., New York, London).
4.  **Choose Image**: Select **Ubuntu 24.04 (LTS)** or **22.04 (LTS)**.
5.  **Choose Size**:
    *   Basic
    *   Regular
    *   **$6/month** (1 GB / 1 CPU) is enough for testing. For better performance, choose **$12/month** (2 GB RAM).
6.  **Authentication Method**: Choose **Password** and create a strong root password.
7.  **Hostname**: Name it `aegen-node-1`.
8.  Click **Create Droplet**.

Wait a minute for it to start. Copy the **IP Address** (e.g., `164.92.123.45`).

---

## 💻 Step 2: Connect to Your Server

Open your terminal on your computer.

**Command:**
```bash
ssh root@YOUR_DROPLET_IP
```
*Replace `YOUR_DROPLET_IP` with the actual IP address.*

**Example:** `ssh root@164.92.123.45`

*   Type `yes` if asked about fingerprints.
*   Enter the password you created.

---

## 🏗️ Step 3: Fast Deployment (The Easy Way)

Once logged in, run these commands one by one:

**1. Install Git:**
```bash
apt update && apt install -y git
```

**2. Clone the Repository:**
```bash
git clone https://github.com/EmekaIwuagwu/aegen-chain.git /opt/aegen
cd /opt/aegen
```

**3. Run the Auto-Setup Script:**
```bash
chmod +x deploy/quick_deploy.sh
./deploy/quick_deploy.sh
```
*Type `y` (Yes) if it asks to install Docker.*

**This script will:**
*   Install Docker & Docker Compose.
*   Build the Aegen Node software from C++ source code.
*   Start the entire network (Validator node, Explorer, Monitoring tools).

**Wait Time:** This might take 5-10 minutes. Grab a coffee! ☕

---

## 🌐 Step 4: Verify It Works

Once the script finishes, verify your node is running:

**Check Containers:**
```bash
docker ps
```
You should see 6 containers running (aegen-node-1, aegen-node-2, aegen-node-3, explorer, prometheus, grafana).

**Check Logs:**
```bash
docker logs -f aegen-node-1
```
You should see logs like `[INFO] Block produced at height...`. Press `Ctrl+C` to exit logs.

---

## 🖥️ Step 5: Access the Dashboard

Open your web browser (Chrome/Edge/Safari) and go to:

**http://YOUR_DROPLET_IP**

*Example:* `http://164.92.123.45`

You should see the **Aegen Scan** dashboard! 
*   **Status**: Should show "● Connected to RPC".
*   **Blocks**: You should see blocks increasing every few seconds.
*   **Tokens**: Click "Tokens" in the menu to see the Aegen Token.

---

## 🔧 Troubleshooting

**"I don't see the dashboard."**
*   Make sure you used `http://` and NOT `https://`.
*   DigitalOcean might block port 80. The explorer runs on port 80 by default in our setup.

**"The node stopped."**
*   Restart everything:
    ```bash
    cd /opt/aegen
    docker-compose -f docker-compose.testnet.yml restart
    ```

**"I want to update the code."**
*   Run:
    ```bash
    cd /opt/aegen
    git pull origin main
    docker-compose -f docker-compose.testnet.yml down
    docker build -t aegen-node:latest . --no-cache
    docker-compose -f docker-compose.testnet.yml up -d --force-recreate
    ```

---

## 🔑 Wallet & Tokens

*   **Create Token**: This is currently done via code (`main.cpp`) during Genesis.
*   **Transfer Tokens**: You can write scripts or use the future Wallet App to transfer tokens.
*   **View Tokens**: Go to the **Tokens** tab in the Explorer.

**Congratulations! You are now running an L2 Blockchain! 🚀**
