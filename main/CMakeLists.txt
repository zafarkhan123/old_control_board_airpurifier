
# Search for all source code files recursively
file(GLOB_RECURSE SOURCES   *.c
                            app/**/*.c
                            middleware/**/*.c
                            driver/**/*.c
                            external/**/*.c)

# Print all source files to compile
message(STATUS "SOURCES: ${SOURCES}")

# Set includes for all main project folders
set(INCLUDES    app
                middleware
                driver
                external)

# Print project includes
message(STATUS "INCLUDES: ${INCLUDES}")

# Create component containing all project files
idf_component_register( SRCS "main.c" "${SOURCES}"
                        INCLUDE_DIRS "${INCLUDES}"
                        )

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/app/webServer/front")
if(EXISTS ${WEB_SRC_DIR}/dist)
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
