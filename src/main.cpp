#include "wifi_scanner.hpp"
#include <iostream>
#include <iomanip>
#include <unistd.h>

void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "  -j, --json     Output as JSON (default)\n";
    std::cout << "  -t, --table    Output as formatted table\n";
    std::cout << "\nNote: This program requires root privileges to scan WiFi networks.\n";
    std::cout << "Run with: sudo " << programName << "\n";
}


void printAsTable(const std::vector<WifiScanner::NetworkInfo>& networks) {
    if (networks.empty()) {
        std::cout << "No networks found.\n";
        return;
    }

    std::cout << std::left
              << std::setw(32) << "SSID"
              << std::setw(20) << "BSSID"
              << std::setw(10) << "Signal"
              << std::setw(10) << "Strength"
              << std::setw(10) << "Freq(MHz)"
              << std::setw(15) << "Security"
              << "\n";

    std::cout << std::string(97, '-') << "\n";

    for (const auto& net : networks) {
        int dbm = net.signal_mbm / 100;
        int percent = net.getSignalPercent();

        std::cout << std::left
                  << std::setw(32) << net.ssid
                  << std::setw(20) << net.bssid
                  << std::setw(10) << (std::to_string(dbm) + " dBm")
                  << std::setw(10) << (std::to_string(percent) + "%")
                  << std::setw(10) << net.frequency
                  << std::setw(15) << net.security
                  << "\n";
    }

    std::cout << "\nTotal networks found: " << networks.size() << "\n";
}

void printAsJson(const std::vector<WifiScanner::NetworkInfo>& networks) {
    std::cout << "[";
    for (size_t i = 0; i < networks.size(); ++i) {
        std::cout << networks[i].toJson();
        if (i < networks.size() - 1) {
            std::cout << ",";
        }
    }
    std::cout << "]\n";
}

int main(int argc, char* argv[]) {
    bool useTable = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-t" || arg == "--table") {
            useTable = true;
        } else if (arg == "-j" || arg == "--json") {
            useTable = false;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printHelp(argv[0]);
            return 1;
        }
    }

    if (geteuid() != 0) {
        std::cerr << "Error: This program requires root privileges.\n";
        std::cerr << "Please run with: sudo " << argv[0] << "\n";
        return 1;
    }

    try {
        WifiScanner scanner;

        if (useTable) {
            std::cout << "Scanning for WiFi networks...\n\n";
        }

        auto networks = scanner.scanNetwork();

        if (useTable) {
            printAsTable(networks);
        } else {
            printAsJson(networks);
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
