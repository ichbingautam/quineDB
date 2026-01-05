include(FetchContent)

# --- GoogleTest ---
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --- Unit Tests ---
add_executable(unit_tests
    unit/test_map.cpp
)

target_link_libraries(unit_tests
    PRIVATE
    GTest::gtest_main
    quine-storage
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(unit_tests)


# --- Google Benchmark ---
# Only build benchmarks if requested (or always for now)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

if(BUILD_BENCHMARKS)
    FetchContent_Declare(
      benchmark
      URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    add_executable(db_benchmarks
        benchmarks/bm_shard.cpp
    )

    target_link_libraries(db_benchmarks
        PRIVATE
        benchmark::benchmark
        quine-storage
    )

    target_include_directories(db_benchmarks PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
endif()
