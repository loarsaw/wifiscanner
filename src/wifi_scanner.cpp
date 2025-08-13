#include "wifi_scanner.hpp"
#include <sdbus-c++/sdbus-c++.h>
#include <iomanip>
#include <memory>
#include <iostream>
#include <map>
#include <unistd.h> // for sleep()

const std::string NM_SERVICE = "org.freedesktop.NetworkManager";
const std::string NM_PATH = "/org/freedesktop/NetworkManager";
const std::string NM_INTERFACE = "org.freedesktop.NetworkManager";
const std::string WIRELESS_INTERFACE = "org.freedesktop.NetworkManager.Device.Wireless";
const std::string AP_INTERFACE = "org.freedesktop.NetworkManager.AccessPoint";
const std::string DEVICE_INTERFACE = "org.freedesktop.NetworkManager.Device";

WifiScanner::WifiScanner() {}
WifiScanner::~WifiScanner() {}

std::vector<WifiScanner::NetworkInfo> WifiScanner::scanNetwork() {
    std::vector<NetworkInfo> networks;
    try {
        auto connection = sdbus::createSystemBusConnection();
        auto nm_proxy = sdbus::createProxy(*connection, NM_SERVICE, NM_PATH);

        std::vector<sdbus::ObjectPath> devices;
        nm_proxy->callMethod("GetDevices").onInterface(NM_INTERFACE).storeResultsTo(devices);

        for (const auto& device_path : devices) {
            auto device_proxy = sdbus::createProxy(*connection, NM_SERVICE, device_path);
            sdbus::Variant variant, nameVariant;

            device_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(DEVICE_INTERFACE, "DeviceType").storeResultsTo(variant);
            device_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(DEVICE_INTERFACE, "Interface").storeResultsTo(nameVariant);

            uint32_t device_type = variant.get<uint32_t>();
            if (device_type == 2) { // Wireless
                auto device_networks = scanWifiDevice(*connection, device_path);
                networks.insert(networks.end(), device_networks.begin(), device_networks.end());
            }
        }
    } catch (const sdbus::Error& e) {
        std::cerr << "DBUS ERROR: " << e.what() << " " << e.getName() << std::endl;
    }
    return networks;
}

std::vector<WifiScanner::NetworkInfo> WifiScanner::scanWifiDevice(sdbus::IConnection& connection, const sdbus::ObjectPath& device_path) {
    std::vector<NetworkInfo> networks;
    auto wifi_proxy = sdbus::createProxy(connection, NM_SERVICE, device_path);
    wifi_proxy->callMethod("RequestScan").onInterface(WIRELESS_INTERFACE).withArguments(std::map<std::string, sdbus::Variant>{});
    sleep(2);

    std::vector<sdbus::ObjectPath> accessPoints;
    wifi_proxy->callMethod("GetAccessPoints").onInterface(WIRELESS_INTERFACE).storeResultsTo(accessPoints);

    for (const auto& ap_path : accessPoints) {
        auto ap_proxy = sdbus::createProxy(connection, NM_SERVICE, ap_path);
        try {
            NetworkInfo network;
            sdbus::Variant ssidVariant, strengthVariant, flagsVariant, wpaFlagVariant, rsnFlagVariant;

            ap_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(AP_INTERFACE, "Ssid").storeResultsTo(ssidVariant);
            std::vector<uint8_t> ssid_bytes = ssidVariant.get<std::vector<uint8_t>>();
            network.ssid = ssid_bytes.empty() ? "<hidden>" : std::string(ssid_bytes.begin(), ssid_bytes.end());

            ap_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(AP_INTERFACE, "Strength").storeResultsTo(strengthVariant);
            network.strength = strengthVariant.get<uint8_t>();

            ap_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(AP_INTERFACE, "Flags").storeResultsTo(flagsVariant);
            ap_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(AP_INTERFACE, "WpaFlags").storeResultsTo(wpaFlagVariant);
            ap_proxy->callMethod("Get").onInterface("org.freedesktop.DBus.Properties")
                .withArguments(AP_INTERFACE, "RsnFlags").storeResultsTo(rsnFlagVariant);

            network.security = determineSecurity(
                flagsVariant.get<uint32_t>(),
                wpaFlagVariant.get<uint32_t>(),
                rsnFlagVariant.get<uint32_t>()
            );

            networks.push_back(network);
        } catch (const sdbus::Error&) {
            continue;
        }
    }
    return networks;
}

std::string WifiScanner::determineSecurity(uint32_t flags, uint32_t wpa_flags, uint32_t rsn_flags) {
    if ((flags & 0x1) == 0) {
        if (wpa_flags == 0 && rsn_flags == 0) {
            return "Open";
        }
        if ((wpa_flags & 0x1) || (rsn_flags & 0x1)) {
            return "WPA/WPA2";
        }
        return "Encrypted";
    }
    return "WEP";
}

