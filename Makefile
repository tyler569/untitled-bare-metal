.PHONY: all allx86 allarm clean

all: allx86 allarm

allx86:
	cmake -B buildx86 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64.cmake -G Ninja
	cmake --build buildx86

allarm:
	cmake -B buildarm -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64.cmake -G Ninja
	cmake --build buildarm

clean:
	rm -rf buildx86 buildarm cmake-build-*
