#include "wifi_scanner.hpp"
#include <linux/nl80211.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <net/if.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <ifaddrs.h>
#include <iostream>
#include <algorithm>

struct ScanData {
    std::vector<WifiScanner::NetworkInfo>* networks;
};

static std::string find_wireless_interface() {
    struct ifaddrs *ifaddr, *ifa;
    std::string interface;

    if (getifaddrs(&ifaddr) == -1) {
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == NULL) continue;

        std::string name(ifa->ifa_name);
        if (name.find("wlan") == 0 || name.find("wlp") == 0 ||
            name.find("wlo") == 0 || name.find("wlx") == 0) {
            interface = name;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return interface.empty() ? "wlan0" : interface;
}

static int scan_result_handler(struct nl_msg* msg, void* arg) {
    ScanData* data = static_cast<ScanData*>(arg);
    struct genlmsghdr* gnlh = static_cast<struct genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));
    struct nlattr* tb[NL80211_ATTR_MAX + 1];
    struct nlattr* bss[NL80211_BSS_MAX + 1];

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_BSS]) {
        return NL_SKIP;
    }

    if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], NULL)) {
        return NL_SKIP;
    }

    WifiScanner::NetworkInfo network;

    if (bss[NL80211_BSS_BSSID]) {
        uint8_t* mac = static_cast<uint8_t*>(nla_data(bss[NL80211_BSS_BSSID]));
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (int i = 0; i < 6; i++) {
            if (i > 0) oss << ":";
            oss << std::setw(2) << static_cast<int>(mac[i]);
        }
        network.bssid = oss.str();
    }

    if (bss[NL80211_BSS_SIGNAL_MBM]) {
        network.signal_mbm = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
    }

    if (bss[NL80211_BSS_FREQUENCY]) {
        network.frequency = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
    }

    if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
        uint8_t* ie = static_cast<uint8_t*>(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
        int ielen = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

        bool has_rsn = false;
        bool has_wpa = false;
        bool has_privacy = false;

        for (int i = 0; i < ielen; ) {
            if (i + 1 >= ielen) break;

            uint8_t id = ie[i];
            uint8_t len = ie[i + 1];

            if (i + 2 + len > ielen) break;

            if (id == 0) {
                if (len > 0 && len <= 32) {
                    network.ssid = std::string(reinterpret_cast<char*>(&ie[i + 2]), len);
                } else {
                    network.ssid = "<hidden>";
                }
            }
            else if (id == 48) {
                has_rsn = true;
            }
            else if (id == 221 && len >= 4) {
                if (memcmp(&ie[i + 2], "\x00\x50\xf2\x01", 4) == 0) {
                    has_wpa = true;
                }
            }

            i += 2 + len;
        }

        if (bss[NL80211_BSS_CAPABILITY]) {
            uint16_t capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
            has_privacy = (capa & (1 << 4)) != 0;
        }

        if (has_rsn && has_wpa) {
            network.security = "WPA/WPA2";
        } else if (has_rsn) {
            network.security = "WPA2";
        } else if (has_wpa) {
            network.security = "WPA";
        } else if (has_privacy) {
            network.security = "WEP";
        } else {
            network.security = "Open";
        }
    }

    if (!network.ssid.empty()) {
        data->networks->push_back(network);
    }

    return NL_SKIP;
}

static int finish_handler(struct nl_msg* msg, void* arg) {
    int* ret = static_cast<int*>(arg);
    *ret = 0;
    return NL_SKIP;
}

static int error_handler(struct sockaddr_nl* nla, struct nlmsgerr* err, void* arg) {
    int* ret = static_cast<int*>(arg);
    *ret = err->error;
    return NL_STOP;
}

static int ack_handler(struct nl_msg* msg, void* arg) {
    int* ret = static_cast<int*>(arg);
    *ret = 0;
    return NL_STOP;
}

WifiScanner::WifiScanner() {}
WifiScanner::~WifiScanner() {}

std::vector<WifiScanner::NetworkInfo> WifiScanner::scanNetwork() {
    std::vector<NetworkInfo> networks;

    struct nl_sock* sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate netlink socket" << std::endl;
        return networks;
    }

    if (genl_connect(sock) < 0) {
        std::cerr << "Failed to connect to generic netlink" << std::endl;
        nl_socket_free(sock);
        return networks;
    }

    int nl80211_id = genl_ctrl_resolve(sock, "nl80211");
    if (nl80211_id < 0) {
        std::cerr << "nl80211 not found (kernel might be too old or WiFi not available)" << std::endl;
        nl_close(sock);
        nl_socket_free(sock);
        return networks;
    }

    std::string interface = find_wireless_interface();
    std::cerr << "Using wireless interface: " << interface << std::endl;

    int if_index = if_nametoindex(interface.c_str());
    if (if_index == 0) {
        std::cerr << "Wireless interface " << interface << " not found" << std::endl;
        std::cerr << "Please check available interfaces with: ip link show" << std::endl;
        nl_close(sock);
        nl_socket_free(sock);
        return networks;
    }

    struct nl_msg* msg = nlmsg_alloc();
    if (!msg) {
        std::cerr << "Failed to allocate netlink message" << std::endl;
        nl_close(sock);
        nl_socket_free(sock);
        return networks;
    }

    genlmsg_put(msg, 0, 0, nl80211_id, 0, 0, NL80211_CMD_TRIGGER_SCAN, 0);
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

    struct nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT);
    int err = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

    if (nl_send_auto(sock, msg) < 0) {
        std::cerr << "Failed to send scan trigger" << std::endl;
        nlmsg_free(msg);
        nl_cb_put(cb);
        nl_close(sock);
        nl_socket_free(sock);
        return networks;
    }

    while (err > 0) {
        nl_recvmsgs(sock, cb);
    }

    nlmsg_free(msg);
    nl_cb_put(cb);

    if (err < 0) {
        std::cerr << "Scan trigger failed with error: " << err << std::endl;
        nl_close(sock);
        nl_socket_free(sock);
        return networks;
    }

    bool scan_complete = false;
    for (int attempt = 0; attempt < 10 && !scan_complete; attempt++) {

        msg = nlmsg_alloc();
        if (!msg) continue;

        genlmsg_put(msg, 0, 0, nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
        nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

        cb = nl_cb_alloc(NL_CB_DEFAULT);
        ScanData scan_data = {&networks};

        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, scan_result_handler, &scan_data);
        err = 1;
        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);

        nl_send_auto(sock, msg);

        while (err > 0) {
            nl_recvmsgs(sock, cb);
        }

        nl_cb_put(cb);
        nlmsg_free(msg);

        if (!networks.empty() || err < 0) {
            scan_complete = true;
        }
    }

    nl_close(sock);
    nl_socket_free(sock);

    if (!networks.empty()) {
        std::sort(networks.begin(), networks.end(),
            [](const NetworkInfo& a, const NetworkInfo& b) {
                if (a.ssid != b.ssid) return a.ssid < b.ssid;
                return a.signal_mbm > b.signal_mbm;
            });

        auto last = std::unique(networks.begin(), networks.end(),
            [](const NetworkInfo& a, const NetworkInfo& b) {
                return a.ssid == b.ssid;
            });
        networks.erase(last, networks.end());
    }

    return networks;
}

std::string WifiScanner::NetworkInfo::toJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"ssid\":\"" << ssid << "\",";
    json << "\"bssid\":\"" << bssid << "\",";
    json << "\"signal_dbm\":" << (signal_mbm / 100) << ",";
    json << "\"signal_percent\":" << getSignalPercent() << ",";
    json << "\"frequency\":" << frequency << ",";
    json << "\"security\":\"" << security << "\"";
    json << "}";
    return json.str();
}

int WifiScanner::NetworkInfo::getSignalPercent() const {
    int dbm = signal_mbm / 100;
    if (dbm <= -100) return 0;
    if (dbm >= -50) return 100;
    return 2 * (dbm + 100);
}
