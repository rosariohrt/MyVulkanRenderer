#pragma once

#include "core/pipeline.h"
#include "core/swap_chain.h"
#include "core/vulkan_device.h"
#include "core/window.h"
#include "scene/model.h"

// std
#include <memory>
#include <vector>

namespace mvr
{

class FirstApp
{
  public:
	static constexpr uint32_t WIDTH  = 800;
	static constexpr uint32_t HEIGHT = 600;

	FirstApp();
	~FirstApp();

	FirstApp(const FirstApp &)            = delete;
	FirstApp &operator=(const FirstApp &) = delete;

	void run();

  private:
	Window                       window{WIDTH, HEIGHT, "MyVulkanRenderer"};
	VulkanDevice                 device{window};
	SwapChain                    swapChain{device, window.getExtent()};
	std::unique_ptr<Pipeline>    pipeline;
	VkPipelineLayout             pipelineLayout;
	std::vector<VkCommandBuffer> commandBuffers;
	std::unique_ptr<Model>       model;

	void loadModel();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffers();
	void drawFrame();
};

}        // namespace mvr