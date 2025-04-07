#include "header.h"
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <algorithm>

MemoryInfo SystemResourceTracker::getMemoryInfo() {
    struct sysinfo info;
    sysinfo(&info);

    // Read from /proc/meminfo for more accurate memory usage (htop-compatible)
    unsigned long memTotal = 0, memFree = 0;
    unsigned long buffers = 0, cached = 0, sReclaimable = 0, shmem = 0;
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        unsigned long value;
        iss >> key >> value;

        if (key == "MemTotal:") {
            memTotal = value;
        } else if (key == "MemFree:") {
            memFree = value;
        } else if (key == "Buffers:") {
            buffers = value;
        } else if (key == "Cached:") {
            cached = value;
        } else if (key == "SReclaimable:") {
            sReclaimable = value;
        } else if (key == "Shmem:") {
            shmem = value;
        }
    }

    // Calculate memory usage similar to htop
    unsigned long cachedTotal = cached + sReclaimable - shmem;
    unsigned long usedDiff = memFree + cachedTotal + buffers;
    unsigned long usedMem = (memTotal >= usedDiff) ? memTotal - usedDiff : memTotal - memFree;

    MemoryInfo mem;
    mem.total_ram = memTotal / 1024;  // Convert KB to MB
    mem.used_ram = usedMem / 1024;    // Convert KB to MB

    mem.total_swap = info.totalswap / (1024 * 1024);
    mem.used_swap = (info.totalswap - info.freeswap) / (1024 * 1024);

    mem.ram_percent = (float)mem.used_ram / mem.total_ram * 100.0f;
    mem.swap_percent = info.totalswap > 0 ? (float)mem.used_swap / mem.total_swap * 100.0f : 0.0f;

    return mem;
}

DiskInfo SystemResourceTracker::getDiskInfo() {
    struct statvfs stat;
    statvfs("/", &stat);

    DiskInfo disk;
    disk.total_space = stat.f_blocks * stat.f_frsize / (1024 * 1024 * 1024);
    disk.used_space = (stat.f_blocks - stat.f_bfree) * stat.f_frsize / (1024 * 1024 * 1024);
    disk.usage_percent = (float)disk.used_space / disk.total_space * 100.0f;

    return disk;
}

vector<Proc> SystemResourceTracker::getProcessList() {
    vector<Proc> processes;
    DIR *dir = opendir("/proc");
    if (!dir) return processes;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            string pid = entry->d_name;
            string statPath = "/proc/" + pid + "/stat";
            ifstream statFile(statPath);

            if (statFile.is_open()) {
                Proc process;
                process.pid = stoi(pid);

                string line;
                getline(statFile, line);

                size_t nameStart = line.find('(');
                size_t nameEnd = line.rfind(')');

                if (nameStart != string::npos && nameEnd != string::npos) {
                    process.name = line.substr(nameStart + 1, nameEnd - nameStart - 1);

                    istringstream iss(line.substr(nameEnd + 1));
                    string field;
                    vector<string> fields;

                    while (iss >> field) {
                        fields.push_back(field);
                    }

                    if (fields.size() >= 24) {
                        process.state = fields[0][0];
                        process.vsize = stoll(fields[20]);
                        process.rss = stoll(fields[21]);
                        process.utime = stoll(fields[11]);
                        process.stime = stoll(fields[12]);
                    }

                    processes.push_back(process);
                }
                statFile.close();
            }
        }
    }

    closedir(dir);
    return processes;
}
