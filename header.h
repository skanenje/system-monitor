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
    ~Networks();
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
    MemoryInfo getMemoryInfo();
    DiskInfo getDiskInfo();
    vector<Proc> getProcessList();
};

class CPUUsageTracker {
private:
    CPUStats lastStats;
    float currentUsage;

public:
    CPUUsageTracker();
    float calculateCPUUsage();
    float getCurrentUsage();
};

class ProcessUsageTracker {
private:
    map<int, pair<long long, long long>> lastProcessCPUTime;
    float deltaTime; // Added for frame-based timing

public:
    ProcessUsageTracker();
    float calculateProcessCPUUsage(const Proc& process);
    void updateDeltaTime(float dt); // Added declaration
};

class NetworkTracker {
public:
    Networks getNetworkInterfaces();
    map<string, RX> getNetworkRX();
    map<string, TX> getNetworkTX();
};

// System functions
string CPUinfo();
const char* getOsName();
string getCurrentUsername();
string getHostname();
map<char, int> countProcessStates();
int getTotalProcessCount();
float getCPUTemperature();
float getFanSpeed();
string formatNetworkBytes(long long bytes);

template<typename... Args>
string TextF(const char* fmt, Args... args);

#endif