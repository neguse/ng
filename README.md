# ng

ng : This is a game engine / framework / library for neguse.

## Motivation

* Extremely fast development of 2D video game prototyping.
* Almost no need of asset creation.

## Build

```
Install vcpkg
./vcpkg install --triplet x64-<windows|linux|osx> glm sdl2 sdl2-image fmt gl3w freetype

If you use VSCode, add below lines to .vscode/settings.json
    "cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
    }
```