# System Monitor

## Overview
The **System Monitor** is a desktop application designed to monitor computer system resources and performance. This project is built using **C++** and the **Dear ImGui** library for the graphical user interface. The application provides real-time monitoring of CPU, RAM, SWAP, network activity, and other system resources.

## Objectives
The main goal of this project is to demonstrate proficiency in **C++ programming** and the ability to work with new libraries. The application will:
- Display **system information**, including the OS type, logged-in user, and hostname.
- Monitor **CPU performance**, including graphs for CPU usage, fan speed, and temperature.
- Provide **memory and process monitoring**, including RAM, SWAP, and disk usage with a process table.
- Track **network activity**, including IP addresses and RX/TX data statistics.

## Technologies Used
- **C++** for system programming
- **Dear ImGui** for UI development
- **SDL2** for handling input and window management
- **OpenGL** for rendering graphics
- **Linux /proc filesystem** for gathering system information

## Installation
To set up the development environment, install the required dependencies:
```sh
sudo apt update
sudo apt install libsdl2-dev
```
Clone the repository:
```sh
git clone https://learn.zone01kisumu.ke/git/skanenje/system-monitor.git
cd system-monitor
```
Compile the project using the provided **Makefile**:
```sh
make
```
Run the application:
```sh
./system-monitor
```

## Project Structure
```
system-monitor/
├── header.h                    # Header file containing struct definitions and function prototypes
├── main.cpp                    # Main file that initializes SDL, ImGui, and OpenGL
├── mem.cpp                     # Handles memory and process monitoring
├── network.cpp                 # Handles network monitoring
├── system.cpp                  # Handles system resource monitoring
├── Makefile                    # Build instructions
├── imgui/                      # Dear ImGui library files
│   └── lib/
│       ├── backend/            # ImGui backend files (SDL & OpenGL3 implementation)
│       ├── gl3w/               # OpenGL loader
│       ├── imgui.cpp           # Core ImGui files
│       ├── imgui_draw.cpp      # Rendering functions
│       ├── imgui_widgets.cpp   # UI widgets
```

## Features
### 1. **System Monitor**
- OS type
- Logged-in user
- Hostname
- Running processes (tasks)
- CPU information (usage graph, temperature, fan speed, etc.)

### 2. **Memory & Process Monitor**
- RAM usage (visual representation)
- SWAP memory usage
- Disk usage
- Process table with:
  - **PID** (Process ID)
  - **Name**
  - **State**
  - **CPU Usage** (%)
  - **Memory Usage** (%)
- Filter/search box for processes
- Multi-row selection

### 3. **Network Monitor**
- Display network interfaces (IPv4 addresses)
- **RX (Receive)** stats: bytes, packets, errors, drops, etc.
- **TX (Transmit)** stats: bytes, packets, errors, collisions, etc.
- Visual representation of network usage with adaptive scaling

## Usage
- The application opens with three main sections: **System**, **Memory & Processes**, and **Network**.
- Users can interact with UI elements such as **checkboxes, sliders, and buttons** to control monitoring features.
- The CPU section includes an **FPS slider** and a **graph scale slider**.
- To select a process use Ctrl + click;

## Learning Outcomes
By working on this project, you will gain experience in:
- **C++ programming**
- **UI/UX development with Dear ImGui**
- **Handling system resources (CPU, Memory, Disk, Network)**
- **Using the Linux `/proc` and `/sysfs` filesystems**
- **Working with SDL and OpenGL**

## Contributing
Contributions are welcome! If you find issues or want to improve the project, feel free to submit a pull request.

## License
This project is open-source and free to use.

---
_Something wrong? Submit an issue or propose a change!_

