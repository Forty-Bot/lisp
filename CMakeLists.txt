# lisp-forty, a lisp interpreter
# Copyright (C) 2014-16 Sean Anderson
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

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

# Generated files go here
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

# Add the mpc include dir
include_directories(mpc ${GENERATED_DIR} "${CMAKE_SOURCE_DIR}/src")

# Include extra modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(src)

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
