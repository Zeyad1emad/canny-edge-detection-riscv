# ==========================================
# Compilers Configuration
# ==========================================
# Native compiler for host-side testing (x86/ARM)
HOST_CXX = g++

# Cross-compiler for RISC-V target with Vector support
RV_CXX   = riscv64-unknown-elf-g++

# ==========================================
# Compilation Flags
# ==========================================
# Path to GoogleTest was previously defined in ~/.bashrc as GTEST_ROOT
HOST_FLAGS = -I$(GTEST_ROOT)/include -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread

# RISC-V flags targeting 64-bit, GCV (General + Compressed + Vector) architecture [cite: 29]
RV_FLAGS   = -march=rv64gcv -O2

# ==========================================
# Build Targets
# ==========================================
# 1. TEST: Compiles and runs the GoogleTest suite natively on the host
test:
	@mkdir -p bin
	$(HOST_CXX) tests/test_main.cpp tests/test_sobel.cpp src/canny.cpp src/sobel.cpp $(HOST_FLAGS) -o bin/unit_tests
	./bin/unit_tests

# 2. CANNY_RV: Cross-compiles the pipeline for RISC-V target
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) src/main.cpp src/canny.cpp src/sobel.cpp -o bin_rv/canny_riscv

# 3. RUN: Executes the compiled RISC-V binary on QEMU with VLEN=128
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv

# 4. CLEAN: Removes all generated binary and object files
clean:
	rm -rf bin bin_rv






