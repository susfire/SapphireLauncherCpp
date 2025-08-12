# SapphireLauncherCpp

Super simple command line launcher for [Sapphire](https://github.com/SapphireServer/Sapphire). Developed for use on MacOS with [Whisky](https://github.com/Whisky-App/Whisky), but should work on Windows too.

# Usage

- Place `sapphire.exe` next to `ffxiv_dx11.exe` (`game` folder).
- Run `sapphire.exe` to create a template `sapphire.ini`.
- Adjust the `username` and `password` to match your setup.
- If your user does not yet exist, run `sapphire.exe create` to create it. Otherwise run `sapphire.exe` to start the game.

# Building

```sh
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
```

You can use [zig-cross](https://github.com/mrexodia/zig-cross) with `--toolchain ~/zig-cross/x86_64-windows-gnu.cmake` when configuring CMake to cross-compile for Windows on MacOS.
