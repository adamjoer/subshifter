cmake_minimum_required(VERSION 3.16)
project(subshifter)

set(CMAKE_CXX_STANDARD 17)

add_executable(subshifter
    main.cpp
    subtitle_shifter.cpp subtitle_shifter.h
    time_stamp.h
)
