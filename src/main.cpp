#include "wifi_scanner.hpp"
#include <iostream>

int main() {
    WifiScanner wifi;
    auto networks = wifi.scanNetwork();

    std::cout << "[";
    for (size_t i = 0; i < networks.size(); ++i) {
        std::cout << networks[i].toJson();
        if (i < networks.size() - 1) {
            std::cout << ",";
        }
    }
    std::cout << "]\n";
}

