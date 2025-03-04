# System Monitoring Application

## Overview
A comprehensive system monitoring application built with C++ and ImGui, providing real-time insights into system resources, processes, and network statistics.

## Features

### System Window
- **Operating System Detection**
- **Username Display**
- **Hostname Information**
- **Total Process Count**
- **CPU Information**

#### System Performance Tabs
- **CPU Usage Graph**
- **Fan Speed Monitor**
- **Thermal Temperature Tracking**

### Memory and Processes Window
- **RAM Usage Statistics**
- **SWAP Usage Tracking**
- **Disk Usage Overview**
- **Detailed Process Table**
  - Process Filtering
  - Multi-select Capabilities

### Network Window
- **Network Interface Details**
- **Receiver (RX) Network Statistics**
- **Transmitter (TX) Network Statistics**

## Prerequisites

### Dependencies
- **SDL2**
- **ImGui**
- **OpenGL**
- **C++17 Compiler**

### Build Requirements
- **CMake**
- **Make**
- **gcc/g++ Compiler**

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
./monitor
