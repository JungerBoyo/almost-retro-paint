find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  target_compile_options(${PROJECT_NAME} INTERFACE --coverage -O0 -g)
  target_link_libraries(${PROJECT_NAME} INTERFACE --coverage)
endif()

add_test(NAME run_tests COMMAND catch2_main)
# ...
# set_tests_properties ( ... )

add_executable(catch2_main config.cpp)

add_subdirectory(hello_world)
# ...

target_link_libraries(catch2_main
  PRIVATE
    hello_world_TEST
  # ...  
)

target_link_system_libraries(catch2_main
  PRIVATE
    Catch2::Catch2
)