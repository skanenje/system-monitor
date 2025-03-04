#include "header.h"
#include <cstring>
#include <fstream>
#include <pwd.h>
#include <sstream>
// get cpu id and information, you can use `proc/cpuinfo`
// In system.cpp
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

const char *getOsName() {
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
// Get current logged-in username
string getCurrentUsername() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return pw->pw_name;
    }
    
    // Fallback methods
    const char* user = getenv("USER");
    if (user) return user;
    
    return "Unknown";
}

string getHostname() {
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        // Additional trim/sanitization
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
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            ifstream statFile(statPath);
            if (statFile.is_open()) {
                int pid;
                string name, fullStat;
                char state;
                
                // Read entire stat line to handle process names with spaces
                getline(statFile, fullStat);
                
                // Extract state safely
                size_t statePos = fullStat.find_last_of(")");
                if (statePos != string::npos && statePos + 2 < fullStat.length()) {
                    state = fullStat[statePos + 2];
                    processStates[state]++;
                }
                
                statFile.close();
            }
        }
    }
    
    closedir(dir);
    return processStates;
}
// Total number of processes
int getTotalProcessCount() {
    map<char, int> states = countProcessStates();
    int total = 0;
    for (const auto& pair : states) {
        total += pair.second;
    }
    return total;
}
// CPU Usage Calculation
class CPUUsageTracker {
private:
    CPUStats lastStats = {0};
    float currentUsage = 0.0f;

public:
    float calculateCPUUsage() {
        // Read current CPU stats from /proc/stat
        std::ifstream statFile("/proc/stat");
        std::string line;
        std::getline(statFile, line);
        
        CPUStats current;
        sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld", 
               &current.user, &current.nice, &current.system, 
               &current.idle, &current.iowait, &current.irq, 
               &current.softirq, &current.steal, &current.guest, 
               &current.guestNice);

        // Calculate total and idle time differences
        long long int prevTotal = lastStats.user + lastStats.nice + lastStats.system + 
                                  lastStats.idle + lastStats.iowait + lastStats.irq + 
                                  lastStats.softirq + lastStats.steal;
        
        long long int currentTotal = current.user + current.nice + current.system + 
                                     current.idle + current.iowait + current.irq + 
                                     current.softirq + current.steal;
        
        long long int totalDiff = currentTotal - prevTotal;
        long long int idleDiff = current.idle - lastStats.idle;

        // Calculate CPU usage percentage
        if (totalDiff > 0) {
            currentUsage = 100.0f * (totalDiff - idleDiff) / totalDiff;
        }

        lastStats = current;
        return currentUsage;
    }

    float getCurrentUsage() { return currentUsage; }
};

// Temperature Retrieval
float getCPUTemperature() {
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (tempFile.is_open()) {
        int temp;
        tempFile >> temp;
        return temp / 1000.0f;  // Convert millidegrees to degrees
    }
    return 0.0f;
}

// Fan Speed Retrieval (may vary by system)
float getFanSpeed() {
    // Placeholder - actual implementation depends on specific system
    std::ifstream fanFile("/sys/class/hwmon/hwmon0/fan1_input");
    if (fanFile.is_open()) {
        float speed;
        fanFile >> speed;
        return speed;
    }
    return 0.0f;
}
// Process CPU usage tracking
class ProcessUsageTracker {
private:
    map<int, pair<long long, long long>> lastProcessCPUTime;
    long long lastTotalCPUTime = 0;

public:
    float calculateProcessCPUUsage(const Proc& process) {
        long long currentTotalCPUTime = 0;
        // Read total CPU time from /proc/stat
        ifstream statFile("/proc/stat");
        string line;
        while (getline(statFile, line)) {
            if (line.substr(0, 3) == "cpu ") {
                sscanf(line.c_str(), "cpu %lld", &currentTotalCPUTime);
                break;
            }
        }

        // Process's CPU time
        long long processCPUTime = process.utime + process.stime;

        // Calculate delta
        long long totalDelta = currentTotalCPUTime - lastTotalCPUTime;
        
        // Check if process exists in map
        if (lastProcessCPUTime.find(process.pid) == lastProcessCPUTime.end()) {
            lastProcessCPUTime[process.pid] = {0, 0};
        }
        
        auto& lastProcTime = lastProcessCPUTime[process.pid].first;
        long long procDelta = processCPUTime - lastProcTime;

        // Update tracking
        lastProcessCPUTime[process.pid] = {processCPUTime, currentTotalCPUTime};
        lastTotalCPUTime = currentTotalCPUTime;

        // Calculate percentage (protect against division by zero)
        return totalDelta > 0 ? (100.0f * procDelta) / totalDelta : 0.0f;
    }
};