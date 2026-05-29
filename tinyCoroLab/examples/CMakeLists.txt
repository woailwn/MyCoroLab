file(GLOB SOURCES "*.cpp")

set(debug_name coro_debug)

# auto build new examples
foreach(SOURCE ${SOURCES})
    get_filename_component(FILENAME "${SOURCE}" NAME)
    get_filename_component(TARNAME "${SOURCE}" NAME_WE)
    add_executable(${TARNAME} ${FILENAME})
    target_link_libraries(${TARNAME} ${PROJECT_NAME})
    target_compile_definitions(${TARNAME} PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
    target_compile_definitions(${TARNAME} PRIVATE "$<$<CONFIG:RELEASE>:RELEASE>")

    # if(${TARNAME} STREQUAL ${debug_name})
    #     target_compile_options(${TARNAME} PRIVATE "-g")
    # else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(${TARNAME} PRIVATE "-g")
    endif()
    if(ENABLE_COMPILE_OPTIMIZE)
      target_compile_options(${TARNAME} PUBLIC -O3)
    endif()
    # endif()
endforeach()

if(TARGET ${debug_name})
    add_custom_target(build-debug
        COMMAND echo "build ${debug_name}..."
        DEPENDS ${debug_name}
    )
    add_custom_target(run-debug
        COMMAND $<TARGET_FILE:${debug_name}>
        DEPENDS ${debug_name}
    )
endif()