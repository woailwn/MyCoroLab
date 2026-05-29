set(BENCHMARK_ENABLE_TESTING NO CACHE BOOL "Disable Google Benchmark tests")

add_subdirectory(spdlog)
add_subdirectory(googletest)
add_subdirectory(benchmark)
