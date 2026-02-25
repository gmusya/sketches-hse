function(declare_task)
    set(HW_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    file(GLOB_RECURSE SKETCH_SOURCES ${HW_DIR}/sketch/*.cpp)
    file(GLOB_RECURSE SKETCH_HEADERS ${HW_DIR}/sketch/*.h ${HW_DIR}/sketch/*.hpp)

    get_filename_component(HW_NAME ${HW_DIR} NAME)

    set(HW_SKETCH sketch_${HW_NAME})
    set(HW_TESTS test_${HW_NAME})
    set(HW_BENCH bench_${HW_NAME})

    if (SKETCH_SOURCES)
        add_library(${HW_SKETCH} STATIC ${SKETCH_SOURCES} ${SKETCH_HEADERS})
        target_include_directories(${HW_SKETCH} PUBLIC ${HW_DIR})
    else()
        add_library(${HW_SKETCH} INTERFACE ${SKETCH_HEADERS})
        target_include_directories(${HW_SKETCH} INTERFACE ${HW_DIR})
    endif()

    file(GLOB_RECURSE TEST_SOURCES ${HW_DIR}/tests/*.cpp)
    
    add_executable(${HW_TESTS} ${TEST_SOURCES} ${CMAKE_SOURCE_DIR}/contrib/gmock_main.cc)
    target_link_libraries(${HW_TESTS} gmock ${HW_SKETCH})
    target_include_directories(${HW_TESTS} PUBLIC ${HW_DIR})

    file(GLOB_RECURSE BENCH_SOURCES ${HW_DIR}/bench/*.cpp)
    
    if (${BENCH_SOURCES})
        add_executable(${HW_BENCH} ${BENCH_SOURCES})
        target_link_libraries(${HW_BENCH} benchmark ${HW_SKETCH})
        target_include_directories(${HW_BENCH} PUBLIC ${HW_DIR})
    endif()

    if (TEST_SOLUTION)
        add_custom_target(
                run_${HW_TESTS}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS ${HW_TESTS}
                COMMAND ${CMAKE_BINARY_DIR}/${HW_TESTS})

        add_dependencies(test-all run_${HW_TESTS})

        if (${BENCH_SOURCES})
            add_custom_target(
                    run_${HW_BENCH}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    DEPENDS ${HW_BENCH}
                    COMMAND ${CMAKE_BINARY_DIR}/${HW_BENCH})

            add_dependencies(test-all run_${HW_BENCH})
        endif()

    endif()
endfunction()

add_custom_target(test-all)
