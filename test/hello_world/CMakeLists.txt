add_library(hello_world_TEST OBJECT hello_world_test.cpp)

target_link_libraries(hello_world_TEST
  PRIVATE
    hello_world_INC
    hello_world_IMPL
)

target_link_system_libraries(hello_world_TEST
  PRIVATE 
    Catch2::Catch2
)