cmake_minimum_required(VERSION 3.20)
project(quine VERSION 0.1.0 LANGUAGES CXX)

# Global Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Compiler Warnings (High strictness for DB)
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    # add_compile_options(-Wconversion -Wshadow) # Enable later
endif()

# Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include Directories
include_directories(include)

if(APPLE)
    include_directories(include/stub)
    add_definitions(-DQUINE_MOCK_URING)
endif()

# Dependencies (Example stub)
find_package(Threads REQUIRED)
# find_package(LibUring) # Uncomment when we add the module and use it

# Subdirectories
add_subdirectory(src)
# add_subdirectory(tests)
# add_subdirectory(tools)
