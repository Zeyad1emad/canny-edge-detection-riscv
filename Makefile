# ==========================================
# Compilers Configuration
# ==========================================
HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-g++

# ==========================================
# Compilation Flags
# ==========================================
HOST_FLAGS = -lgtest -lgtest_main -lpthread

RV_FLAGS   = -march=rv64gcv -O2

# ==========================================
# Build Targets
# ==========================================

# 1. TEST: Runs only one test file
test:
	@mkdir -p bin
	$(HOST_CXX) tests/test_gaussian.cpp src/gaussian_blur.cpp \
	$(HOST_FLAGS) -o bin/unit_tests
	./bin/unit_tests

# 2. CANNY_RV: Cross-compiles the pipeline for RISC-V target
canny_rv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) src/main.cpp -o bin_rv/canny_riscv

# 3. RUN: Executes the compiled RISC-V binary on QEMU
run: canny_rv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/canny_riscv

# 4. CLEAN
clean:
	rm -rf bin bin_rv
