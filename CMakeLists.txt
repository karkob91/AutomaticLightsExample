cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug messages
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMake generator: ${CMAKE_GENERATOR}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

# Read the JSON configuration file
file(READ "${CMAKE_SOURCE_DIR}/config.json" CONFIG_JSON)

# Parse the JSON configuration
string(JSON PROJECT_CONFIG GET ${CONFIG_JSON})
string(JSON PROJECT_NAME GET ${PROJECT_CONFIG} projectName)
string(JSON DEZYNE_RUNTIME_PATH GET ${PROJECT_CONFIG} dezyneRuntimePath)
string(JSON DZN_PATH GET ${PROJECT_CONFIG} dznPath)

# Validate paths from JSON
if(NOT PROJECT_NAME OR PROJECT_NAME STREQUAL "")
    message(FATAL_ERROR "Failed to parse projectName from config.json")
endif()

if(NOT DZN_PATH OR NOT DEZYNE_RUNTIME_PATH)
    message(FATAL_ERROR "Both 'dznPath' and 'dezyneRuntimePath' must be specified in config.json.")
endif()

# Append "dzn.cmd" or "dzn" to the path depending on the system
if(WIN32)
    set(DZN_TOOL "${DZN_PATH}/dzn.cmd")
else()
    set(DZN_TOOL "${DZN_PATH}/dzn")
endif()

# Ensure the tool exists
if(NOT EXISTS "${DZN_TOOL}")
    message(FATAL_ERROR "Dezyne tool not found at ${DZN_TOOL}")
endif()

# Debug output for JSON parsing
message(STATUS "Parsed project name: ${PROJECT_NAME}")
message(STATUS "Parsed Dezyne runtime path: ${DEZYNE_RUNTIME_PATH}")
message(STATUS "Dezyne tool path: ${DZN_TOOL}")

# Set project name
project(${PROJECT_NAME})

# Debug message for project confirmation
message(STATUS "Project successfully configured!")

###########################################################
## Dezyne Runtime                                        ##
###########################################################

add_library(DezyneRuntime STATIC)
file(GLOB DEZYNE_RUNTIME_FILES "${DEZYNE_RUNTIME_PATH}/*.cc" "${DEZYNE_RUNTIME_PATH}/dzn/*.hh")
target_sources(DezyneRuntime PRIVATE ${DEZYNE_RUNTIME_FILES})
target_include_directories(DezyneRuntime PUBLIC "${DEZYNE_RUNTIME_PATH}")

# Add a post-build step to run ranlib
add_custom_command(
    TARGET DezyneRuntime POST_BUILD
    COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:DezyneRuntime>
    COMMENT "Running ranlib on libDezyneRuntime.a"
)

###########################################################
## Dezyne Code Generation                                ##
###########################################################

file(GLOB_RECURSE DEZYNE_MODELS "Models/*.dzn")

set(DEZYNE_CODE_FILES)
foreach(DEZYNE_MODEL ${DEZYNE_MODELS})
    get_filename_component(MODEL_WE ${DEZYNE_MODEL} NAME_WE)
    set(MODEL_CC "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_WE}.cc")
    set(MODEL_HH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_WE}.hh")

    add_custom_command(
        OUTPUT "${MODEL_CC}" "${MODEL_HH}"
        COMMAND "${DZN_TOOL}" code "${DEZYNE_MODEL}"
        DEPENDS "${DEZYNE_MODEL}"
        COMMENT "Generating ${MODEL_CC} and ${MODEL_HH} from ${DEZYNE_MODEL}"
    )

    list(APPEND DEZYNE_CODE_FILES "${MODEL_CC}" "${MODEL_HH}")
endforeach()

add_custom_target(generate_dezyne_code ALL DEPENDS ${DEZYNE_CODE_FILES})

###########################################################
## Collect all source and header files in the root      ##
###########################################################

file(GLOB ROOT_SOURCES "*.cpp" "*.cc" "*.c")
file(GLOB ROOT_HEADERS "*.hh" "*.h")

###########################################################
## Main Executable                                       ##
###########################################################

add_executable(${PROJECT_NAME} ${ROOT_SOURCES} ${ROOT_HEADERS})

# Ensure the executable depends on code generation
add_dependencies(${PROJECT_NAME} generate_dezyne_code)

# Include directories for generated header files and project root
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}" # The generated header files are here
    "${CMAKE_SOURCE_DIR}"         # The root directory for other .cpp and .hh files
)

# Add generated sources conditionally after generation
foreach(DEZYNE_MODEL ${DEZYNE_MODELS})
    get_filename_component(MODEL_WE ${DEZYNE_MODEL} NAME_WE)
    set(MODEL_CC "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_WE}.cc")
    set(MODEL_HH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_WE}.hh")

    if(EXISTS "${MODEL_CC}")
        target_sources(${PROJECT_NAME} PRIVATE "${MODEL_CC}")
    endif()

    if(EXISTS "${MODEL_HH}")
        target_sources(${PROJECT_NAME} PRIVATE "${MODEL_HH}")
    endif()
endforeach()

# Compiler options and linking
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wpedantic -Wunused -Wold-style-cast -Wconversion)
target_link_libraries(${PROJECT_NAME} PRIVATE DezyneRuntime)

# Set the output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
