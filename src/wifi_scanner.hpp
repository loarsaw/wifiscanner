#ifndef WIFI_SCANNER_HPP
#define WIFI_SCANNER_HPP

#include <string>
#include <vector>
#include <cstdint>

class WifiScanner {
public:
    struct NetworkInfo {
        std::string ssid;
        std::string bssid;
        int32_t signal_mbm = 0;      
        uint32_t frequency = 0;       
        std::string security = "Open";
        
        int getSignalPercent() const;
        std::string toJson() const;
    };

    WifiScanner();
    ~WifiScanner();
    
    std::vector<NetworkInfo> scanNetwork();
};

#endif 