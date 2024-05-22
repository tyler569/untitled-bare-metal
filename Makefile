.PHONY: all

all:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64.cmake -G Ninja
	cmake --build build

