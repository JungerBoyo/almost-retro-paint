cmake_minimum_required(VERSION 3.20)

### before-project config ###

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

#############################


### project setup ###

project(almost-retro-paint
    VERSION 0.0.1
)

######################


### sub dirs ###

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(config)
add_subdirectory(src)

################

### package project ###
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "./")
include(CPack)
########################