#!/bin/bash

# Ensure Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Homebrew not found. Please install it from https://brew.sh/"
    exit 1
fi

echo "Installing build tools..."
brew install cmake ninja

echo "Installing graphics and math libraries..."
brew install glfw glm

echo "Installing model and data loaders..."
brew install tinyobjloader tinygltf nlohmann-json

echo "Setting up compiler tools..."
xcode-select --install 2>/dev/null || echo "Compiler tools already present."

echo ""
echo "============================================================"
echo "IMPORTANT: Vulkan SDK for macOS must be installed manually."
echo "============================================================"
echo "1. Download the macOS installer from https://vulkan.lunarg.com/sdk/home"
echo "2. Run the installer (.dmg) and follow the prompts."
echo "3. Move the installed SDK folder to a convenient location and rename it accordingly."
echo "   For example: mv ~/VulkanSDK/<version> ~/dev/tools/vulkansdk"
echo "4. Add the setup script to your ~/.zshrc:"
echo "   echo 'source ~/dev/tools/vulkansdk/setup-env.sh' >> ~/.zshrc"
echo ""
echo "After configuring your terminal, you can build the project:"
echo "cmake -B build -S . -G Ninja"
echo "cmake --build build"