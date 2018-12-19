cmake_minimum_required(VERSION 3.8)

project(ArkanoidServer)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Setting the C++ standard directly
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Src/network)

# Create files lists
file(GLOB_RECURSE SRC_NETWORK
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/network/*.*"
)

file(GLOB_RECURSE SRC_GAME
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/game/*.*"
)

file(GLOB_RECURSE SRC_PROTOCOL
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/protocol/*.*"
)

# Add the executable
add_executable(ArkanoidServer Src/ArkanoidServer.cpp
               ${SRC_NETWORK}
               ${SRC_GAME}
               ${SRC_PROTOCOL})

# Add source groups in project
source_group("network" FILES ${SRC_NETWORK})
source_group("game" FILES ${SRC_GAME})
source_group("protocol" FILES ${SRC_PROTOCOL})

# Add link libraries
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
target_link_libraries(ArkanoidServer LINK_PRIVATE Ws2_32 Iphlpapi)
endif()
