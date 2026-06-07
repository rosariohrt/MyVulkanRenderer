#pragma once

#include "core/input_manager.h"
#include "core/pipeline.h"
#include "core/swap_chain.h"
#include "core/vulkan_device.h"
#include "core/window.h"
#include "scene/camera.h"
#include "scene/model.h"
#include "scene/texture.h"
#include "ubo.h"

// std
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace mvr
{

class FirstApp
{
  public:
	static constexpr uint32_t WIDTH      = 800;
	static constexpr uint32_t HEIGHT     = 600;
	uint32_t                  frameIndex = 0;

	FirstApp();
	~FirstApp();

	FirstApp(const FirstApp &)            = delete;
	FirstApp &operator=(const FirstApp &) = delete;

	void run();

  private:
	Window       window{WIDTH, HEIGHT, "MyVulkanRenderer"};
	VulkanDevice device{window};
	InputManager input{window};
	Camera camera{{2.0f, 2.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	std::unique_ptr<SwapChain>           swapChain;
	vk::raii::DescriptorSetLayout        descriptorSetLayout = nullptr;
	vk::raii::DescriptorPool             descriptorPool      = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
	vk::raii::PipelineLayout             pipelineLayout = nullptr;
	std::unique_ptr<Pipeline>            pipeline;
	std::unique_ptr<Model>               model;
	std::unique_ptr<Texture>             texture;
	std::vector<vk::raii::CommandBuffer> commandBuffers;

	void processInput(float deltaTime);
	void loadModel();
	void loadTexture();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffers();
	void recreateSwapChain();
	void updateUniformBuffer(uint32_t frameIndex);
	void recordCommandBuffer(uint32_t imageIndex);
	void drawFrame();
};

}        // namespace mvr