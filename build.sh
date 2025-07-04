#!/bin/bash

# Tenstorrent Tensix Emulator Build Script
# Usage: ./build.sh [clean|debug|release|run|demo]

set -e  # Exit on any error

PROJECT_NAME="Tensix Emulator"
BUILD_DIR="build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_banner() {
    echo -e "${BLUE}"
    echo "=========================================================="
    echo "    Tenstorrent Tensix Architecture Emulator"
    echo "              Build Script v1.0"
    echo "=========================================================="
    echo -e "${NC}"
}

print_usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  clean   - Clean build directory"
    echo "  debug   - Build in debug mode"
    echo "  release - Build in release mode (default)"
    echo "  run     - Build and run interactively"
    echo "  demo    - Build and run quick demo"
    echo "  help    - Show this help message"
    echo ""
}

check_dependencies() {
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}Error: CMake is not installed${NC}"
        echo "Please install CMake 3.16 or higher"
        exit 1
    fi
    
    # Check CMake version
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    echo "Found CMake version: $CMAKE_VERSION"
    
    # Check for C++ compiler
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -n1)
        echo "Found compiler: $GCC_VERSION"
    elif command -v clang++ &> /dev/null; then
        CLANG_VERSION=$(clang++ --version | head -n1)
        echo "Found compiler: $CLANG_VERSION"
    else
        echo -e "${RED}Error: No C++ compiler found${NC}"
        echo "Please install GCC or Clang"
        exit 1
    fi
    
    echo -e "${GREEN}All dependencies satisfied${NC}"
    echo ""
}

clean_build() {
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo "Build directory cleaned"
    else
        echo "Build directory does not exist"
    fi
    echo ""
}

create_build_dir() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${YELLOW}Creating build directory...${NC}"
        mkdir -p "$BUILD_DIR"
    fi
}

configure_cmake() {
    local build_type=$1
    echo -e "${YELLOW}Configuring CMake (${build_type})...${NC}"
    
    cd "$BUILD_DIR"
    cmake -DCMAKE_BUILD_TYPE="$build_type" ..
    cd ..
    
    echo -e "${GREEN}CMake configuration completed${NC}"
    echo ""
}

build_project() {
    echo -e "${YELLOW}Building project...${NC}"
    
    cd "$BUILD_DIR"
    
    # Use parallel build if possible
    if command -v nproc &> /dev/null; then
        JOBS=$(nproc)
    else
        JOBS=4
    fi
    
    make -j"$JOBS"
    cd ..
    
    echo -e "${GREEN}Build completed successfully${NC}"
    echo ""
}

run_emulator() {
    local mode=$1
    
    if [ ! -f "$BUILD_DIR/bin/tensix_emulator" ]; then
        echo -e "${RED}Error: Emulator binary not found${NC}"
        echo "Please build the project first"
        exit 1
    fi
    
    echo -e "${YELLOW}Running Tensix Emulator...${NC}"
    echo ""
    
    if [ "$mode" = "demo" ]; then
        # Non-interactive demo mode
        echo "n" | "$BUILD_DIR/bin/tensix_emulator"
    else
        # Interactive mode
        "$BUILD_DIR/bin/tensix_emulator"
    fi
}

print_system_info() {
    echo -e "${BLUE}System Information:${NC}"
    echo "OS: $(uname -s) $(uname -r)"
    echo "Architecture: $(uname -m)"
    echo "CPU cores: $(nproc 2>/dev/null || echo 'unknown')"
    echo "Available memory: $(free -h 2>/dev/null | awk '/^Mem:/ {print $2}' || echo 'unknown')"
    echo ""
}

main() {
    print_banner
    
    # Parse command line arguments
    case "${1:-release}" in
        clean)
            clean_build
            ;;
        debug)
            check_dependencies
            print_system_info
            create_build_dir
            configure_cmake "Debug"
            build_project
            echo -e "${GREEN}Debug build completed${NC}"
            echo "To run: $BUILD_DIR/bin/tensix_emulator"
            ;;
        release)
            check_dependencies
            print_system_info
            create_build_dir
            configure_cmake "Release"
            build_project
            echo -e "${GREEN}Release build completed${NC}"
            echo "To run: $BUILD_DIR/bin/tensix_emulator"
            ;;
        run)
            check_dependencies
            print_system_info
            create_build_dir
            configure_cmake "Release"
            build_project
            run_emulator "interactive"
            ;;
        demo)
            check_dependencies
            print_system_info
            create_build_dir
            configure_cmake "Release"
            build_project
            run_emulator "demo"
            ;;
        help|--help|-h)
            print_usage
            ;;
        *)
            echo -e "${RED}Unknown command: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
}

# Only run main if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi