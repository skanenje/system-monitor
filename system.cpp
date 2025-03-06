#include "header.h"
#include <cstring>
#include <fstream>
#include <pwd.h>
#include <sstream>

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

#include <fstream>
#include <string>
#include <sstream>

float getCPUTemperature() {
    std::ifstream tempFile("/proc/acpi/ibm/thermal");
    if (tempFile.is_open()) {
        std::string line;
        if (getline(tempFile, line)) {
            // Parse the temperatures line
            // Format is "temperatures: 50 -128 0 0 39 0 0 -128"
            if (line.find("temperatures:") != std::string::npos) {
                std::istringstream iss(line.substr(line.find(":") + 1));
                int temp;
                iss >> temp; // Read the first temperature value
                return temp; // Already in degrees C, no need to divide by 1000
            }
        }
    }
    return 0.0f;
}

float getFanSpeed() {
    std::ifstream fanFile("/proc/acpi/ibm/fan");
    if (fanFile.is_open()) {
        std::string line;
        while (getline(fanFile, line)) {
            // Look for the line that starts with "speed:"
            // Format is "speed:      2814"
            if (line.find("speed:") != std::string::npos) {
                std::istringstream iss(line.substr(line.find(":") + 1));
                float speed;
                iss >> speed;
                return speed;
            }
        }
    }
    return 0.0f;
}

Networks::~Networks() {
    for (auto& ip : ip4s) free(ip.name);
}

CPUUsageTracker::CPUUsageTracker() : lastStats{0}, currentUsage(0.0f) {}

float CPUUsageTracker::calculateCPUUsage() {
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

float CPUUsageTracker::getCurrentUsage() { return currentUsage; }

ProcessUsageTracker::ProcessUsageTracker() : deltaTime(0.0f), updateInterval(1.0f), lastUpdateTime(0.0f) {}

void ProcessUsageTracker::updateDeltaTime(float dt) {
    deltaTime = dt;
}

float ProcessUsageTracker::calculateProcessCPUUsage(const Proc& process, float currentTime) {
    // Return cached value if not time to update
    if (currentTime - lastUpdateTime < updateInterval) {
        auto it = cpuUsageCache.find(process.pid);
        return (it != cpuUsageCache.end()) ? it->second : 0.0f;
    }

    // Time to update: calculate new CPU usage
    long long processCPUTime = process.utime + process.stime;
    if (lastProcessCPUTime.find(process.pid) == lastProcessCPUTime.end()) {
        lastProcessCPUTime[process.pid] = {processCPUTime, 0};
        cpuUsageCache[process.pid] = 0.0f;
        lastUpdateTime = currentTime;
        return 0.0f; // First measurement
    }

    auto [lastProcTime, _] = lastProcessCPUTime[process.pid];
    long long procDelta = processCPUTime - lastProcTime;

    float cpuUsage = 0.0f;
    if (deltaTime > 0 && procDelta > 0) {
        float ticksPerSecond = sysconf(_SC_CLK_TCK);
        cpuUsage = (procDelta / (deltaTime * ticksPerSecond)) * 100.0f;
        cpuUsage = min(cpuUsage, 100.0f); // Cap at 100%
    }

    // Update cache and last time
    cpuUsageCache[process.pid] = cpuUsage;
    lastProcessCPUTime[process.pid] = {processCPUTime, 0};
    lastUpdateTime = currentTime;

    return cpuUsage;
}