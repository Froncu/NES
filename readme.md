# FroNES (W.I.P.)
An NES emulator written in **C++23** using **coroutines**.

## Roadmap
At the moment I'm implementing the **NMOS 6502** which served as the base for the **Ricoh 2A03** used in the NES. The reason why I didn't directly start with the actual processor used is because there is a lot more documentation available for the former in contrast to the latter. Once I verify the emulators accuracy (by using available test programs, among others [Klaus2m5's test suite](https://github.com/Klaus2m5/6502_65C02_functional_tests)), I will look into how I'd need to modify it in order to model the 2A03. Afterwards, I will move on to the next big step; **the PPU**.

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) - Core functionality (entry point, system events, input, windowing, etc.)
- [Dear ImGui](https://github.com/ocornut/imgui) - GUI
- [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) - Program loading

Those are managed through my custom [vcpkg](https://github.com/Froncu/vcpkg) fork and will be automatically installed during the configuration process.

## Quickstart

### Prerequisites

- C++23 compatible compiler
- The latest stable version of [Ninja](https://ninja-build.org/)
- [CMake](https://cmake.org/) 3.15 or higher

### Cloning

Clone the repository **with** the `--recursive` flag. This will cause the vcpkg submodule to also be pulled:

```bash
git clone https://github.com/Froncu/Fronge --recursive
```

### Configuring & Building

The project includes CMake presets (`CMakePresets.json`) that configure vcpkg integration automatically. You can configure and build using your preferred IDE or command line.

**Note:** If creating custom configure/build presets, ensure they inherit from/set `configurePreset` to the provided `default` preset to maintain proper vcpkg integration.

### Usage

The emulator's windows is split up in 2 main sections:
- **Memory**
   - There is an overview of the entire memory available. You can scroll or use the "**Jump to address**" input box to navigate to a desired location. Additionally, there are inputs for controlling both the **amount of bytes per row** and the **amount of visible rows**.
   - Most importantly, "**Select program**" will invoke the platform-native file open dialog and allow you to select a binary program to load into the emulator. "**Load address**" allows specifying where the load should take place in memory.
- **CPU**
   - You can view the state of any of the processor's registers.
   - You can set the "**Program counter**" to a desired value. By default, when the 6502 resets, it loads the value stored in the RESET vector (0xFFFC - 0xFFFD) into the program counter. However, some programs use this (and/or other) vector(s) for other purposes. For example, the [Klaus2m5's functional test](https://github.com/Klaus2m5/6502_65C02_functional_tests/blob/master/6502_functional_test.a65) uses the NMI, RESET and IRQ/BRK vectors as traps for unexpected behaviour and instead expects you to set the program counter to the correct address. Reason why I allow for this.
   - There is some basic execution control available:
      - **Tick** will execute 1 cycle
      - The **tick repeatedly** switch will resume/pause the emulation
      - **Step** will execute as many cycles as are needed to execute the current instruction
      - **Reset** will trigger a reset