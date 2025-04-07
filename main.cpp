#include "header.h"
#include <SDL2/SDL.h>
#include <GL/gl3w.h>
#include <vector>
#include <algorithm>
#include <set>
// Global performance trackers
struct NetworkRate {
    map<string, pair<long long, float>> lastRX, lastTX; // Last bytes, timestamp
    map<string, float> rxRate, txRate; // Rates in bytes/sec
    void update(NetworkTracker& tracker, float time) {
        auto rxStats = tracker.getNetworkRX();
        auto txStats = tracker.getNetworkTX();
        for (auto& [iface, rx] : rxStats) {
            if (lastRX.count(iface)) {
                float dt = time - lastRX[iface].second;
                rxRate[iface] = dt > 0 ? (rx.bytes - lastRX[iface].first) / dt : 0;
            }
            lastRX[iface] = {rx.bytes, time};
        }
        for (auto& [iface, tx] : txStats) {
            if (lastTX.count(iface)) {
                float dt = time - lastTX[iface].second;
                txRate[iface] = dt > 0 ? (tx.bytes - lastTX[iface].first) / dt : 0;
            }
            lastTX[iface] = {tx.bytes, time};
        }
    }
};

static CPUUsageTracker cpuTracker;
static ProcessUsageTracker processTracker;
static vector<float> cpuUsageHistory(100, 0.0f);
static vector<float> temperatureHistory(100, 0.0f);
static NetworkRate rateTracker;
static vector<float> cpuUsageBuffer(5, 0.0f);  // Buffer for last 5 readings
static int bufferIndex = 0;

// Timing variables for graph updates
static float cpuUpdateTime = 0.0f;
static float fanUpdateTime = 0.0f;
static float thermalUpdateTime = 0.0f;

void systemWindow(const char* id, ImVec2 size, ImVec2 position) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin(id);
    ImGui::SetWindowSize(size);
    ImGui::SetWindowPos(position);

    ImGui::BeginChild("SystemInfo", ImVec2(0, 150), true);
    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("Username: %s", getCurrentUsername().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("Total Processes: %d", getTotalProcessCount());
    ImGui::Text("CPU Type: %s", CPUinfo().c_str());
    auto states = countProcessStates();
    ImGui::Text("Process States:");
    for (const auto& [state, count] : states) {
        ImGui::Text("  %c: %d", state, count);
    }
    ImGui::EndChild();

    if (ImGui::BeginTabBar("SystemPerformanceTabs")) {
       if (ImGui::BeginTabItem("CPU")) {
        static bool pauseGraph = false;
        static float graphFPS = 30.0f;
        static float graphYScale = 100.0f;
        float currentCPUUsage = cpuTracker.calculateCPUUsage();

        // Add moving average calculation
        cpuUsageBuffer[bufferIndex] = currentCPUUsage;
        bufferIndex = (bufferIndex + 1) % cpuUsageBuffer.size();

        float smoothedCPUUsage = 0.0f;
        for (float usage : cpuUsageBuffer) {
            smoothedCPUUsage += usage;
        }
        smoothedCPUUsage /= cpuUsageBuffer.size();

        if (!pauseGraph) {
            float updateInterval = 1.0f / graphFPS;
            cpuUpdateTime += io.DeltaTime;
            if (cpuUpdateTime >= updateInterval) {
                cpuUsageHistory.erase(cpuUsageHistory.begin());
                cpuUsageHistory.push_back(smoothedCPUUsage);  // Use smoothed value
                cpuUpdateTime = 0.0f;
            }
        }

        ImGui::Checkbox("Pause Graph", &pauseGraph);
        ImGui::SliderFloat("Graph FPS", &graphFPS, 1.0f, 60.0f);
        ImGui::SliderFloat("Y-Scale", &graphYScale, 10.0f, 200.0f);

        ImGui::PlotLines("CPU Usage", cpuUsageHistory.data(), cpuUsageHistory.size(),
                        0, TextF("CPU: %.1f%%", smoothedCPUUsage).c_str(),  // Use smoothed value
                        0.0f, graphYScale, ImVec2(0, 80));
        ImGui::EndTabItem();
    }

        if (ImGui::BeginTabItem("Fan")) {
            static bool pauseGraph = false;
            static float graphFPS = 30.0f;
            static float graphYScale = 5000.0f;
            static vector<float> fanSpeedHistory(100, 0.0f);
            float fanSpeed = getFanSpeed();
            bool fanAvailable = fanSpeed > 0;

            if (!pauseGraph) {
                float updateInterval = 1.0f / graphFPS;
                fanUpdateTime += io.DeltaTime;
                if (fanUpdateTime >= updateInterval) {
                    fanSpeedHistory.erase(fanSpeedHistory.begin());
                    fanSpeedHistory.push_back(fanSpeed);
                    fanUpdateTime = 0.0f;
                }
            }

            ImGui::Checkbox("Pause Graph", &pauseGraph);
            ImGui::SliderFloat("Graph FPS", &graphFPS, 1.0f, 60.0f);
            ImGui::SliderFloat("Y-Scale", &graphYScale, 1000.0f, 10000.0f);

            if (fanAvailable) {
                ImGui::Text("Fan Status: Active");
                ImGui::Text("Fan Speed: %.0f RPM", fanSpeed);
                ImGui::Text("Fan Level: %s",
                            fanSpeed < 1000 ? "Low" : fanSpeed < 3000 ? "Medium" : "High");

                ImGui::PlotLines("Fan Speed", fanSpeedHistory.data(), fanSpeedHistory.size(),
                                0, TextF("%.0f RPM", fanSpeed).c_str(),
                                0.0f, graphYScale, ImVec2(0, 80));
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Fan information not available on this system");
                ImGui::Text("Fan monitoring is supported on some ThinkPad models and");
                ImGui::Text("other systems with accessible fan sensors.");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Thermal")) {
            static bool pauseGraph = false;
            static float graphFPS = 30.0f;
            static float graphYScale = 100.0f;
            float temperature = getCPUTemperature();
            bool tempAvailable = temperature > 0.1f; // Small threshold to detect valid readings

            if (!pauseGraph) {
                float updateInterval = 1.0f / graphFPS;
                thermalUpdateTime += io.DeltaTime;
                if (thermalUpdateTime >= updateInterval) {
                    temperatureHistory.erase(temperatureHistory.begin());
                    temperatureHistory.push_back(temperature);
                    thermalUpdateTime = 0.0f;
                }
            }

            ImGui::Checkbox("Pause Graph", &pauseGraph);
            ImGui::SliderFloat("Graph FPS", &graphFPS, 1.0f, 60.0f);
            ImGui::SliderFloat("Y-Scale", &graphYScale, 10.0f, 200.0f);

            if (tempAvailable) {
                ImGui::Text("Current Temperature: %.1f°C", temperature);
                ImGui::PlotLines("Temperature", temperatureHistory.data(), temperatureHistory.size(),
                                0, TextF("Temp: %.1f°C", temperature).c_str(),
                                0.0f, graphYScale, ImVec2(0, 80));

                // Add temperature status indicator
                if (temperature < 50.0f) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Temperature Status: Normal");
                } else if (temperature < 70.0f) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Temperature Status: Warm");
                } else if (temperature < 85.0f) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Temperature Status: Hot");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Temperature Status: Critical!");
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Temperature information not available");
                ImGui::Text("The system is using a hardware-agnostic approach to find");
                ImGui::Text("temperature sensors. No compatible sensors were found.");
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void memoryProcessesWindow(const char* id, ImVec2 size, ImVec2 position) {
    ImGuiIO& io = ImGui::GetIO();
    processTracker.updateDeltaTime(io.DeltaTime);
    float currentTime = ImGui::GetTime(); // Get current time for throttling

    ImGui::Begin(id);
    ImGui::SetWindowSize(size);
    ImGui::SetWindowPos(position);

    SystemResourceTracker resourceTracker;
    MemoryInfo memInfo = resourceTracker.getMemoryInfo();
    DiskInfo diskInfo = resourceTracker.getDiskInfo();

    ImGui::BeginChild("Memory Info", ImVec2(0, 150), true);
    ImGui::Text("RAM Usage: %ld MB / %ld MB (%.2f%%)",
                memInfo.used_ram, memInfo.total_ram, memInfo.ram_percent);
    ImGui::ProgressBar(memInfo.ram_percent / 100.0f, ImVec2(0, 0),
                       TextF("%.2f%%", memInfo.ram_percent).c_str());

    ImGui::Text("SWAP Usage: %ld MB / %ld MB (%.2f%%)",
                memInfo.used_swap, memInfo.total_swap, memInfo.swap_percent);
    ImGui::ProgressBar(memInfo.swap_percent / 100.0f, ImVec2(0, 0),
                       TextF("%.2f%%", memInfo.swap_percent).c_str());

    ImGui::Text("Disk Usage: %ld GB / %ld GB (%.2f%%)",
                diskInfo.used_space, diskInfo.total_space, diskInfo.usage_percent);
    ImGui::ProgressBar(diskInfo.usage_percent / 100.0f, ImVec2(0, 0),
                       TextF("%.2f%%", diskInfo.usage_percent).c_str());
    ImGui::EndChild();

    static char processFilter[256] = "";
    ImGui::InputText("Filter Processes", processFilter, sizeof(processFilter));

    vector<Proc> processes = resourceTracker.getProcessList();

    // Track selected processes
    static set<int> selectedPids;

    if (ImGui::BeginTable("Processes", 5,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("CPU Usage");
        ImGui::TableSetupColumn("Memory Usage");
        ImGui::TableHeadersRow();

        for (const auto& proc : processes) {
            if (processFilter[0] != '\0' && strstr(proc.name.c_str(), processFilter) == nullptr)
                continue;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool isSelected = selectedPids.count(proc.pid) > 0;
            // Use Selectable for the entire row, starting with PID
            if (ImGui::Selectable(TextF("%d", proc.pid).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                if (ImGui::GetIO().KeyCtrl) {
                    // Multi-select with Ctrl
                    if (isSelected) selectedPids.erase(proc.pid); // Deselect
                    else selectedPids.insert(proc.pid); // Select
                } else {
                    // Single-select without Ctrl
                    selectedPids.clear();
                    selectedPids.insert(proc.pid);
                }
            }

            // Display remaining columns
            ImGui::TableNextColumn(); ImGui::Text("%s", proc.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%c", proc.state);
            ImGui::TableNextColumn();
            float cpuUsage = processTracker.calculateProcessCPUUsage(proc, currentTime);
            ImGui::Text("%.2f%%", cpuUsage);
            ImGui::TableNextColumn();
            float memPercent = (proc.vsize / 1024.0f) / memInfo.total_ram * 100.0f;
            ImGui::Text("%.2f%%", memPercent);
        }
        ImGui::EndTable();
    }

    // Optional: Display the number of selected processes
    ImGui::Text("Selected processes: %zu", selectedPids.size());

    ImGui::End();
}

void networkWindow(const char* id, ImVec2 size, ImVec2 position) {
    ImGui::Begin(id);
    ImGui::SetWindowSize(size);
    ImGui::SetWindowPos(position);

    NetworkTracker networkTracker;
    Networks interfaces = networkTracker.getNetworkInterfaces();
    ImGui::Text("Network Interfaces:");
    for (const auto& iface : interfaces.ip4s) {
        ImGui::Text("%s: %s", iface.name, iface.addressBuffer);
    }

    if (ImGui::BeginTabBar("NetworkTabs")) {
        if (ImGui::BeginTabItem("RX (Receiver)")) {
            map<string, RX> rxStats = networkTracker.getNetworkRX();
            if (ImGui::BeginTable("RX Stats", 8, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
                ImGui::TableSetupColumn("Interface");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableSetupColumn("Packets");
                ImGui::TableSetupColumn("Errs");
                ImGui::TableSetupColumn("Drop");
                ImGui::TableSetupColumn("FIFO");
                ImGui::TableSetupColumn("Frame");
                ImGui::TableSetupColumn("Compressed");
                ImGui::TableHeadersRow();

                for (const auto& [iface, rx] : rxStats) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%s", iface.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", formatNetworkBytes(rx.bytes).c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%d", rx.packets);
                    ImGui::TableNextColumn(); ImGui::Text("%d", rx.errs);
                    ImGui::TableNextColumn(); ImGui::Text("%d", rx.drop);
                    ImGui::TableNextColumn(); ImGui::Text("%d", rx.fifo);
                    ImGui::TableNextColumn(); ImGui::Text("%d", rx.frame);
                    ImGui::TableNextColumn(); ImGui::Text("%d", rx.compressed);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("TX (Transmitter)")) {
            map<string, TX> txStats = networkTracker.getNetworkTX();
            if (ImGui::BeginTable("TX Stats", 8, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
                ImGui::TableSetupColumn("Interface");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableSetupColumn("Packets");
                ImGui::TableSetupColumn("Errs");
                ImGui::TableSetupColumn("Drop");
                ImGui::TableSetupColumn("FIFO");
                ImGui::TableSetupColumn("Colls");
                ImGui::TableSetupColumn("Compressed");
                ImGui::TableHeadersRow();

                for (const auto& [iface, tx] : txStats) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%s", iface.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", formatNetworkBytes(tx.bytes).c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%d", tx.packets);
                    ImGui::TableNextColumn(); ImGui::Text("%d", tx.errs);
                    ImGui::TableNextColumn(); ImGui::Text("%d", tx.drop);
                    ImGui::TableNextColumn(); ImGui::Text("%d", tx.fifo);
                    ImGui::TableNextColumn(); ImGui::Text("%d", tx.colls);
                    ImGui::TableNextColumn(); ImGui::Text("%d", tx.compressed);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Network Usage")) {
            static bool showRX = true, showTX = true;
            ImGui::Checkbox("Show RX", &showRX);
            ImGui::SameLine();
            ImGui::Checkbox("Show TX", &showTX);

            map<string, RX> rxStats = networkTracker.getNetworkRX();
            map<string, TX> txStats = networkTracker.getNetworkTX();
            rateTracker.update(networkTracker, ImGui::GetTime()); // Update rates

            if (showRX) {
                ImGui::Text("RX Network Usage:");
                for (const auto& [iface, rx] : rxStats) {
                    if (iface.find("lo") != string::npos) continue;
                    float rate = rateTracker.rxRate[iface]; // Bytes per second
                    float scaledRate = rate / (1024 * 1024); // Scale to MB/s for progress bar
                    ImGui::Text("%s:", iface.c_str());
                    ImGui::SameLine(150);
                    ImGui::ProgressBar(scaledRate, ImVec2(-1, 0), formatNetworkBytes(rate).c_str());
                }
            }

            if (showTX) {
                ImGui::Text("TX Network Usage:");
                for (const auto& [iface, tx] : txStats) {
                    if (iface.find("lo") != string::npos) continue;
                    float rate = rateTracker.txRate[iface]; // Bytes per second
                    float scaledRate = rate / (1024 * 1024); // Scale to MB/s for progress bar
                    ImGui::Text("%s:", iface.c_str());
                    ImGui::SameLine(150);
                    ImGui::ProgressBar(scaledRate, ImVec2(-1, 0), formatNetworkBytes(rate).c_str());
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (gl3wInit() != 0) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    bool done = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE))
                done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImVec2 mainDisplay = io.DisplaySize;
        memoryProcessesWindow("== Memory and Processes ==", ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 30), ImVec2((mainDisplay.x / 2) + 10, 10));
        systemWindow("== System ==", ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 30), ImVec2(10, 10));
        networkWindow("== Network ==", ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 60), ImVec2(10, (mainDisplay.y / 2) + 50));

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
