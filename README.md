# SapphireLauncherCpp

Super simple command line launcher for [Sapphire](https://github.com/SapphireServer/Sapphire). Developed for use on MacOS with [Whisky](https://github.com/Whisky-App/Whisky), but should work on Windows too.

# Usage

- Place `sapphire.exe` next to `ffxiv_dx11.exe` (`game` folder).
- Run `sapphire.exe` to create a template `sapphire.ini`.
- Adjust the `username` and `password` to match your setup.
- If your user does not yet exist, run `sapphire.exe -create` to create it. Otherwise run `sapphire.exe` to start the game.

# Building

```sh
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
```

You can use [zig-cross](https://github.com/mrexodia/zig-cross) with `--toolchain ~/zig-cross/x86_64-windows-gnu.cmake` when configuring CMake to cross-compile for Windows on MacOS.

# Configuration

The `sapphire.ini` file has the follow fields:

```toml
[sapphire]
api = http://127.0.0.1:80
username = <username>
password = <password>
executable = ffxiv_dx11.exe
debugger =
languageIndex = 1
```

## Languages
```js
日本語 = 0
English = 1
Deutsch = 2
Français = 3
```

## Debugging

- If you run `sapphire.exe -nostart` the launcher will print the command line without starting the game.
- You can set `debugger = C:\x64dbg\release\x96dbg.exe` and hold down _Ctrl_ or run `sapphire.exe -debug` to start the game under a debugger.
