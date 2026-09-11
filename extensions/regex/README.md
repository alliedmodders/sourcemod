# PCRE2 Build Instructions
As of writing, we're using pcre2 v10.48

https://github.com/PCRE2Project/pcre2/releases/tag/pcre2-10.48

## Configuration

Use the following CMake configuration to configure pcre2

```
cmake -B build \
    -DBUILD_SHARED_LIBS=OFF \
    -DPCRE2_BUILD_PCRE2_8=ON \
    -DPCRE2_BUILD_PCRE2_16=OFF \
    -DPCRE2_BUILD_PCRE2_32=OFF \
    -DPCRE2_BUILD_PCRE2GREP=OFF \
    -DPCRE2_BUILD_TESTS=OFF \
    -DPCRE2_SUPPORT_JIT=OFF
```

Use the -A flag to specify what architecture you want on Windows

On Windows, append these 2 options

```
-DCMAKE_POLICY_DEFAULT_CMP0091=NEW
-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
```

## Build

Use the following command to build the pcre2 static library

```
cmake --build build --config Release --target pcre2-8-static
```
