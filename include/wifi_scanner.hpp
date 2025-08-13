#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <sdbus-c++/sdbus-c++.h> // so sdbus::IConnection and sdbus::ObjectPath are known
#include <cstdint> // for uint32_t

class WifiScanner {
public:
    struct NetworkInfo {
        std::string ssid;
        int strength;
        std::string security;
        std::string toJson() const {
            std::stringstream ss;
            ss << "{\"ssid\":\"" << ssid << "\",\"strength\":" << strength
               << ",\"security\":\"" << security << "\"}";
            return ss.str();
        }
    };

    WifiScanner();
    ~WifiScanner();
    std::vector<NetworkInfo> scanNetwork();

private:
    std::vector<NetworkInfo> scanWifiDevice(sdbus::IConnection& connection, const sdbus::ObjectPath& device_path);
    std::string determineSecurity(uint32_t flags, uint32_t wpa_flags, uint32_t rsn_flags);
};

