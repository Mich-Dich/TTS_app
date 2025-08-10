# application\_template

A simple, cross-platform C++ application template with a graphical user interface (GUI) powered by ImGui and GLFW.

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Prerequisites](#prerequisites)
4. [Installation & Setup](#installation--setup)

   * [Initialize Submodules](#initialize-submodules)
   * [Run Setup Script](#run-setup-script)
5. [Building the Application](#building-the-application)

   * [Automatic Build (VSCode)](#automatic-build-vscode)
   * [Manual Build](#manual-build)
6. [Window Manager Integration](#window-manager-integration)
7. [Usage](#usage)
8. [Dashboard Module](#dashboard-module)
9. [Contributing](#contributing)
10. [License](#license)

---

## 1. Overview

`application_template` provides a minimal but fully functional starting point for GUI-based C++ applications. It uses:

* **ImGui** for immediate-mode GUI widgets
* **GLFW** for window and input management
* **OpenGL** (configurable via `GL_renderer`)
* **Premake5** for build configuration

The template handles platform detection, dependency setup, and IDE integration out of the box.

## 2. Features

* Cross-platform support (Linux; Windows support coming soon)
* Automatic and manual build workflows
* IDE configuration for VSCode (default) and others
* Customizable dashboard module for runtime statistics or visuals
* Git submodule management for external dependencies

## 3. Prerequisites

* **Git** (with submodule support)
* **C++20** compatible compiler (e.g., `g++`, `clang++`)
* **Python 3** (for setup script)

## 4. Installation & Setup

### Run Setup Script

Execute the Python setup script to verify and install missing dependencies, configure the environment, and generate IDE/project files:

```bash
python3 setup.py
```

* The script detects your OS (Linux only) and ensures all libraries and tools are available.
* Select your IDE when prompted (currently VSCode is supported).

## 5. Building the Application

### Automatic Build (VSCode)

If you selected **VSCode** during setup:

###### While in the VSCode editor:
* Press <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>B</kbd> to build the application.
* Press <kbd>F5</kbd> to launch the application in the debugger after building.

### Manual Build

From the project root directory:

```bash
cd application_template
.vscode/build.sh      # Generate gmake2 files and run premake
make -j               # Compile with parallel jobs
```

If you modify `premake5.lua`, regenerate build files:

```bash
make clean && vendor/premake/premake5 gmake2
make -j
```

**Optional Bash Aliases** (add to your `~/.bashrc`):

```bash
alias app_build='cd ~/workspace/application_template && .vscode/build.sh && make -j'
alias app_run='app_build && bin/Debug-linux-x86_64/application_template/application_template'
```

Replace `~/workspace/application_template` with your project path.

## 6. Window Manager Integration

If you use a tiling window manager (e.g., `Krohnkite`), ImGui may spawn floating glfw windows with a prefix. Add the following rule to allow floating windows:

```
ISM -
```

This matches ImGui popup windows by their title prefix.

## 7. Usage

Run the compiled binary directly or via IDE. The main window displays ImGui controls—customize widgets in `application.cpp`.

## 8. Dashboard Module

The dashboard files (`dashboard.h` / `dashboard.cpp`) are entry points for adding custom UI panels or runtime metrics. Extend this module by implementing:


### Lifecycle Methods

1. `dashboard::init()`

   * Called once at application startup.
   * Load fonts, initialize data sources (e.g., attach to performance counters or networking subsystems).
   * Allocate any persistent buffers or GPU textures if needed.

2. `dashboard::draw(f32 deltaTime)`

   * Called every frame after your main rendering loop begins.
   * Use ImGui calls to create windows, plots, and widgets.
   * Query real-time data (e.g., FPS, memory usage) and display it.
   * Example:

     ```cpp
     void dashboard::draw() {
         ImGui::Begin("Performance Metrics");
         ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
         ImGui::PlotLines("Frame Times (ms)", frameTimeBuffer.data(), frameTimeBuffer.size(), 0,
                          "Frame Time", 0.0f, 50.0f, ImVec2(0, 80));
         ImGui::End();
     }
     ```

3. `dashboard::update(f32 deltaTime)` *(optional)*

   * If you need to sample or compute data at a different rate than the render loop.
   * For example, update network throughput or perform expensive calculations.

4. `dashboard::shutdown()` *(optional)*

   * Clean up resources allocated in `initialize()` (free textures, buffers).
   * Called on application exit.

### Extending the Dashboard

* **Modular Panels**: Split different panels into private helper functions. Call them from `update()` and `draw()`:

  ```cpp
  void dashboard::draw() {
      draw_performance_panel();
      draw_network_panel();
      draw_custom_controls();
  }
  ```

* **Dynamic Layout**: Use `ImGui::BeginChild` and `ImGui::Columns` to arrange multiple panels in a single window.

* **Theming & Styling**: Customize colors and fonts via `ImGuiStyle` or push/pop style variables around specific widgets.

* **Input Handling**: Capture user inputs (e.g., toggling debug display) and store in Dashboard state to show or hide panels.

* **Unit Testing**: Abstract data collection into testable components. Inject mock data sources to verify plotting and formatting.


Example:

```cpp
void dashboard::draw() {
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    // Add your widgets here
    ImGui::End();
}
```

## 9. Contributing

Contributions welcome! Please fork the repo and submit pull requests against `main`. Ensure you:

* Follow existing coding style (C++20, clang-format)
* Update documentation when adding features
* Test on Linux (other OS support TBD)

## 10. License

This project is licensed under the [MIT License](LICENSE).
