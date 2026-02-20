md release
cmake -B release -G "Visual Studio 18 2026" -DCMAKE_TOOLCHAIN_FILE="C:\Users\lmnt1x\Progs\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_BUILD_TYPE=Release
@REM cmake -B release -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="C:\Progs\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_BUILD_TYPE=Release