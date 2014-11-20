# lisp-forty, a lisp interpreter
# Copyright (C) 2014 Sean Anderson
#
# This file is part of lisp-forty.
#
# lisp-forty is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lisp-forty is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lisp-forty.  If not, see <http://www.gnu.org/licenses/>.


# version requirements
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
# name
project(lisp-forty)

# Include extra modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Include Editline
find_package(Editline)
if(EDITLINE_FOUND)
  include_directories(${EDITLINE_INCLUDE_DIR})
  set(LIBS ${LIBS} ${EDITLINE_LIBRARIES})
  add_definitions(-DWITH_EDITLINE)
  link_libraries(edit)
endif(EDITLINE_FOUND)

# Link to math.h
if(UNIX)
  link_libraries(m)
endif(UNIX)

# Add c99 flag
add_definitions(-std=c99)
# Add the mpc include dir
include_directories(mpc)
# Make the lisp
add_executable(lisp-forty lisp.c mpc/mpc.c mpc/mpc.h)
