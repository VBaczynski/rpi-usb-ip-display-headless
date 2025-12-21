#!/bin/bash
#
# setup-ip-display.sh
# USB IP Display setup script for headless Raspberry Pi servers
#
# Usage:
#   curl -sSL https://raw.githubusercontent.com/.../setup-ip-display.sh | sudo bash
# or:
#   sudo bash setup-ip-display.sh
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  USB IP Display - Setup Script${NC}"
echo -e "${GREEN}  For headless Raspberry Pi servers${NC}"
echo -e "${GREEN}========================================${NC}"
echo

# Check root
if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}Error: Please run with sudo${NC}"
  echo "  sudo bash $0"
  exit 1
fi

# ============================================
# 1. Create the IP sender script
# ============================================
echo -e "${YELLOW}[1/4] Creating script...${NC}"

cat > /usr/local/bin/usb-ip-display.sh << 'SCRIPT'
#!/bin/bash
#
# usb-ip-display.sh
# Sends IP addresses to USB display
#

LOG_TAG="usb-ip-display"

log() {
    logger -t "$LOG_TAG" "$1"
}

# Find serial port for Pico
# VID 239a = Adafruit (TinyUSB), VID 2e8a = Raspberry Pi (Pico SDK)
find_display_port() {
    for dev in /dev/ttyACM*; do
        if [ -e "$dev" ]; then
            # Check VID via udevadm
            local vid=$(udevadm info -q property -n "$dev" 2>/dev/null | grep "ID_VENDOR_ID" | cut -d= -f2)
            if [ "$vid" = "239a" ] || [ "$vid" = "2e8a" ]; then
                echo "$dev"
                return 0
            fi
        fi
    done
    return 1
}

# Send data to display
send_to_display() {
    local port="$1"
    
    # Configure port
    stty -F "$port" 115200 cs8 -cstopb -parenb -echo 2>/dev/null || return 1
    
    # Hostname
    echo "HOST:$(hostname)" > "$port"
    
    # Get IP addresses
    while IFS= read -r line; do
        local iface=$(echo "$line" | awk '{print $2}')
        local ip=$(echo "$line" | awk '{print $4}')
        
        # Skip loopback
        [[ "$iface" == "lo" ]] && continue
        
        case "$iface" in
            wlan*|wl*)
                echo "WIFI:$ip" > "$port"
                ;;
            eth*|en*|enp*|ens*)
                echo "ETH:$ip" > "$port"
                ;;
            *)
                echo "OTHER:$ip" > "$port"
                ;;
        esac
        
        # Small delay between commands
        sleep 0.1
        
    done < <(ip -o -4 addr show scope global 2>/dev/null)
    
    log "Data sent to $port"
}

# Main logic
main() {
    log "Starting..."
    
    # Delay for USB initialization
    sleep 2
    
    local port=$(find_display_port)
    
    if [ -z "$port" ]; then
        log "USB display not found"
        exit 1
    fi
    
    log "Found display: $port"
    send_to_display "$port"
}

main "$@"
SCRIPT

chmod +x /usr/local/bin/usb-ip-display.sh
echo -e "${GREEN}   ✓ /usr/local/bin/usb-ip-display.sh${NC}"

# ============================================
# 2. Create udev rules
# ============================================
echo -e "${YELLOW}[2/4] Creating udev rules...${NC}"

cat > /etc/udev/rules.d/99-usb-ip-display.rules << 'UDEV'
# USB IP Display - auto-start on connection
# VID 239a = Adafruit (TinyUSB)
ACTION=="add", SUBSYSTEM=="tty", ATTRS{idVendor}=="239a", TAG+="systemd", ENV{SYSTEMD_WANTS}="usb-ip-display.service"
# VID 2e8a = Raspberry Pi (Pico SDK)
ACTION=="add", SUBSYSTEM=="tty", ATTRS{idVendor}=="2e8a", TAG+="systemd", ENV{SYSTEMD_WANTS}="usb-ip-display.service"
UDEV

echo -e "${GREEN}   ✓ /etc/udev/rules.d/99-usb-ip-display.rules${NC}"

# ============================================
# 3. Create systemd service
# ============================================
echo -e "${YELLOW}[3/4] Creating systemd service...${NC}"

cat > /etc/systemd/system/usb-ip-display.service << 'SERVICE'
[Unit]
Description=USB IP Display - send IP addresses to USB display
After=network-online.target
Wants=network-online.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/usb-ip-display.sh
RemainAfterExit=no
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
SERVICE

echo -e "${GREEN}   ✓ /etc/systemd/system/usb-ip-display.service${NC}"

# ============================================
# 4. Create refresh timer
# ============================================
echo -e "${YELLOW}[4/4] Creating refresh timer...${NC}"

cat > /etc/systemd/system/usb-ip-display-refresh.service << 'SERVICE'
[Unit]
Description=USB IP Display - periodic refresh
After=network-online.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/usb-ip-display.sh
SERVICE

cat > /etc/systemd/system/usb-ip-display-refresh.timer << 'TIMER'
[Unit]
Description=USB IP Display - refresh timer (every 30 seconds)

[Timer]
OnBootSec=30
OnUnitActiveSec=30
AccuracySec=5

[Install]
WantedBy=timers.target
TIMER

echo -e "${GREEN}   ✓ /etc/systemd/system/usb-ip-display-refresh.timer${NC}"

# ============================================
# Activation
# ============================================
echo
echo -e "${YELLOW}Activating services...${NC}"

# Reload udev
udevadm control --reload-rules
udevadm trigger

# Reload systemd
systemctl daemon-reload
systemctl enable usb-ip-display-refresh.timer
systemctl start usb-ip-display-refresh.timer

echo
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Installation complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo
echo "The USB display will now automatically show hostname"
echo "and IP addresses when connected."
echo
echo "IP addresses refresh every 30 seconds."
echo
echo -e "${YELLOW}Management commands:${NC}"
echo "  sudo systemctl status usb-ip-display-refresh.timer  # timer status"
echo "  sudo /usr/local/bin/usb-ip-display.sh               # manual run"
echo "  journalctl -t usb-ip-display                        # view logs"
echo
echo -e "${YELLOW}To uninstall:${NC}"
echo "  sudo systemctl disable usb-ip-display-refresh.timer"
echo "  sudo rm /etc/udev/rules.d/99-usb-ip-display.rules"
echo "  sudo rm /etc/systemd/system/usb-ip-display*.service"
echo "  sudo rm /etc/systemd/system/usb-ip-display*.timer"
echo "  sudo rm /usr/local/bin/usb-ip-display.sh"
echo
