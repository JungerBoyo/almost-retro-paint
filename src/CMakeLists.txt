find_package(spdlog REQUIRED)
find_package(CLI11 REQUIRED)
find_package(ftxui REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(${PROJECT_NAME} 
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    CLI11::CLI11
    spdlog::spdlog
    
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") 
  target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
endif()