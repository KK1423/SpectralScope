Windows build notes

- Install SFML 3 for your MSVC or MinGW toolchain.
  - Option A (vcpkg):
    - vcpkg install sfml3:x64-windows
    - Use `-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake` when invoking CMake
  - Option B (SFML prebuilt):
    - Download SFML 3 binaries for MSVC/MinGW and set `SFML_DIR` or `SFML_ROOT` so CMake can find it.

- Configure and build with CMake (from project root):

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DSFML_DIR="C:/Path/To/SFML/lib/cmake/SFML"
cmake --build build --config Release
```

- Configure and build with MSYS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
- Notes:
  - This project requires C++17.
  - We removed reliance on `M_PI` macros; math constants are provided in `math_constants.h`.
  - SFML provides cross-platform audio capture; ensure the SFML `audio` module is available for your build.
  - If using MinGW, ensure the correct SFML builds for MinGW are installed and `CMake` finds them.

  - MSYS2 / MinGW package note (pkgconf vs pkg-config):

    Recent MSYS2 packages use `mingw-w64-x86_64-pkgconf` (provides `pkg-config` functionality). Do NOT try to install `mingw-w64-x86_64-pkg-config` alongside `pkgconf` — they conflict. Use these safe commands in the **MSYS2 MinGW 64-bit** shell:

    ```bash
    # update
    pacman -Syu
    # install toolchain, cmake, ninja, pkgconf and SFML
    pacman -S --needed base-devel mingw-w64-x86_64-toolchain \
      mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
      mingw-w64-x86_64-pkgconf mingw-w64-x86_64-sfml
    ```

    If you already attempted an install and saw the conflict prompt, revert the partial choice and ensure `pkgconf` is installed (remove `mingw-w64-x86_64-pkg-config` if present):

    ```bash
    pacman -R mingw-w64-x86_64-pkg-config
    pacman -S mingw-w64-x86_64-pkgconf
    pacman -S mingw-w64-x86_64-cmake  # reinstall if needed
    ```

    This keeps `mingw-w64-x86_64-cmake` satisfied (it depends on `pkgconf`) and avoids the package conflict.
