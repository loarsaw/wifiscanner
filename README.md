# WiFi Scanner

A lightweight, efficient WiFi network scanner for Linux systems using the nl80211 kernel interface. Perfect for embedded systems like Raspberry Pi and desktop Linux distributions.

## Features

✅ **Auto-detects wireless interface** - Works with `wlan0`, `wlo1`, `wlp3s0`, etc.  
✅ **No external dependencies** - Uses native nl80211 (no NetworkManager required)  
✅ **Dual output formats** - JSON for automation, table for humans  
✅ **Comprehensive network info** - SSID, BSSID, signal strength, frequency, security  
✅ **Smart duplicate filtering** - Shows strongest signal per network  
✅ **Cross-platform** - Works on Ubuntu, Debian, Raspberry Pi OS, Arch, etc.

## Requirements

- Linux kernel 2.6.24+ (with nl80211 support)
- C++11 compiler (g++ or clang++)
- libnl-3 and libnl-genl-3 libraries
- Root privileges (for WiFi scanning)

## Installation

### Ubuntu/Debian/Raspberry Pi OS

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential libnl-3-dev libnl-genl-3-dev

# Clone or download the project
git clone https://github.com/yourusername/wifiscanner.git
cd wifiscanner

# Compile
clang++ -o wifiscanner src/main.cpp src/wifi_scanner.cpp \
    -I/usr/include/libnl3 -lnl-3 -lnl-genl-3 -std=c++17

# Or with g++
g++ -o wifiscanner src/main.cpp src/wifi_scanner.cpp \
    -I/usr/include/libnl3 -lnl-3 -lnl-genl-3 -std=c++17
```

### Arch Linux

```bash
sudo pacman -S base-devel libnl

# Compile (same as above)
```

## Usage

### Basic Scanning

```bash
# JSON output (default) - good for scripting
sudo ./wifiscanner

# Human-readable table output
sudo ./wifiscanner --table

# Show help
./wifiscanner --help
```

### Example Output

**Table format** (`--table`):
```
Scanning for WiFi networks...
Using wireless interface: wlo1

SSID                            BSSID               Signal    Strength  Freq(MHz) Security       
-------------------------------------------------------------------------------------------------
MyHomeWiFi                      aa:bb:cc:dd:ee:ff   -45 dBm   90%       2437      WPA2           
CoffeeShop_Guest                11:22:33:44:55:66   -67 dBm   66%       5180      WPA/WPA2       
xfinitywifi                     99:88:77:66:55:44   -82 dBm   36%       2462      Open           

Total networks found: 3
```

**JSON format** (default):
```json
[
  {
    "ssid": "MyHomeWiFi",
    "bssid": "aa:bb:cc:dd:ee:ff",
    "signal_dbm": -45,
    "signal_percent": 90,
    "frequency": 2437,
    "security": "WPA2"
  },
  {
    "ssid": "CoffeeShop_Guest",
    "bssid": "11:22:33:44:55:66",
    "signal_dbm": -67,
    "signal_percent": 66,
    "frequency": 5180,
    "security": "WPA/WPA2"
  }
]
```

## Use Cases

### Scripting
```bash
# Get strongest network
sudo ./wifiscanner | jq '.[0].ssid'

# Count available networks
sudo ./wifiscanner | jq '. | length'

# Find networks on 5GHz band
sudo ./wifiscanner | jq '.[] | select(.frequency > 5000)'
```

### System Monitoring
```bash
# Periodic scanning
watch -n 30 'sudo ./wifiscanner --table'

# Log to file
sudo ./wifiscanner >> wifi_scan_$(date +%Y%m%d_%H%M%S).json
```

## Project Structure

```
wifiscanner/
├── src/
│   ├── main.cpp              # Main program with CLI interface
│   ├── wifi_scanner.cpp      # WiFi scanning implementation
│   └── wifi_scanner.hpp      # Header file
├── README.md
└── wifiscanner               # Compiled binary
```

## Binary Size

| Compiler | Size (stripped) | Size (unstripped) |
|----------|----------------|-------------------|
| g++ -O2  | ~95 KB         | ~180 KB           |
| clang++ -O2 | ~90 KB      | ~175 KB           |

**To reduce size further:**
```bash
# Compile with optimization and strip
clang++ -o wifiscanner src/main.cpp src/wifi_scanner.cpp \
    -I/usr/include/libnl3 -lnl-3 -lnl-genl-3 -std=c++17 -O2 -s
```

## Troubleshooting

### "nl80211 not found"
Your kernel doesn't support nl80211. Update your kernel or check if WiFi drivers are loaded:
```bash
lsmod | grep -i wifi
```

### "Interface not found"
Check your wireless interface name:
```bash
ip link show
# or
iwconfig
```

### Permission Denied
WiFi scanning requires root privileges:
```bash
sudo ./wifiscanner
```

### No Networks Found (but WiFi is working)
The interface might be managed by NetworkManager. Stop scanning temporarily:
```bash
sudo systemctl stop NetworkManager
sudo ./wifiscanner --table
sudo systemctl start NetworkManager
```

## Security Types Detected

- **Open** - No encryption
- **WEP** - Legacy encryption (insecure)
- **WPA** - WiFi Protected Access
- **WPA2** - WPA version 2 (most common)
- **WPA/WPA2** - Mixed mode (supports both)

## Performance

- **Scan time**: 1-3 seconds (adaptive)
- **Memory usage**: < 5 MB
- **CPU usage**: Minimal (single scan)

## Embedded Systems

### Raspberry Pi
Works on all Raspberry Pi models (Zero, 3, 4, 5) with both built-in and USB WiFi adapters.

### Resource Usage
- Binary size: ~90-180 KB
- RAM usage: ~2-5 MB during scan
- No background processes

### Cross-Compilation
```bash
# For ARM (Raspberry Pi)
arm-linux-gnueabihf-g++ -o wifiscanner src/main.cpp src/wifi_scanner.cpp \
    -I/usr/arm-linux-gnueabihf/include/libnl3 \
    -lnl-3 -lnl-genl-3 -std=c++17
```

## API Usage

### As a Library

```cpp
#include "wifi_scanner.hpp"

WifiScanner scanner;
auto networks = scanner.scanNetwork();

for (const auto& net : networks) {
    std::cout << "SSID: " << net.ssid << "\n";
    std::cout << "Signal: " << net.signal_mbm / 100 << " dBm\n";
    std::cout << "Security: " << net.security << "\n";
}
```

## Contributing

Contributions welcome! Please submit pull requests or open issues on GitHub.

## License

MIT License - See LICENSE file for details

## Acknowledgments

- Uses Linux nl80211 kernel interface
- Built with libnl (netlink library)

## Author

Created for embedded systems and Linux WiFi scanning needs.

## Version History

- **v1.0** - Initial release with nl80211 support
  - Auto-detect wireless interface
  - JSON and table output formats
  - Smart duplicate filtering
  - Comprehensive error handling