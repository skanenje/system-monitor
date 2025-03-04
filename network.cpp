#include "header.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

string formatNetworkBytes(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double value = bytes;

    while (value >= 1024 && unitIndex < 4) {
        value /= 1024;
        unitIndex++;
    }

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unitIndex]);
    return string(buffer);
}

class NetworkTracker {
public:
    Networks getNetworkInterfaces() {
        Networks nets;
        struct ifaddrs *ifap, *ifa;
        
        if (getifaddrs(&ifap) == -1) {
            return nets;
        }

        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
                
                IP4 interface;
                interface.name = strdup(ifa->ifa_name);
                inet_ntop(AF_INET, &(addr->sin_addr), interface.addressBuffer, INET_ADDRSTRLEN);
                
                nets.ip4s.push_back(interface);
            }
        }

        freeifaddrs(ifap);
        return nets;
    }

    map<string, RX> getNetworkRX() {
        map<string, RX> rxStats;
        ifstream netDevFile("/proc/net/dev");
        string line;
        
        getline(netDevFile, line); // Skip header line
        getline(netDevFile, line); // Skip second header line

        while (getline(netDevFile, line)) {
            istringstream iss(line);
            string interfaceName;
            getline(iss, interfaceName, ':');
            interfaceName = interfaceName.substr(interfaceName.find_first_not_of(" \t"));
            
            RX rx;
            iss >> rx.bytes >> rx.packets >> rx.errs >> rx.drop 
                >> rx.fifo >> rx.frame >> rx.compressed >> rx.multicast;
            
            rxStats[interfaceName] = rx;
        }

        netDevFile.close();
        return rxStats;
    }

    map<string, TX> getNetworkTX() {
        map<string, TX> txStats;
        ifstream netDevFile("/proc/net/dev");
        string line;
        
        getline(netDevFile, line); // Skip header line
        getline(netDevFile, line); // Skip second header line

        while (getline(netDevFile, line)) {
            istringstream iss(line);
            string interfaceName;
            getline(iss, interfaceName, ':');
            interfaceName = interfaceName.substr(interfaceName.find_first_not_of(" \t"));
            
            // Skip RX values (8 values)
            long long rxDummy;
            for (int i = 0; i < 8; ++i) {
                iss >> rxDummy;
            }
            
            // Now read TX values
            TX tx;
            iss >> tx.bytes >> tx.packets >> tx.errs >> tx.drop 
                >> tx.fifo >> tx.colls >> tx.carrier >> tx.compressed;
            
            txStats[interfaceName] = tx;
        }

        netDevFile.close();
        return txStats;
    }
};