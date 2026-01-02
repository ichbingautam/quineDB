add_executable(zephyra-server main.cpp)

target_include_directories(zephyra-server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link threads (needed for std::thread)
find_package(Threads REQUIRED)
target_link_libraries(zephyra-server PRIVATE Threads::Threads)
