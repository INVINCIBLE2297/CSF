cmake_minimum_required(VERSION 3.10)
project(csfdemo LANGUAGES CXX)

set(CSF_BINARY_DIR ${CMAKE_BINARY_DIR}/csf)
add_subdirectory(../src ${CSF_BINARY_DIR})
add_executable(csfdemo CSFDemo.cpp)
target_link_libraries(csfdemo CSF)
if(OpenMP_CXX_FOUND)
    target_link_libraries(csfdemo PUBLIC CSF OpenMP::OpenMP_CXX)
endif()

install(TARGETS csfdemo DESTINATION bin)
