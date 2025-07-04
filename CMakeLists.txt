cmake_minimum_required(VERSION 3.16)
project(TensixEmulator VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Create the main executable
add_executable(tensix_emulator 
    main.cpp
    tensix_emulator.cpp
    llm_model.cpp
)

# Create the enhanced MatMul analysis executable
add_executable(enhanced_matmul_analysis 
    enhanced_matmul_analysis.cpp
    tensix_emulator.cpp
    llm_model.cpp
)

# Create the enhanced MatMul main demo
add_executable(enhanced_matmul_main 
    enhanced_matmul_main.cpp
    tensix_emulator.cpp
    llm_model.cpp
)

# Performance analysis tool
add_executable(performance_profiler 
    performance_profiler.cpp
    tensix_emulator.cpp
)

# LLaMA-7B performance analysis
add_executable(llama7b_performance_analysis 
    llama7b_performance_analysis.cpp
    tensix_emulator.cpp
    llm_model.cpp
)

# Link libraries
target_link_libraries(tensix_emulator PRIVATE Threads::Threads)
target_link_libraries(enhanced_matmul_analysis PRIVATE Threads::Threads)
target_link_libraries(enhanced_matmul_main PRIVATE Threads::Threads)
target_link_libraries(performance_profiler PRIVATE Threads::Threads)
target_link_libraries(llama7b_performance_analysis PRIVATE Threads::Threads)

# Set output directory
set_target_properties(tensix_emulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
set_target_properties(enhanced_matmul_analysis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
set_target_properties(enhanced_matmul_main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
set_target_properties(performance_profiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
set_target_properties(llama7b_performance_analysis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Add custom target for running the emulator
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/bin/tensix_emulator
    DEPENDS tensix_emulator
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Tensix Emulator"
)

# Add custom target for quick demo (non-interactive)
add_custom_target(demo
    COMMAND echo "n" | ${CMAKE_BINARY_DIR}/bin/tensix_emulator
    DEPENDS tensix_emulator
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Tensix Emulator Demo (quick)"
)

# Installation
install(TARGETS tensix_emulator
    RUNTIME DESTINATION bin
)

# Print configuration information
message(STATUS "")
message(STATUS "Tenstorrent Tensix Emulator Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  make tensix_emulator - Build the emulator")
message(STATUS "  make run             - Build and run interactively")
message(STATUS "  make demo            - Build and run quick demo")
message(STATUS "  make install         - Install to system")
message(STATUS "")
