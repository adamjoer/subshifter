cmake_minimum_required(VERSION 3.16)
project(subshifter)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES
    main.cpp
    subtitle_shifter.cpp subtitle_shifter.h
    time_stamp.h
)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.32 REQUIRED COMPONENTS program_options)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif()
