#Copyright (c) 2014, Sean Anderson All rights reserved.
#version requirements
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
#name
project(lisp)

#Include extra modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#Include Editline
find_package(Editline)
include_directories(${EDITLINE_INCLUDE_DIR})
set(LIBS ${LIBS} ${EDITLINE_LIBRARIES})
if(EDITLINE_FOUND)
  add_definitions(-DWITH_EDITLINE)
  link_libraries(edit)
endif(EDITLINE_FOUND)

#Link to math.h
if(UNIX)
  link_libraries(m)
endif(UNIX)

#Add c99 flag
add_definitions(-std=c99)
#add executable
add_executable(lisp lisp.c mpc.c)