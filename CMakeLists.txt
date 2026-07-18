cmake_minimum_required(VERSION 3.10)
project(ISPPracticeOne VERSION 1.0.0 LANGUAGES CXX)

# Enforce strict C++20 standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSYS2/GCC 16 specific optimization & warning flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# ---------------------------------------------------------------------------
# Third-Party Dependencies (Git Submodules)
# ---------------------------------------------------------------------------

# # 1. Compiled Library Example: GLFW
# # We disable GLFW documentation and test builds to keep our compile fast.
# set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
# set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
# set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
# add_subdirectory(third_party/glfw)

add_library(PocketFFT INTERFACE)
target_include_directories(PocketFFT INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pocketfft)

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)

# ---------------------------------------------------------------------------
# Main Application Target
# ---------------------------------------------------------------------------

add_executable(${PROJECT_NAME} 
    src/main.cpp
)

# Setup include directories for our own code
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# # Link dependencies
# # 'glfw' is the exported target from the glfw subfolder.
# target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Windows specific flag: Hides the console window in the background for GUI apps
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)