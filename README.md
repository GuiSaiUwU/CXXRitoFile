![Human made badge](https://img.shields.io/badge/human-coded-green?logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyNCIgaGVpZ2h0PSIyNCIgdmlld0JveD0iMCAwIDI0IDI0IiBmaWxsPSJub25lIiBzdHJva2U9IiNmZmZmZmYiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIiBjbGFzcz0ibHVjaWRlIGx1Y2lkZS1wZXJzb24tc3RhbmRpbmctaWNvbiBsdWNpZGUtcGVyc29uLXN0YW5kaW5nIj48Y2lyY2xlIGN4PSIxMiIgY3k9IjUiIHI9IjEiLz48cGF0aCBkPSJtOSAyMCAzLTYgMyA2Ii8+PHBhdGggZD0ibTYgOCA2IDIgNi0yIi8+PHBhdGggZD0iTTEyIDEwdjQiLz48L3N2Zz4=)

# CXXRitofilee
Basicly doing [pyritofile](https://github.com/GuiSaiUwU/pyritofile-package) in c++ yeah!


## Requirements:
- MSVC Build tools
- CMake & Ninja build tools
- Submodules

## How to compile:
### Clone the repo
```bash
git clone https://github.com/GuiSaiUwU/CXXRitoFile/ --recursive
```

### VSCode:
1. Ctrl+Shift+P
1. `Tasks: Run Task` and run `CMAKE: Build configure project`
1. Now you can debug with the launch `CMake: Test TestRito` or manually run the task `CMake: build project`

### Visual studio / Manually:
1. `cmake -B build -S . -G "Visual Studio 17 2022`
- Open the generated solution `(CXXRitoFilee.sln)` under `/build` if you want to use Visual Studio
- Run `cmake --build build --config Debug` if don't want to use an IDE