# SpectralScope

SpectralScope is a real-time, C++ audio visualizer. It captures audio from your microphone, system audio (Windows WASAPI loopback), or a local audio file, and renders a dynamic, circular frequency spectrum using a custom Fast Fourier Transform (FFT) implementation and SFML 3.

## Features

* **Multiple Audio Sources:**
  * **System Loopback (Windows Only):** Captures desktop audio directly using WASAPI.
  * **Microphone Input:** Cross-platform microphone capture via SFML.
  * **File Playback:** Play and visualize local audio files.
* **Custom FFT Engine:** Includes a custom C++ Radix-2 Cooley-Tukey FFT implementation with configurable DFT sizes, decimation, and various windowing functions (Blackman-Nuttall, Flat Top, Hann).
* **Dynamic Visualization:** Renders a radial spectrum that dynamically scales and shifts color based on frequency amplitude and bass response.
* **Responsive UI:** The visualizer automatically adjusts to window resizing, maintaining aspect ratios and scaling smoothly.

## Controls

The application responds to keyboard inputs depending on your current audio source.

### File Playback Mode
* **`Space`**: Play / Pause
* **`Up` / `Down`**: Adjust volume up or down (Hold `Ctrl` for larger increments)
* **`Left` / `Right`**: Seek backward or forward by 5 seconds (Hold `Ctrl` to seek by 30 seconds)

### Live Audio Mode (Microphone / Loopback)
* **`Up`**: Increase input gain
* **`Down`**: Decrease input gain

## Dependencies

* **C++17** compatible compiler
* **CMake** (3.10 or higher)
* **SFML 3** (Modules: Graphics, Window, System, Audio)
* **Windows SDK** (Only required if building with Windows Loopback support for `Mmdeviceapi.h`, `Audioclient.h`, etc.)

## Building

SpectralScope uses CMake for its build system. By default, the build includes Windows loopback audio capture if you are building on a Windows machine.

### Standard Build (Linux / macOS / Windows)

```bash
# Clone the repository
git clone https://github.com/KK1423/SpectralScope.git
cd SpectralScope

# Configure the project
# Note: You may need to pass -DSFML_DIR=/path/to/sfml/lib/cmake/SFML if SFML is not in your system path
cmake -S . -B build 

# Build the project
cmake --build build --config Release

```

### Windows Build Notes

For specific instructions regarding Windows toolchains (MSVC, MinGW, MSYS2) and dealing with SFML 3 dependencies via `vcpkg` or `pacman`, please refer to the detailed [README_WINDOWS.md](README_WINDOWS.md).

### Configuration Options

You can tweak standard cache variables in CMake:

* `ENABLE_WINDOWS_LOOPBACK` (Default: `ON` on Windows) - Enables or disables the WASAPI loopback capture feature.
* `DFT_SIZE` (Default: `512`) - Changes the size of the Fast Fourier Transform (gives 256 bins of resolution in output).
* `DECIM` (Default: `16`) - FFT decimation factor. This defines the frequency range represented by the output (24KHz Nyquist /16 -> 1500Hz).

## Usage

Run the executable from your build directory.

**Live Capture (Loopback or Microphone):**
If run without arguments, SpectralScope will attempt to capture system audio (if built with loopback enabled on Windows). If loopback fails or is not supported, it falls back to the default microphone.

```bash
./main

```

**File Playback:**
Pass the path to an audio file as a command-line argument. The window title will indicate the file being played.

```bash
./main path/to/your/song.wav

```

