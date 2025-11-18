mkdir build-linux-x86_64
cd build-linux-x86_64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/linux.toolchain.cmake -DTARGET_ARCH=x86_64
cd ..
mkdir build-linux-arm64
cd build-linux-arm64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/linux.toolchain.cmake -DTARGET_ARCH=arm64
