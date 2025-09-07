cmake_minimum_required (VERSION 3.5.0)

project (iCON-tests)
enable_language (ASM C)

# Produce build file for clang-tidy
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform switch
if (WIN32)
    MESSAGE (STATUS "Configuring for Windows")
    set (PLATFORM "mingw32")
elseif (UNIX)
    MESSAGE (STATUS "Configuring for Linux (implied Ubuntu)")
    set (PLATFORM "ubuntu")
else ()
    MESSAGE (ERROR "Unsupported platform")
endif ()

enable_testing ()

add_custom_target(
    coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory coverage
    COMMAND gcovr -f ../main --html --html-details -o coverage/coverage.html
    COMMAND gcovr -f ../main --xml-pretty -o coverage/coverage.xml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

set(CMAKE_C_FLAGS          "--coverage")
set(CMAKE_EXE_LINKER_FLAGS     "-lgcov")

# Tests and test doubles
set(UT_INCLUDE_TEST_DIRECTORIES utFramework
                                utTestDoubles
                                utPlatformIncludes
                                )

# Source code header files
set(UT_INCLUDE_SRC_DIRECTORIES  ../main
                                ../main/app
                                ../main/middleware
                                ../main/driver
                                ../main/external
                                )

# Create target for coverage indexing
macro(accept_file_pattern pattern input_list)
  foreach(src_file ${${input_list}})
    if(${src_file} MATCHES ${pattern})
    else()
      list(REMOVE_ITEM ${input_list} "${src_file}")
    endif()
  endforeach()
endmacro()

macro(reject_file_pattern pattern input_list)
  foreach(src_file ${${input_list}})
    if(${src_file} MATCHES ${pattern})
      list(REMOVE_ITEM ${input_list} "${src_file}")
    endif()
  endforeach()
endmacro()

# Search for all source code files recursively
file(GLOB_RECURSE all_src   ../main/app/**/*.c
                            ../main/middleware/**/*.c
                            ../main/driver/**/*.c
                            )

add_library(gcov_all_sources EXCLUDE_FROM_ALL ${all_src})

target_include_directories(gcov_all_sources PRIVATE ${UT_INCLUDE_TEST_DIRECTORIES}
                                                    ${UT_INCLUDE_SRC_DIRECTORIES})

macro(create_test TEST_NAME)
    if(${ARGC} LESS 1)
        message(FATAL_ERROR "macro create_test: test name not provided")
    endif()

    add_executable (${TEST_NAME} ${ARGN})
    
    target_include_directories(${TEST_NAME} PRIVATE ${UT_INCLUDE_TEST_DIRECTORIES}
                                                    ${UT_INCLUDE_SRC_DIRECTORIES})

    target_link_libraries(${TEST_NAME} m)

    add_test (${TEST_NAME} ${TEST_NAME})
endmacro(create_test)
                                                    
# Include tests
include(main/middleware/middleware.cmake)
include(main/app/application.cmake)
include(main/external/external.cmake)
include(main/driver/driver.cmake)

# Print status
message(STATUS "Status:")
message(STATUS "  CMAKE_BUILD_NAME:  ${CMAKE_BUILD_NAME}")
message(STATUS "  BUILD DIRECTORY:   ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "  CMAKE_BUILD_TYPE:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  HW_VERSION:        ${HW_VERSION}")
message(STATUS "  PLATFORM:          ${PLATFORM}")
message(STATUS "  ENABLE_TESTS:      ${ENABLE_TESTS}")