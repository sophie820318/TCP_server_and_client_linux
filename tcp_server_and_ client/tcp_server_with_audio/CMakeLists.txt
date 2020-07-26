#指定cmake最小版本
cmake_minimum_required(VERSION 3.5.1)
project (servertest)
#new add properties
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -g")

#源文件
set(SOURCES 
    src/main.cpp
    src/tcpserver.cpp
    src/pthread_define.c
    src/audio_capture.c
    )

#链接pthread库  
link_libraries(-lpthread -lasound)

add_executable(${PROJECT_NAME} ${SOURCES})

#头文件
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include 
    )
