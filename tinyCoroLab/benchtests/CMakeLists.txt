file(GLOB_RECURSE TINYCORO_BENCHTEST_SOURCES "${PROJECT_SOURCE_DIR}/benchtests/*bench.cpp")

set(benchbuild_list "")
set(benchtest_list "")

add_custom_target(_build-benchtests)

foreach (tinycoro_benchtest_source ${TINYCORO_BENCHTEST_SOURCES})
  get_filename_component(tinycoro_benchtest_filename ${tinycoro_benchtest_source} NAME)
  string(REPLACE ".cpp" "" tinycoro_benchtest_name ${tinycoro_benchtest_filename})
  add_executable(${tinycoro_benchtest_name} EXCLUDE_FROM_ALL ${tinycoro_benchtest_source})
  add_dependencies(_build-benchtests ${tinycoro_benchtest_name})

  target_link_libraries(${tinycoro_benchtest_name} ${PROJECT_NAME} benchmark)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${tinycoro_benchtest_name} PRIVATE "-g")
  endif()
  if(ENABLE_COMPILE_OPTIMIZE)
    target_compile_options(${tinycoro_benchtest_name} PUBLIC -O3)
  endif()

  set_target_properties(${tinycoro_benchtest_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchtests"
    COMMAND ${tinycoro_benchtest_name}
  )

  string(REPLACE "_bench" "" tinycoro_benchtest_command ${tinycoro_benchtest_name})
  add_custom_target(benchbuild-${tinycoro_benchtest_command}
    COMMAND echo "build ${tinycoro_benchtest_command} benchtest..."
    DEPENDS ${tinycoro_benchtest_name}
    COMMENT "build ${tinycoro_benchtest_command} benchtests..."
  )

  add_custom_target(benchtest-${tinycoro_benchtest_command}
    COMMAND $<TARGET_FILE:${tinycoro_benchtest_name}>
    DEPENDS ${tinycoro_benchtest_name}
    COMMENT "Running ${tinycoro_benchtest_command} benchtests..."
  )

  list(APPEND benchbuild_list benchbuild-${tinycoro_benchtest_command})
  list(APPEND benchtest_list benchtest-${tinycoro_benchtest_command})
endforeach()

add_custom_target(build-benchtests 
  COMMAND echo "[benchtests build command]"
  COMMAND ${PROJECT_SOURCE_DIR}/scripts/print_list.sh ${benchbuild_list}
  COMMAND echo "[benchtests run command]"
  COMMAND ${PROJECT_SOURCE_DIR}/scripts/print_list.sh ${benchtest_list}
  DEPENDS _build-benchtests
)