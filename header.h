#ifndef header_H
#define header_H
#include <pwd.h>
#include <numeric>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cpuid.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ctime>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <sstream>

using namespace std;

struct CPUStats {
    long long int user, nice, system, idle, iowait, irq, softirq, steal, guest, guestNice;
};

struct Proc {
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4 {
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks {
    vector<IP4> ip4s;
    ~Networks() { for (auto& ip : ip4s) free(ip.name); }
};

struct RX {
    int bytes, packets, errs, drop, fifo, frame, compressed, multicast;
};

struct TX {
    int bytes, packets, errs, drop, fifo, colls, carrier, compressed;
};
struct MemoryInfo {
    long total_ram, used_ram, total_swap, used_swap;
    float ram_percent, swap_percent;
};

struct DiskInfo {
    long total_space, used_space;
    float usage_percent;
};

class SystemResourceTracker {
public:
    MemoryInfo getMemoryInfo() {
        struct sysinfo info;
        sysinfo(&info);
        MemoryInfo mem;
        mem.total_ram = info.totalram * info.mem_unit / (1024 * 1024); // MB
        mem.used_ram = (info.totalram - info.freeram) * info.mem_unit / (1024 * 1024); // MB
        mem.total_swap = info.totalswap * info.mem_unit / (1024 * 1024); // MB
        mem.used_swap = (info.totalswap - info.freeswap) * info.mem_unit / (1024 * 1024); // MB
        mem.ram_percent = (mem.used_ram / (float)mem.total_ram) * 100.0f;
        mem.swap_percent = mem.total_swap ? (mem.used_swap / (float)mem.total_swap) * 100.0f : 0.0f;
        return mem;
    }

    DiskInfo getDiskInfo() {
        struct statvfs stat;
        statvfs("/", &stat);
        DiskInfo disk;
        disk.total_space = (stat.f_blocks * stat.f_frsize) / (1024 * 1024 * 1024); // GB
        disk.used_space = ((stat.f_blocks - stat.f_bfree) * stat.f_frsize) / (1024 * 1024 * 1024); // GB
        disk.usage_percent = (disk.used_space / (float)disk.total_space) * 100.0f;
        return disk;
    }

    vector<Proc> getProcessList() {
        vector<Proc> processes;
        DIR* dir = opendir("/proc");
        if (!dir) return processes;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type != DT_DIR || !isdigit(entry->d_name[0])) continue;

            int pid = atoi(entry->d_name);
            string statPath = string("/proc/") + entry->d_name + "/stat";
            ifstream statFile(statPath);
            if (!statFile.is_open()) continue;

            Proc proc;
            proc.pid = pid;
            string comm;
            statFile >> proc.pid >> comm >> proc.state;
            comm = comm.substr(1, comm.size() - 2); // Remove parentheses
            proc.name = comm;

            statFile >> proc.utime >> proc.stime; // Skip other fields until vsize and rss
            for (int i = 0; i < 19; i++) statFile >> proc.vsize; // vsize is 23rd field
            statFile >> proc.rss;

            proc.vsize /= 1024; // Convert to KB
            proc.rss *= getpagesize() / 1024; // Convert pages to KB
            processes.push_back(proc);
            statFile.close();
        }
        closedir(dir);
        return processes;
    }
};

class CPUUsageTracker {
private:
    CPUStats lastStats = {0};
    float currentUsage = 0.0f;

public:
    float calculateCPUUsage() {
        ifstream statFile("/proc/stat");
        string line;
        getline(statFile, line);
        
        CPUStats current;
        sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
               &current.user, &current.nice, &current.system,
               &current.idle, &current.iowait, &current.irq,
               &current.softirq, &current.steal, &current.guest,
               &current.guestNice);

        long long prevTotal = lastStats.user + lastStats.nice + lastStats.system +
                              lastStats.idle + lastStats.iowait + lastStats.irq +
                              lastStats.softirq + lastStats.steal;
        long long currentTotal = current.user + current.nice + current.system +
                                 current.idle + current.iowait + current.irq +
                                 current.softirq + current.steal;
        long long totalDiff = currentTotal - prevTotal;
        long long idleDiff = current.idle - lastStats.idle;

        if (totalDiff > 0) {
            currentUsage = 100.0f * (totalDiff - idleDiff) / totalDiff;
        }
        lastStats = current;
        return currentUsage;
    }

    float getCurrentUsage() { return currentUsage; }
};

class ProcessUsageTracker {
private:
    map<int, pair<long long, long long>> lastProcessCPUTime;
    float deltaTime = 0.0f;

public:
    void updateDeltaTime(float dt) { deltaTime = dt; }

    float calculateProcessCPUUsage(const Proc& process) {
        long long processCPUTime = process.utime + process.stime;
        auto it = lastProcessCPUTime.find(process.pid);
        if (it == lastProcessCPUTime.end()) {
            lastProcessCPUTime[process.pid] = {processCPUTime, 0};
            return 0.0f; // First measurement, no delta yet
        }

        auto [lastProcTime, _] = it->second;
        long long procDelta = processCPUTime - lastProcTime;

        if (deltaTime > 0 && procDelta > 0) {
            float ticksPerSecond = sysconf(_SC_CLK_TCK);
            float cpuUsage = (procDelta / (deltaTime * ticksPerSecond)) * 100.0f;
            lastProcessCPUTime[process.pid] = {processCPUTime, 0};
            return min(cpuUsage, 100.0f); // Cap at 100% per process
        }
        lastProcessCPUTime[process.pid] = {processCPUTime, 0};
        return 0.0f;
    }
};

// ... (everything up to NetworkTracker remains the same)

class NetworkTracker {
    public:
        Networks getNetworkInterfaces() {
            Networks nets;
            struct ifaddrs *ifap, *ifa;
            if (getifaddrs(&ifap) == -1) return nets;
    
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
            getline(netDevFile, line); // Skip header
            getline(netDevFile, line); // Skip second header
    
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
            getline(netDevFile, line); // Skip header
            getline(netDevFile, line); // Skip second header
    
            while (getline(netDevFile, line)) {
                istringstream iss(line);
                string interfaceName;
                getline(iss, interfaceName, ':');
                interfaceName = interfaceName.substr(interfaceName.find_first_not_of(" \t"));
                
                long long rxDummy;
                for (int i = 0; i < 8; ++i) iss >> rxDummy;
                
                TX tx;
                iss >> tx.bytes >> tx.packets >> tx.errs >> tx.drop 
                    >> tx.fifo >> tx.colls >> tx.carrier >> tx.compressed;
                txStats[interfaceName] = tx;
            }
            netDevFile.close();
            return txStats;
        }
    };
    
    // ... (rest of header.h remains unchanged)

string CPUinfo() {
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    return string(CPUBrandString);
}

const char* getOsName() {
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

string getCurrentUsername() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) return pw->pw_name;
    const char* user = getenv("USER");
    if (user) return user;
    return "Unknown";
}

string getHostname() {
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[HOST_NAME_MAX-1] = '\0';
        return string(hostname);
    }
    return "Unknown";
}

map<char, int> countProcessStates() {
    map<char, int> processStates;
    DIR *dir = opendir("/proc");
    if (!dir) return processStates;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            ifstream statFile(statPath);
            if (statFile.is_open()) {
                string fullStat;
                getline(statFile, fullStat);
                size_t statePos = fullStat.find_last_of(")");
                if (statePos != string::npos && statePos + 2 < fullStat.length()) {
                    char state = fullStat[statePos + 2];
                    processStates[state]++;
                }
                statFile.close();
            }
        }
    }
    closedir(dir);
    return processStates;
}

int getTotalProcessCount() {
    map<char, int> states = countProcessStates();
    int total = 0;
    for (const auto& pair : states) total += pair.second;
    return total;
}

float getCPUTemperature() {
    ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (tempFile.is_open()) {
        int temp;
        tempFile >> temp;
        return temp / 1000.0f;
    }
    return 0.0f;
}

float getFanSpeed() {
    ifstream fanFile("/sys/class/hwmon/hwmon0/fan1_input");
    if (fanFile.is_open()) {
        float speed;
        fanFile >> speed;
        return speed;
    }
    return 0.0f;
}

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

template<typename... Args>
string TextF(const char* fmt, Args... args) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), fmt, args...);
    return string(buffer);
}

#endif