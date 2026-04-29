cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Main lib
add_library(CXXRitoFile STATIC
    "src/wad.cpp"
    "src/skn.cpp"
    "src/bin.cpp"
    "src/anm.cpp"
    "src/mapgeo.cpp"
    "include/tex.hpp"
    "src/tex.cpp"
)
target_include_directories(CXXRitoFile 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/xxhash/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib/
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gzip-hpp/include/
            
)

target_compile_options(CXXRitoFile PRIVATE /wd4068 /W4 /WX)

# Executables
add_executable(TestRito "TestRito.cpp")
target_link_libraries(TestRito PRIVATE CXXRitoFile)

# ZLIB
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib)
target_link_libraries(CXXRitoFile PRIVATE zlibstatic)

# GZIP (ZLIB Wrapper)
add_definitions(-D ZLIB_CONST)

# ZSTD
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third-party/zstd/build/cmake)
target_link_libraries(CXXRitoFile PRIVATE libzstd_static)

# Links uwu
target_link_libraries(TestRito PRIVATE zlibstatic libzstd_static)