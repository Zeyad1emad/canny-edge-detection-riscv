HOST_CXX = g++
RV_CXX   = riscv64-unknown-elf-clang++

GTEST_ROOT = $(HOME)/gtest-install
HOST_FLAGS = -std=c++17 -I$(GTEST_ROOT)/include -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread

RV_FLAGS = -march=rv64gcv -O2 -target riscv64-unknown-elf --gcc-toolchain=$(HOME)/riscv -static

test:
	@mkdir -p bin
	$(HOST_CXX) $(HOST_FLAGS) -Isrc tests/test_magnitude.cpp src/magnitude.cpp -o bin/unit_tests
	./bin/unit_tests

magnitude_rvv:
	@mkdir -p bin_rv
	$(RV_CXX) $(RV_FLAGS) -Isrc tests/test_magnitude_rvv.cpp src/magnitude.cpp src/magnitude_rvv.cpp -o bin_rv/test_magnitude_rvv

run_rvv_128: magnitude_rvv
	qemu-riscv64 -cpu rv64,v=true,vlen=128 ./bin_rv/test_magnitude_rvv

run_rvv_256: magnitude_rvv
	qemu-riscv64 -cpu rv64,v=true,vlen=256 ./bin_rv/test_magnitude_rvv

run_rvv_512: magnitude_rvv
	qemu-riscv64 -cpu rv64,v=true,vlen=512 ./bin_rv/test_magnitude_rvv

clean:
	rm -rf bin bin_rv
