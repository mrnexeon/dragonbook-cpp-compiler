# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name
project(compiler)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the source directory
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

# Add executable (main.cpp)
add_executable(${PROJECT_NAME}
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/Inter.h
    ${SOURCE_DIR}/Lexer.h
    ${SOURCE_DIR}/Parser.h
    ${SOURCE_DIR}/Symbols.h
)

# Include the nlohmann json.hpp header
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR}/nlohmann)

# # Optional: Enable warnings (for GCC/Clang)
# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
# endif()
