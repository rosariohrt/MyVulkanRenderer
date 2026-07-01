#!/bin/bash

echo "Installing dependencies for Vulkan Tutorial..."

# Function to detect the package manager
detect_package_manager() {
    if command -v apt-get &> /dev/null; then
        echo "apt"
    elif command -v dnf &> /dev/null; then
        echo "dnf"
    elif command -v pacman &> /dev/null; then
        echo "pacman"
    else
        echo "unknown"
    fi
}

# Install dependencies based on the package manager
PACKAGE_MANAGER=$(detect_package_manager)

case $PACKAGE_MANAGER in
    apt)
        echo "Detected Ubuntu/Debian-based system"
        echo "Installing build essentials..."
        sudo apt-get update
        sudo apt-get install -y build-essential cmake ninja-build

        echo "Installing GLFW..."
        sudo apt-get install -y libglfw3-dev

        echo "Installing GLM..."
        sudo apt-get install -y libglm-dev

        echo "Installing X Window System dependencies..."
        sudo apt-get install -y libxxf86vm-dev libxi-dev

        echo "Installing clang compiler..."
        sudo apt-get install -y clang
        ;;
    dnf)
        echo "Detected Fedora/RHEL-based system"
        echo "Installing build essentials..."
        sudo dnf install -y gcc-c++ cmake ninja-build

        echo "Installing GLFW..."
        sudo dnf install -y glfw-devel

        echo "Installing GLM..."
        sudo dnf install -y glm-devel

        echo "Installing X Window System dependencies..."
        sudo dnf install -y libXxf86vm-devel libXi-devel

        echo "Installing clang compiler..."
        sudo dnf install -y clang
        ;;
    pacman)
        echo "Detected Arch-based system"
        echo "Installing build essentials..."
        sudo pacman -S --needed base-devel cmake ninja

        echo "Installing GLFW..."
        sudo pacman -S --needed glfw-x11 || sudo pacman -S --needed glfw-wayland

        echo "Installing GLM..."
        sudo pacman -S --needed glm

        echo "Installing clang compiler..."
        sudo pacman -S --needed clang
        ;;
    *)
        echo "Unsupported package manager. Please install the following packages manually:"
        echo "- build-essential or equivalent (gcc, g++, make)"
        echo "- cmake"
        echo "- ninja-build"
        echo "- libglfw3-dev or equivalent"
        echo "- libglm-dev or equivalent"
        echo "- libxxf86vm-dev and libxi-dev or equivalent"
        echo "- clang compiler"
        exit 1
        ;;
esac

# Vulkan SDK installation instructions
echo ""
echo "Now you need to install the Vulkan SDK:"
echo "1. Download the tarball from https://vulkan.lunarg.com/"
echo "2. Extract it to a convenient location, for example:"
echo "   mkdir -p ~/vulkansdk"
echo "   tar -xf vulkansdk-linux-x86_64-<version>.tar.xz -C ~/vulkansdk"
echo "   cd ~/vulkansdk"
echo "   ln -s <version> default"
echo ""
echo "3. Add the following to your ~/.bashrc or ~/.zshrc:"
echo "   source ~/vulkansdk/default/setup-env.sh"
echo ""
echo "4. Restart your terminal or run: source ~/.bashrc"
echo ""
echo "5. Verify installation by running: vkcube"
echo ""

echo "All dependencies have been installed successfully!"
echo "You can now use CMake to build your Vulkan project:"
echo "cmake -B build -S . -G Ninja"
echo "cmake --build build"

exit 0
