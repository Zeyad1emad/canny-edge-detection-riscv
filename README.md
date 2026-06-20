# Canny Edge Detection on RISC-V with Vector Extension

![CI](https://github.com/Zeyad1emad/canny-edge-detection-riscv/actions/workflows/ci.yml/badge.svg)

A full Canny edge detection pipeline implemented in C++, cross-compiled for RISC-V (rv64gcv), and executed on QEMU user-mode emulation. The project demonstrates scalar baseline, compiler optimization, and hand-optimized RVV (RISC-V Vector) intrinsic implementations.

**Authored by:** Dr. Omar Ahmed Nasr — Embedded Systems Course  
**Team Size:** 4 Students | **Language:** C++ | **Target:** RISC-V rv64gcv

---

## Table of Contents

- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Building the RISC-V Toolchain](#building-the-risc-v-toolchain)
- [Building QEMU](#building-qemu)
- [Build Reference](#build-reference)
- [Running the Pipeline](#running-the-pipeline)
- [Running Tests](#running-tests)
- [Python Scripts](#python-scripts)
- [Optimization Flags](#optimization-flags)
- [Pipeline Stages](#pipeline-stages)
- [Team & Contributions](#team--contributions)

---

## Project Structure

```
canny-edge-detection-riscv/
├── src/
│   ├── main.cpp              # Host / scalar RISC-V entry point
│   ├── main_rvv.cpp          # RVV-optimized entry point
│   ├── main_bench.cpp        # Benchmarking entry point
│   ├── gaussian_blur.cpp/.h  # Gaussian blur (scalar)
│   ├── sobel.cpp/.h          # Sobel gradient (scalar)
│   ├── magnitude.cpp/.h      # Gradient magnitude (scalar)
│   ├── direction.cpp/.h      # Gradient direction (scalar)
│   └── *_rvv.cpp             # RVV intrinsic implementations
├── tests/
│   ├── test_gaussian.cpp     # GoogleTest — Gaussian blur
│   ├── test_sobel.cpp        # GoogleTest — Sobel operator
│   ├── test_magnitude.cpp    # GoogleTest — Magnitude
│   ├── test_direction.cpp    # GoogleTest — Direction
│   ├── test_main.cpp         # GoogleTest — Full pipeline
│   ├── test_gaussian_rvv.cpp # QEMU-side equivalence test
│   ├── test_sobel_rvv.cpp    # QEMU-side equivalence test
│   └── test_magnitude_rvv.cpp# QEMU-side equivalence test
├── include/                  # Shared headers
├── run_qemu.py               # RISC-V runner + benchmark script
├── run_canny.py              # Host-side runner script
├── Makefile
└── README.md
```

---

## Prerequisites

### System Packages (Ubuntu / WSL2)

```bash
sudo apt-get update
sudo apt-get install -y \
  autoconf automake build-essential bison flex texinfo gperf \
  libtool patchutils bc git cmake \
  libglib2.0-dev libpixman-1-dev libslirp-dev ninja-build \
  libmpc-dev libmpfr-dev libgmp-dev zlib1g-dev libexpat-dev \
  libgtest-dev python3 python3-pip

pip3 install opencv-python numpy
```

### Windows Users (WSL2 Setup)

Open PowerShell as Administrator and run:

```powershell
wsl --install -d Ubuntu-24.04
```

After reboot, verify with:

```bash
uname -r   # should contain "microsoft"
```

Everything from this point forward runs inside the WSL2 Ubuntu terminal.

---

## Building the RISC-V Toolchain

> This step takes 30–90 minutes. Do it once.

```bash
# Clone the toolchain (shallow clone to save bandwidth)
git clone --recursive --depth 1 --shallow-submodules \
  https://github.com/riscv-collab/riscv-gnu-toolchain

cd riscv-gnu-toolchain

# Configure with Vector extension support
./configure \
  --prefix=$HOME/riscv-toolchain \
  --with-arch=rv64gcv \
  --with-abi=lp64d

# Build (uses all CPU cores)
make -j$(nproc)

# Add to PATH permanently
echo 'export PATH=$HOME/riscv-toolchain/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

**Verify the toolchain:**

```bash
riscv64-unknown-elf-g++ --version
# Expected: gcc version 14.x (or 13.x)

# Test that RVV headers are available
echo '#include <riscv_vector.h>' | riscv64-unknown-elf-g++ -march=rv64gcv -x c++ - -c -o /dev/null
# No output = success
```

---

## Building QEMU

```bash
# Clone QEMU (shallow clone)
git clone --depth 1 https://github.com/qemu/qemu
cd qemu

# Configure for user-mode RISC-V only (much faster than full build)
./configure --target-list=riscv64-linux-user --enable-plugins

# Build and install
make -j$(nproc)
sudo make install
```

**Verify QEMU:**

```bash
qemu-riscv64 --version
# Expected: QEMU emulator version 9.x
```

---

## Build Reference

All commands are run from the **project root directory**.

### Quick Start — Build Everything

```bash
make all
```

This builds: host pipeline, RVV pipeline, scalar RISC-V pipeline, and all tests.

---

### Block 1 — Host Side (PC / Native / GoogleTest)

#### Build Host Pipeline Executable

Compiles and links the full scalar pipeline natively on your PC using `g++`.

```bash
make run_host
# Output: pipeline_host.out
```

#### Build and Run All Host GoogleTests

Compiles each test file separately (avoids duplicate `main()` conflict) and runs them all.

```bash
make host_tests
```

This runs in order:
1. `test_gaussian_host` — Gaussian blur unit tests
2. `test_direction_host` — Direction quantization tests
3. `test_magnitude_host` — Magnitude computation tests
4. `test_sobel_host` — Sobel gradient tests
5. `test_main_host` — Full pipeline integration tests

#### Build a Single Host Test

```bash
make test_gaussian_host    # Gaussian only
make test_sobel_host       # Sobel only
make test_magnitude_host   # Magnitude only
make test_direction_host   # Direction only
make test_main_host        # Full pipeline only
```

Run a single test manually after building:

```bash
./test_gaussian_host
```

---

### Block 2 — RVV Side (RISC-V Vector / QEMU)

#### Build RVV Pipeline Executable

Cross-compiles the RVV-optimized pipeline using `riscv64-unknown-elf-g++`.

```bash
# Default VLEN=256
make run_rvv

# Custom VLEN (128, 256, or 512)
make run_rvv VLEN=128
make run_rvv VLEN=256
make run_rvv VLEN=512
# Output: pipeline_rvv.out
```

#### Build and Run All RVV Tests on QEMU

```bash
make rvv_tests
```

This compiles and runs on QEMU:
- `test_equiv_rvv` — Scalar vs RVV equivalence (all stages)
- `test_magnitude_rvv` — Magnitude RVV correctness

#### Build Individual RVV Tests

```bash
make test_gaussian_rvv     # Gaussian RVV equivalence
make test_sobel_rvv        # Sobel RVV equivalence
make test_magnitude_rvv    # Magnitude RVV equivalence
```

Run manually on QEMU with custom VLEN:

```bash
qemu-riscv64 -cpu rv64,v=true,vlen=128 ./test_magnitude_rvv
qemu-riscv64 -cpu rv64,v=true,vlen=256 ./test_magnitude_rvv
qemu-riscv64 -cpu rv64,v=true,vlen=512 ./test_magnitude_rvv
```

---

### Block 3 — RISC-V Scalar (No Vector Extension)

Used for performance comparison: same RISC-V binary but vectorization disabled.

#### Build RISC-V Scalar Executable

```bash
make build_rv_scalar
# Output: pipeline_rv_scalar.out
```

#### Run RISC-V Scalar on QEMU

```bash
# Run and pass width + height as arguments
make run_rv_scalar ARGS="640 480"

# Or run manually
qemu-riscv64 ./pipeline_rv_scalar.out 640 480 < input.raw > output.raw
```

---

### Run Both Test Suites (CI Target)

```bash
make test
# Equivalent to: make host_tests rvv_tests
```

---

### Clean All Build Artifacts

```bash
make clean
```

Removes: all test binaries, pipeline executables (`pipeline_host.out`, `pipeline_rvv.out`, `pipeline_rv_scalar.out`).

---

## Running the Pipeline

### Compiler Optimization Flag

You can override the optimization level for any build target:

```bash
make run_host OPT=-O0        # No optimization (slowest, for baseline)
make run_host OPT=-O2        # Moderate optimization
make run_host OPT=-O3        # Full optimization (default)
make run_host OPT=-Os        # Optimize for binary size
make run_host OPT=-Ofast     # Aggressive (may break IEEE compliance)
```

### Auto-Vectorization Report

To see which loops the compiler auto-vectorized:

```bash
make run_host OPT=-O3 VEC_REPORT=-fopt-info-vec-all 2>&1 | grep "vectorized"
```

---

## Python Scripts

### Script 1 — `run_canny.py` (Host-Side Runner)

Runs the native host pipeline on any image file (JPG, PNG, etc.).

**Usage:**

```bash
python3 run_canny.py <input_image> <output_image> [low_thresh] [high_thresh]
```

**Examples:**

```bash
# Basic usage with default thresholds (50, 150)
python3 run_canny.py car.jpg output.png

# Custom thresholds
python3 run_canny.py car.jpg edges.png 30 100

# High contrast image
python3 run_canny.py building.png result.png 80 200
```

**What it does internally:**
1. Reads the input image using OpenCV
2. Converts to grayscale
3. Saves as a temporary `.raw` file (width × height bytes, no headers)
4. Calls `./pipeline_host.out width height input.raw output.raw low high`
5. Reads the output `.raw` file and saves it as a PNG
6. Cleans up temporary files

**Requirements:**
- `./pipeline_host.out` must be compiled first: `make run_host`
- Python packages: `pip3 install opencv-python numpy`

---

### Script 2 — `run_qemu.py` (RISC-V Runner + Benchmark)

Runs the RISC-V pipeline on QEMU. Supports three modes: scalar, rvv, and benchmark.

#### Mode 1 — Scalar (RISC-V without vector extension)

```bash
python3 run_qemu.py -m scalar -i car.jpg -o output_scalar.png
python3 run_qemu.py -m scalar -i car.jpg -o output.png --low 30 --high 120
```

Runs `./pipeline_rv_scalar.out` on QEMU and saves the result as an image.

#### Mode 2 — RVV (RISC-V with vector extension)

```bash
python3 run_qemu.py -m rvv -i car.jpg -o output_rvv.png
python3 run_qemu.py -m rvv -i car.jpg -o output.png --low 50 --high 150
```

Runs `./pipeline_rvv.out` on QEMU and saves the result as an image.

#### Mode 3 — Benchmark (Scalar vs RVV Speedup)

Runs both binaries on the same image, extracts cycle counts from stderr, and computes the speedup ratio.

```bash
python3 run_qemu.py -m benchmark -i car.jpg
```

**Expected output:**

```
[*] Running SCALAR mode for baseline... (Binary: 45,312 bytes)
    -> Scalar Cycles: 12,845,230

[*] Running RVV mode for speedup calculation... (Binary: 52,104 bytes)
    -> RVV Cycles   : 3,211,308

=============================================
 SPEEDUP CALCULATION REPORT
=============================================
 Image            : car.jpg (640x480)
 Scalar Cycles    : 12,845,230
 RVV Cycles       : 3,211,308
---------------------------------------------
 Speedup Ratio    : 4.000x
=============================================
```

> **Note:** Your binary must print `AVERAGE CYCLES : <number>` to stderr for the benchmark mode to extract the cycle count. Add this to your `main_bench.cpp`.

#### All Script Options

```
-m / --mode     Required. One of: scalar, rvv, benchmark
-i / --input    Required. Input image path (jpg, png, etc.)
-o / --output   Output image path (default: output.png) — not needed for benchmark
--low           Low threshold for hysteresis (default: 50)
--high          High threshold for hysteresis (default: 150)
```

**Requirements:**
- Compile first: `make build_rv_scalar run_rvv`
- QEMU must be installed and in PATH
- Python packages: `pip3 install opencv-python numpy`

---

## Optimization Flags

| Flag | Description | Use Case |
|---|---|---|
| `-O0` | No optimization | Baseline measurement |
| `-O2` | Standard optimization | General use |
| `-O3` | Aggressive optimization | Performance benchmark |
| `-Os` | Optimize for size | Embedded binary size study |
| `-Ofast` | Ignore IEEE rules | Maximum speed experiment |
| `-fno-tree-vectorize` | Disable auto-vectorization | Scalar RISC-V baseline |
| `-fopt-info-vec-all` | Print vectorization report | Understand compiler decisions |

---

## Pipeline Stages

| Stage | Input | Output | File |
|---|---|---|---|
| 1. Gaussian Blur | Raw grayscale uint8 | Blurred uint8 | `gaussian_blur.cpp` |
| 2. Sobel Gradient | Blurred uint8 | Gx, Gy int16 (SoA) | `sobel.cpp` |
| 3. Gradient Magnitude | Gx, Gy int16 | Magnitude uint8 | `magnitude.cpp` |
| 4. Gradient Direction | Gx, Gy int16 | Direction uint8 (0/1/2/3) | `direction.cpp` |
| 5. Non-Max Suppression | Magnitude + Direction | Thinned edges | *(bonus)* |
| 6. Double Thresholding | Thinned edges | Strong/weak edges | *(bonus)* |
| 7. Hysteresis | Strong/weak edges | Final binary edges | *(bonus)* |

---

## Team & Contributions

| Member | Role | Phase 2 Task | Phase 6 Task |
|---|---|---|---|
| Student A | Infrastructure | Image I/O, Makefile, CI | Build system, QEMU config |
| Student B | Pipeline | Gaussian Blur | Gaussian RVV (widening) |
| Student C | Testing & Vectorization | Sobel Gradient | Sobel RVV (add/sub) |
| Student D | Magnitude | Gradient Magnitude | Magnitude RVV (reduction) |
| Student E | Direction | Gradient Direction | Direction RVV (masking) |

---

## References

- [RVV 1.0 Intrinsic Specification](https://github.com/riscv-non-isa/riscv-rvv-intrinsic-doc)
- [RISC-V Vector Extension Spec](https://github.com/riscv/riscv-v-spec)
- [RISC-V GNU Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
- [QEMU RISC-V Documentation](https://qemu.org/docs/master/system/target-riscv.html)
- [GoogleTest Documentation](https://google.github.io/googletest)
- [Compiler Explorer (test RVV online)](https://godbolt.org) — select `RISC-V rv64gcv` target

---

*This project was developed as part of the Embedded Systems course.*  
*Authored by Dr. Omar Ahmed Nasr and the project team.*