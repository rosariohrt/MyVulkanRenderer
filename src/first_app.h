#pragma once

#include "core/pipeline.h"
#include "core/swap_chain.h"
#include "core/vulkan_device.h"
#include "core/window.h"
#include "scene/model.h"

// std
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
	Window                               window{WIDTH, HEIGHT, "MyVulkanRenderer"};
	VulkanDevice                         device{window};
	SwapChain                            swapChain{device, window.getExtent()};
	std::unique_ptr<Pipeline>            pipeline;
	vk::raii::PipelineLayout             pipelineLayout = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	std::unique_ptr<Model>               model;

	void loadModel();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffers();

	void recordCommandBuffer(uint32_t imageIndex);
	void drawFrame();

	// Helper Methods
	void transitionImageLayout(
	    uint32_t                imageIndex,
	    vk::ImageLayout         oldLayout,
	    vk::ImageLayout         newLayout,
	    vk::AccessFlags2        srcAccessMask,
	    vk::AccessFlags2        dstAccessMask,
	    vk::PipelineStageFlags2 srcStageMask,
	    vk::PipelineStageFlags2 dstStageMask);
};

}        // namespace mvr