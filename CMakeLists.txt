cmake_minimum_required(VERSION 3.11)
project(sky_over_kharkov C)

set(CMAKE_C_STANDARD 99)

# Find raylib
find_package(raylib QUIET)

if (NOT raylib_FOUND)
    # If raylib is not found via find_package, try pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(raylib REQUIRED raylib)
endif()

# Add executable
add_executable(sky_over_kharkov main.c)

# Link raylib
if (TARGET raylib)
    target_link_libraries(sky_over_kharkov raylib)
else()
    target_include_directories(sky_over_kharkov PRIVATE ${raylib_INCLUDE_DIRS})
    target_link_libraries(sky_over_kharkov ${raylib_LIBRARIES})
endif()

# Link math library on Linux
if (UNIX AND NOT APPLE)
    target_link_libraries(sky_over_kharkov m)
endif()

# Copy images folder to build directory
file(COPY ${CMAKE_SOURCE_DIR}/images DESTINATION ${CMAKE_BINARY_DIR})
