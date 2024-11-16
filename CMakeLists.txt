cmake_minimum_required(VERSION 3.16)

project(tbtennis_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS
    main.cpp
    simskt.cpp
)

add_executable(tbtennis_server ${SRCS})

target_link_libraries(tbtennis_server ws2_32)
