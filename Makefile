# ==========================================
# Compilers Configuration
# ==========================================
HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-g++

# ==========================================
# Compilation Flags
# ==========================================
HOST_FLAGS = -Iinclude -lgtest -lgtest_main -lpthread -lm
RV_FLAGS   = -Iinclude -march=rv64gcv -O2 -lm

# ==========================================
# Source Files
# ==========================================
SRC_FILES = src/gaussian_blur.cpp src/sobel.cpp

# ==========================================
# Build Targets
# ==========================================

# 1. TEST: Runs the comprehensive test suite
test:
	@mkdir -p bin
	$(HOST_CXX) $(SRC_FILES) tests/test_main.cpp $(HOST_FLAGS) -o bin/unit_tests
	./bin/unit_tests

# 2. CANNY_RV: Cross-compiles the pipeline for RISC-V target
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) $(SRC_FILES) src/main.cpp -o bin_rv/canny_riscv

# 3. RUN: Executes the compiled RISC-V binary on QEMU
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv

# 4. CLEAN
clean:
	rm -rf bin bin_rv

.PHONY: test canny_rv run clean
