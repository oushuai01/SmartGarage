cmake_minimum_required(VERSION 3.6)
project(AIparking)

set(TOOLCHAINDIR "/usr/local/arm/5.4.0/usr/bin")

add_subdirectory(SQLite)
add_subdirectory(Audio)
add_subdirectory(Video)
add_subdirectory(RFID)

include_directories(include/)

add_executable(main main.c)
target_link_libraries(main pthread)