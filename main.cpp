#include "header.h"
#include <SDL.h>

/*
NOTE : You are free to change the code as you wish, the main objective is to make the
       application work and pass the audit.

       It will be provided the main function with the following functions :

       - `void systemWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the system window on your screen
       - `void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the memory and processes window on your screen
       - `void networkWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the network window on your screen
*/

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h> // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h> // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h> // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h> // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE      // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h> // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE        // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h> // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// systemWindow, display information for the system monitorization
// Global performance trackers
static CPUUsageTracker cpuTracker;
static std::vector<float> cpuUsageHistory(100, 0.0f);
static std::vector<float> temperatureHistory(100, 0.0f);

void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // System Information Section
    ImGui::BeginChild("SystemInfo", ImVec2(0, 100), true);
    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("Username: %s", getCurrentUsername().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("Total Processes: %d", getTotalProcessCount());
    ImGui::Text("CPU Type: %s", CPUinfo().c_str());
    ImGui::EndChild();

    // Existing Performance Tabs
    if (ImGui::BeginTabBar("SystemPerformanceTabs")) {
        // CPU Performance Tab (existing implementation remains the same)
        if (ImGui::BeginTabItem("CPU")) {
            static bool pauseGraph = false;
            static float graphFPS = 30.0f;
            static float graphYScale = 100.0f;

            float currentCPUUsage = cpuTracker.calculateCPUUsage();

            // Shift history and add new value
            cpuUsageHistory.erase(cpuUsageHistory.begin());
            cpuUsageHistory.push_back(currentCPUUsage);

            ImGui::Checkbox("Pause Graph", &pauseGraph);
            ImGui::SliderFloat("Graph FPS", &graphFPS, 1.0f, 60.0f);
            ImGui::SliderFloat("Y-Scale", &graphYScale, 10.0f, 200.0f);

            ImGui::PlotLines("CPU Usage", cpuUsageHistory.data(), cpuUsageHistory.size(), 
                             0, TextF("CPU: %.1f%%", currentCPUUsage).c_str(), 
                             0.0f, graphYScale, ImVec2(0, 80));
            
            ImGui::EndTabItem();
        }

        // Fan Tab (existing implementation remains the same)
        if (ImGui::BeginTabItem("Fan")) {
            float fanSpeed = getFanSpeed();
            
            ImGui::Text("Fan Speed: %.0f RPM", fanSpeed);
            ImGui::ProgressBar(fanSpeed / 5000.0f, ImVec2(0.0f, 0.0f), TextF("%.0f RPM", fanSpeed).c_str());

            ImGui::EndTabItem();
        }

        // Thermal Tab (existing implementation remains the same)
        if (ImGui::BeginTabItem("Thermal")) {
            float temperature = getCPUTemperature();

            // Shift temperature history and add new value
            temperatureHistory.erase(temperatureHistory.begin());
            temperatureHistory.push_back(temperature);

            ImGui::Text("Current Temperature: %.1f°C", temperature);
            ImGui::PlotLines("Temperature", temperatureHistory.data(), temperatureHistory.size(), 
                             0, TextF("Temp: %.1f°C", temperature).c_str(), 
                             0.0f, 100.0f, ImVec2(0, 80));

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
// memoryProcessesWindow, display information for the memory and processes information
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    SystemResourceTracker resourceTracker;
    
    // Memory Information
    MemoryInfo memInfo = resourceTracker.getMemoryInfo();
    DiskInfo diskInfo = resourceTracker.getDiskInfo();

    // Memory Usage Sections
    ImGui::BeginChild("Memory Info", ImVec2(0, 150), true);
    
    // RAM Usage
    ImGui::Text("RAM Usage: %ld MB / %ld MB (%.2f%%)", 
                memInfo.used_ram, memInfo.total_ram, memInfo.ram_percent);
    ImGui::ProgressBar(memInfo.ram_percent / 100.0f, 
                       ImVec2(0.0f, 0.0f), 
                       TextF("%.2f%%", memInfo.ram_percent).c_str());

    // SWAP Usage
    ImGui::Text("SWAP Usage: %ld MB / %ld MB (%.2f%%)", 
                memInfo.used_swap, memInfo.total_swap, memInfo.swap_percent);
    ImGui::ProgressBar(memInfo.swap_percent / 100.0f, 
                       ImVec2(0.0f, 0.0f), 
                       TextF("%.2f%%", memInfo.swap_percent).c_str());

    // Disk Usage
    ImGui::Text("Disk Usage: %ld GB / %ld GB (%.2f%%)", 
                diskInfo.used_space, diskInfo.total_space, diskInfo.usage_percent);
    ImGui::ProgressBar(diskInfo.usage_percent / 100.0f, 
                       ImVec2(0.0f, 0.0f), 
                       TextF("%.2f%%", diskInfo.usage_percent).c_str());

    ImGui::EndChild();

    // Process Table
    static char processFilter[256] = "";
    ImGui::InputText("Filter Processes", processFilter, IM_ARRAYSIZE(processFilter));

    // Prepare process list
    vector<Proc> processes = resourceTracker.getProcessList();
    static vector<int> selectedProcesses;

    // Process Table
    if (ImGui::BeginTable("Processes", 5, 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_Reorderable | 
        ImGuiTableFlags_Hideable | 
        ImGuiTableFlags_MultiSelect))
    {
        // Table headers
        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("CPU Usage");
        ImGui::TableSetupColumn("Memory Usage");
        ImGui::TableHeadersRow();

        // Process rows
        for (const auto& proc : processes) {
            // Apply filter
            if (processFilter[0] != '\0' && 
                strstr(proc.name.c_str(), processFilter) == nullptr) 
                continue;

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", proc.pid);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", proc.name.c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%c", proc.state);
            
            ImGui::TableNextColumn();
            // Placeholder for actual CPU usage calculation
            ImGui::Text("%.2f%%", 0.0f);
            
            ImGui::TableNextColumn();
            // Memory usage as percentage of total memory
            float memPercent = (proc.vsize / (1024.0f * 1024.0f)) / memInfo.total_ram * 100.0f;
            ImGui::Text("%.2f%%", memPercent);
        }
        
        ImGui::EndTable();
    }

    ImGui::End();
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    NetworkTracker networkTracker;

    // Network Interfaces
    Networks interfaces = networkTracker.getNetworkInterfaces();
    ImGui::Text("Network Interfaces:");
    for (const auto& iface : interfaces.ip4s) {
        ImGui::Text("%s: %s", iface.name, iface.addressBuffer);
    }

    if (ImGui::BeginTabBar("NetworkTabs")) {
        // RX (Receive) Tab
        if (ImGui::BeginTabItem("RX (Receiver)")) {
            map<string, RX> rxStats = networkTracker.getNetworkRX();

            if (ImGui::BeginTable("RX Stats", 8, 
                ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
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

        // TX (Transmit) Tab
        if (ImGui::BeginTabItem("TX (Transmitter)")) {
            map<string, TX> txStats = networkTracker.getNetworkTX();

            if (ImGui::BeginTable("TX Stats", 8, 
                ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable)) {
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
    }
    ImGui::EndTabBar();

    ImGui::End();
}

// Main code
int main(int, char **)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char *name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // render bindings
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // background color
    // note : you are free to change the style of the application
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        {
            ImVec2 mainDisplay = io.DisplaySize;
            memoryProcessesWindow("== Memory and Processes ==",
                                  ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 30),
                                  ImVec2((mainDisplay.x / 2) + 10, 10));
            // --------------------------------------
            systemWindow("== System ==",
                         ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 30),
                         ImVec2(10, 10));
            // --------------------------------------
            networkWindow("== Network ==",
                          ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 60),
                          ImVec2(10, (mainDisplay.y / 2) + 50));
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
