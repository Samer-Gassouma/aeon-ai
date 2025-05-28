cmake_minimum_required(VERSION 3.16)
project(AIQuizAPI)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimizations for CPU performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG")

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(jsoncpp REQUIRED)

# Set output directories to organize build artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/llama.cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/llama.cpp/ggml/include)

# Configure llama.cpp build options
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama: build tests" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama: build examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama: build server" FORCE)

# Add llama.cpp as subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/llama.cpp)

# Source files
set(SOURCES
    src/main.cpp
    src/ai_quiz_generator.cpp
    src/http_server.cpp
)

# Create executable
add_executable(ai_quiz_server ${SOURCES})

# Configure RPATH to find libraries relative to executable location
set_target_properties(ai_quiz_server PROPERTIES
    # RPATH for finding libraries at runtime
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/lib:$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE
    # Additional runtime search paths
    BUILD_RPATH "${CMAKE_BINARY_DIR}/lib"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Link libraries in correct order
target_link_libraries(ai_quiz_server
    PRIVATE
    llama
    ggml
    Threads::Threads
    jsoncpp
    ssl
    crypto
    dl
    m
)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ai_quiz_server PRIVATE
        -mavx2
        -mfma
        -pthread
        -fPIC
    )
endif()

# Copy llama libraries to our lib directory after build
add_custom_command(TARGET ai_quiz_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:llama>
        ${CMAKE_BINARY_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ggml>
        ${CMAKE_BINARY_DIR}/lib/
    COMMENT "Copying llama.cpp libraries to lib directory"
)

# Copy model files to build directory if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/models")
    add_custom_command(TARGET ai_quiz_server POST_BUILD
        COMMENT "Copying model files to build directory"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/models
            ${CMAKE_BINARY_DIR}/bin/models
    )
endif()

# Create installation rules
install(TARGETS ai_quiz_server 
    RUNTIME DESTINATION bin
)

install(TARGETS llama ggml
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(EXISTS "${CMAKE_SOURCE_DIR}/models")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/models/
        DESTINATION bin/models
    )
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Executable will be: ${CMAKE_BINARY_DIR}/bin/ai_quiz_server")
message(STATUS "Libraries will be: ${CMAKE_BINARY_DIR}/lib/")
message(STATUS "Model path: ${CMAKE_BINARY_DIR}/bin/models/")