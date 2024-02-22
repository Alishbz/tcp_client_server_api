 
cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(TCP_API CXX)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    main.cpp 
)

add_executable(main ${SOURCES}  )
 