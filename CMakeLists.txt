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
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
# name
project(lisp-forty VERSION 0.1.1.2)

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

# Fix isatty() includes
if(WIN32)
  add_definitions(-DWINDOWS)
endif(WIN32)

# Link to math.h
if(UNIX)
  link_libraries(m)
endif(UNIX)

# Compile in the standard library
add_custom_command(
	OUTPUT std_lisp.h
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/std.lisp std.lisp
	COMMAND xxd -i std.lisp std_lisp.h
	DEPENDS std.lisp
)

#Configure version at compile-time
configure_file(version.h.in version.h @ONLY)

# Add c99 flag
add_definitions(-std=c99)
# Add the mpc include dir
include_directories(mpc ${CMAKE_CURRENT_BINARY_DIR})
# Make the lisp
add_executable(${PROJECT_NAME} lisp.c mpc/mpc.c mpc/mpc.h std_lisp.h)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")

set(GENERATE_PACKAGE OFF CACHE BOOL "Generate binary package target")

# Change the installation dir or README.md/LICENSE depending on whether we are making a package
if(GENERATE_PACKAGE)
	set(DOC_DIR ".")
else(GENERATE_PACKAGE)
	set(DOC_DIR "share/doc/${PROJECT_NAME}")
endif(GENERATE_PACKAGE)

install(FILES "${CMAKE_SOURCE_DIR}/README.md" "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION "${DOC_DIR}")

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(GENERATE_PACKAGE)
	include(CPack)
endif(GENERATE_PACKAGE)
