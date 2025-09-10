mkdir build-ios
cd build-ios
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/ios.toolchain.cmake -DPLATFORM=OS64 -G Xcode