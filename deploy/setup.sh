#!/bin/bash
set -euo pipefail

# =============================================================
#  Industrial Workcell Server - One-Click Deployment Script
#  Tested on: Ubuntu 22.04 / 24.04 LTS (Tencent Cloud CVM)
# =============================================================

echo "================================================"
echo "  Industrial Workcell Server - Auto Deploy"
echo "================================================"

# --- 1. Install dependencies ---
echo "[1/6] Installing system dependencies..."
apt-get update
apt-get install -y --no-install-recommends \
    cmake ninja-build g++ git \
    qt6-base-dev qt6-base-dev-tools \
    ufw

# --- 2. Create service user ---
echo "[2/6] Creating service user..."
if ! id -u industrial &>/dev/null; then
    useradd -r -s /bin/false -d /opt/industrial industrial
fi
mkdir -p /opt/industrial
chown industrial:industrial /opt/industrial

# --- 3. Clone and build ---
echo "[3/6] Building from source..."
BUILD_DIR=$(mktemp -d)
cp -r "$(dirname "$0")/.." "$BUILD_DIR/src"
cd "$BUILD_DIR/src"

cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SERVER=ON

cmake --build build --parallel

# Run tests
echo "[3.5/6] Running tests..."
ctest --test-dir build --output-on-failure

# Install binary
cp build/industrial-server /opt/industrial/
chown industrial:industrial /opt/industrial/industrial-server
chmod 755 /opt/industrial/industrial-server

# Cleanup
rm -rf "$BUILD_DIR"

# --- 4. Install systemd service ---
echo "[4/6] Installing systemd service..."
cp "$(dirname "$0")/industrial-server.service" /etc/systemd/system/
systemctl daemon-reload
systemctl enable industrial-server

# --- 5. Configure firewall ---
echo "[5/6] Configuring firewall..."
ufw allow 22/tcp    # SSH
ufw allow 9600/tcp  # Industrial server
ufw --force enable

# --- 6. Start service ---
echo "[6/6] Starting service..."
systemctl start industrial-server
sleep 2

if systemctl is-active --quiet industrial-server; then
    echo ""
    echo "================================================"
    echo "  DEPLOYMENT SUCCESSFUL"
    echo "================================================"
    echo ""
    echo "  Server:   running on port 9600"
    echo "  Status:   systemctl status industrial-server"
    echo "  Logs:     journalctl -u industrial-server -f"
    echo "  Stop:     systemctl stop industrial-server"
    echo "  Restart:  systemctl restart industrial-server"
    echo ""
    PUBLIC_IP=$(curl -s http://metadata.tencentyun.com/latest/meta-data/public-ipv4 2>/dev/null || echo "<your-server-ip>")
    echo "  Mobile app connection: ${PUBLIC_IP}:9600"
    echo ""
else
    echo "[ERROR] Service failed to start!"
    systemctl status industrial-server
    exit 1
fi
