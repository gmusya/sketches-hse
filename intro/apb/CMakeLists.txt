add_library(apb_lib sketch/add.cpp)
target_include_directories(apb_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(GTest REQUIRED)

add_executable(test_apb_public tests/test_public.cpp)
target_link_libraries(test_apb_public PRIVATE apb_lib GTest::gtest_main)
add_test(NAME apb_public COMMAND test_apb_public)
