find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(CLI11 REQUIRED)
find_package(ftxui REQUIRED)

add_executable(${PROJECT_NAME} 
  main.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE 
    project_options
    project_warnings
)

target_link_system_libraries(${PROJECT_NAME}
  PRIVATE
    CLI11::CLI11
    fmt::fmt
    spdlog::spdlog
    
    ftxui::screen
    ftxui::dom
    ftxui::component
)

target_include_directories(${PROJECT_NAME} 
  PRIVATE 
    "${CMAKE_BINARY_DIR}/config"
)
