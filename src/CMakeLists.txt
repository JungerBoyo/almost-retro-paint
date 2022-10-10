add_subdirectory(hello_world)

add_executable(${PROJECT_NAME} 
  main.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE 
    project_options
    project_warnings
    hello_world_IMPL
)

target_include_directories(${PROJECT_NAME} 
  PRIVATE 
    "${CMAKE_BINARY_DIR}/config"
)
