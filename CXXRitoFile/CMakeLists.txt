cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RitoFileCPP "TestRito.cpp"
	"src/wad.cpp"
	"include/structs.hpp"
	"include/binary_stream.hpp"
	"src/hash.cpp"
	"src/skn.cpp"
)
target_include_directories(RitoFileCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

# ZLIB
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib)
target_include_directories(RitoFileCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib/)
target_link_libraries(RitoFileCPP PRIVATE zlibstatic)

# GZIP (ZLIB Wrapper)
add_definitions(-D ZLIB_CONST)
target_include_directories(RitoFileCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gzip-hpp/include/)

# XXHash For WAD hashing uwu
target_include_directories(RitoFileCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/xxhash/)

# ZSTD
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third-party/zstd/build/cmake)
target_include_directories(RitoFileCPP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zstd/lib/)
target_link_libraries(RitoFileCPP PRIVATE libzstd_static)
