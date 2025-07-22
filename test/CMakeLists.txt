include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE 
)


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)



include(CTest)
find_package(GTest REQUIRED)


# Test_Render
add_executable(Test_Truchas_Render test_render.cpp)
target_include_directories(Test_Truchas_Render PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(Test_Truchas_Render PRIVATE truchas)
target_link_libraries(Test_Truchas_Render INTERFACE pch_interface)

target_include_directories(Test_Truchas_Render PRIVATE ${GTEST_INCLUDE_DIRS} )                               
target_link_libraries(Test_Truchas_Render PRIVATE ${GTEST_BOTH_LIBRARIES})


include(GoogleTest)
gtest_discover_tests(Test_Truchas_Render)