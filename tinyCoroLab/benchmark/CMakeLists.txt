file(GLOB_RECURSE TINYCORO_BENCH_SOURCES "${PROJECT_SOURCE_DIR}/benchmark/*/*.cpp")

add_custom_target(build-bench COMMAND echo "Building benchmark case...")

add_custom_target(build-benchtools
  COMMAND cargo run --release --manifest-path=${PROJECT_SOURCE_DIR}/third_party/rust_echo_bench/Cargo.toml -- --help
)

foreach (tinycoro_bench_source ${TINYCORO_BENCH_SOURCES})
  get_filename_component(tinycoro_bench_filename ${tinycoro_bench_source} NAME)
  string(REPLACE ".cpp" "" tinycoro_bench_name ${tinycoro_bench_filename})
  add_executable(${tinycoro_bench_name} EXCLUDE_FROM_ALL ${tinycoro_bench_source})
  add_dependencies(build-bench ${tinycoro_bench_name})

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${tinycoro_bench_name} PRIVATE "-g")
  endif()
  if(ENABLE_COMPILE_OPTIMIZE)
    target_compile_options(${tinycoro_bench_name} PUBLIC -O3)
  endif()
  target_link_libraries(${tinycoro_bench_name} ${PROJECT_NAME})

  set_target_properties(${tinycoro_bench_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmark"
  )

  string(REPLACE "_bench" "" tinycoro_bench_command ${tinycoro_bench_name})
  add_custom_target(bench_${tinycoro_bench_command}
    COMMAND $<TARGET_FILE:${tinycoro_bench_name}>
    DEPENDS ${tinycoro_bench_name}
    COMMENT "Running ${tinycoro_bench_command} bench..."
  )
endforeach()
