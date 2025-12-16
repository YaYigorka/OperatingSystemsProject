cmake_minimum_required(VERSION 3.10)
project(MessengerPipe)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    .
    common
    server
    client
)

add_executable(messenger
    main.cpp
    common/common.cpp
    server/server.cpp
    client/client.cpp
)

if(UNIX)
    target_link_libraries(messenger pthread)
endif()