# Canny Edge Detection on RISC-V with Vector Extension

This project implements a Canny edge detection pipeline targeting the RISC-V (rv64gcv) architecture. It demonstrates the optimization journey from a clean scalar C++ baseline to a hand-optimized version using RISC-V Vector (RVV) intrinsics.

## Prerequisites

To build and run this project from scratch, the following environment is required:

- WSL2 with Ubuntu 24.04
- RISC-V GNU Toolchain built from source (rv64gcv support)
- QEMU built from source (riscv64-linux-user)
- GoogleTest installed and configured in environment paths

## Project Structure

- **src/**: Contains core algorithm logic including Gaussian Blur and Sobel Operator.
- **tests/**: Includes GoogleTest unit tests for host-side logic verification.
- **bin/**: Output directory for native host binaries.
- **bin_rv/**: Output directory for RISC-V cross-compiled binaries.

## How to Build and Run

1. **Clone the repository:**
```bash
   git clone https://github.com/Zeyad1emad/canny-edge-detection-riscv.git
   cd canny-edge-detection-riscv
```

2. **Run Host-side Tests (Native):** Executes unit tests on your local machine to verify algorithm logic.
```bash
   make test
```

3. **Cross-compile for RISC-V:** Builds the pipeline using the RISC-V cross-compiler.
```bash
   make canny_rv
```

4. **Run on QEMU:** Executes the RISC-V binary on the emulator with VLEN=128.
```bash
   make run
```

5. **Clean Project:** Removes all generated binaries and object files to start a fresh build.
```bash
   make clean
```

## Team Members (Team 5)

- **Zeyad Emad** - 
- **Ziad Hany** — 
- **Yassen Waeel**-
- **Nancy Salah**-
- **Nor Ahmed**-
