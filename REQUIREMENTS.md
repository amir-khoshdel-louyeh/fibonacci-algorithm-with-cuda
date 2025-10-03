# Requirements

This project is a C++17 CUDA application with an optional Qt 6 desktop GUI.
Linux is the supported platform; CUDA does not run on macOS.

## Runtime

- An NVIDIA GPU with CUDA compute capability 3.5 or newer.
- An NVIDIA driver compatible with the installed CUDA Toolkit. CUDA 13.x
  requires a 580-series driver or newer. Check with:

  ```bash
  nvidia-smi
  ```

## Build tools

| Requirement | Minimum | Notes |
| --- | --- | --- |
| CMake | 3.24 | 3.28+ recommended |
| C++ compiler | C++17 | GCC 11+, Clang 14+, or MSVC 2022 |
| CUDA Toolkit | 12.0 | 13.5 tested; must include `nvcc` and CUDART headers |
| Qt 6 | 6.5 | Only needed for `-DBUILD_GUI=ON`, requires Widgets and Charts |

The CUDA Toolkit must provide both the compiler and the runtime development
headers. Installing only the compiler is not enough: `cuda_runtime.h` must be
available.

## Fedora

```bash
sudo dnf install gcc-c++ cmake cuda-toolkit-13-5
sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel
```

If the full toolkit is too large, the minimal set is:

```bash
sudo dnf install gcc-c++ cmake cuda-compiler-13-5 cuda-cudart-devel-13-5
```

## Ubuntu / Debian

Install the CUDA Toolkit from NVIDIA's repository, then:

```bash
sudo apt install build-essential cmake
sudo apt install qt6-base-dev libqt6charts6-dev
```

## Arch Linux

```bash
sudo pacman -S base-devel cmake cuda
sudo pacman -S qt6-base qt6-charts
```

## Verify the toolchain

```bash
nvcc --version
ldconfig -p | grep -E 'libcudart|libQt6Charts'
```

Both commands must find their libraries before configuring the project.

## Build options

- `-DBUILD_GUI=ON|OFF` (default `ON`): build the Qt 6 desktop application.
  Configure with `-DBUILD_GUI=OFF` on machines without Qt.
- `-DBUILD_TESTING=ON|OFF` (default `ON` via CTest): build and register tests.
