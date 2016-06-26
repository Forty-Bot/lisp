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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}" "*.c")
add_executable(${PROJECT_NAME} ${SOURCES} "${CMAKE_SOURCE_DIR}/mpc/mpc.c")
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")

# Include Editline
find_package(Editline)
if(EDITLINE_FOUND)
	include_directories(${EDITLINE_INCLUDE_DIR})
	set(LIBS ${LIBS} ${EDITLINE_LIBRARIES})
	add_definitions(-DWITH_EDITLINE)
	target_link_libraries(${PROJECT_NAME} edit)
endif(EDITLINE_FOUND)

# Fix isatty() includes
if(WIN32)
  add_definitions(-DWINDOWS)
endif(WIN32)

# Link to math.h
if(UNIX)
	target_link_libraries(${PROJECT_NAME} m)
endif(UNIX)

# Compile in the standard library
set(STD_LIB_IN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/std.lisp")
set(STD_LIB_OUT_FILE "${GENERATED_DIR}/std_lisp.h")
add_custom_command(
	OUTPUT ${STD_LIB_OUT_FILE}
	COMMAND ${CMAKE_COMMAND} -E copy ${STD_LIB_IN_FILE} std.lisp
	COMMAND xxd -i std.lisp ${STD_LIB_OUT_FILE}
	DEPENDS ${STD_LIB_IN_FILE}
)

#Configure version at compile-time
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${GENERATED_DIR}/version.h" @ONLY)
add_custom_target(generate_headers ALL DEPENDS "${GENERATED_DIR}/version.h" "${GENERATED_DIR}/std_lisp.h")

# Add -Wall or equivalent
if(MSVC)
	# Force to always compile with W4
	if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
endif()

# Set the standard to C++11
set(C_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)
