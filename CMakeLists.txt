#############
#  PROJECT  #
#############

cmake_minimum_required(VERSION 3.10)
project(dc-bot)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#############
#  CONCORD  #
#############

set(CONCORD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendors/concord)

include(ExternalProject)

ExternalProject_Add(concord_external
   SOURCE_DIR ${CONCORD_DIR}
   CONFIGURE_COMMAND ""
   BUILD_COMMAND make
   INSTALL_COMMAND ""
   BUILD_IN_SOURCE 1
)

find_library(CONCORD_LIB
   NAMES discord
   PATHS ${CONCORD_DIR}/lib
   NO_DEFAULT_PATH
)

###################
#  GET LIBRARIES  #
###################

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_library(CONCORD_LIB
   NAMES discord          
   PATHS ${CONCORD_DIR}/lib  
   NO_DEFAULT_PATH         
)

include(FetchContent)

FetchContent_Declare(
   ystar
   GIT_REPOSITORY https://github.com/vs-123/ystar.git
   GIT_TAG      main
)

FetchContent_MakeAvailable(ystar)

#################
#  CONFIG.JSON  #
#################

file(COPY ${CMAKE_SOURCE_DIR}/config.json DESTINATION ${CMAKE_BINARY_DIR})

############
#  TARGET  #
############

add_executable(${PROJECT_NAME}
   src/main.c
   src/cbot.c
   src/util.c
)
add_dependencies(${PROJECT_NAME} concord_external)

target_include_directories(${PROJECT_NAME} PRIVATE
   ${CONCORD_DIR}/include
   ${CONCORD_DIR}/gencodecs
   ${CONCORD_DIR}/core
   ${CURL_INCLUDE_DIRS}

   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
   ${CONCORD_LIB}
   ${CURL_LIBRARIES}
   ystar
   Threads::Threads
   m
)
