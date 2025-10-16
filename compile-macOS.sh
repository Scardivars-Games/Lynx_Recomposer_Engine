mkdir build-macOS
cd build-macOS
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/macos.toolchain.cmake -G Xcode
