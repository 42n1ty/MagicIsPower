md release
cmake --preset rel
@REM cmake -B release -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="C:\Progs\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_BUILD_TYPE=Release