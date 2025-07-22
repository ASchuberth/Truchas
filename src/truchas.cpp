#include "truchas.hpp"
#include "config.h"
#include "pch.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace TRUCHAS_APP_NAMESPACE {

void TruchasRender::setup() {

  // GLFW
  createWindow();

  // mInstance
  createInstance();
  createSurface();

  // Physical mDevice
  pickPhysicalDevice();

  // Logical mDevice
  createLogicalDevice();

  // Swapchain
  createSwapChain();
  createImageViews();

  // Graphics Pipelines
  createRenderPass();
  createDescriptorSetLayout();
  createPipelineLayout();
  preparePipelines();

  createCommandPool();
  createDepthResources();
  createFramebuffers();
  createUniformBuffer();
  createDescriptorPool();
  createDescriptorSets();
  allocCommandBuffers();
  createSyncObjects();
}

void TruchasRender::setBGColor(glm::vec4 color) { bgColor = color; }

void TruchasRender::createWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  mMainWindow = glfwCreateWindow(300, 300, "Gila", nullptr, nullptr);

  glfwMaximizeWindow(mMainWindow);
  glfwSetWindowUserPointer(mMainWindow, this);

  glfwSetInputMode(mMainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

vk::Result TruchasRender::createInstance() {

  vkGetInstanceProcAddr =
      dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

  vk::InstanceCreateInfo createInfo;

  createInfo.enabledLayerCount = 0;

  auto glfwExtensionCount = 0u;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  const char *layers = "VK_LAYER_KHRONOS_validation";

  createInfo.enabledLayerCount = 1; // static_cast<uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = &layers;

  createInfo.enabledExtensionCount =
      glfwExtensionCount; // static_cast<uint32_t>(glfwExtensionsVector.size());
  createInfo.ppEnabledExtensionNames =
      glfwExtensions; // glfwExtensionsVector.data();
  createInfo.pApplicationInfo = &mAppInfo;

  vk::Result result = vk::createInstance(&createInfo, nullptr, &mInstance);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance);

  return result;
}

void TruchasRender::createSurface() {

  if (glfwCreateWindowSurface(mInstance, mMainWindow, nullptr,
                              reinterpret_cast<VkSurfaceKHR *>(&mSurface)) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

void TruchasRender::pickPhysicalDevice() {

  std::vector<vk::PhysicalDevice> devices =
      mInstance.enumeratePhysicalDevices();

  if (devices.empty()) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      mPhysicalDevice = device;
      break;
    }
  }

  if (&mPhysicalDevice == VK_NULL_HANDLE) {
    std::cout << "Failed!" << std::endl;
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

SwapChainSupportDetails
TruchasRender::querySwapChainSupport(const vk::PhysicalDevice &device) {

  SwapChainSupportDetails details;

  details.capabilities = device.getSurfaceCapabilitiesKHR(mSurface);
  details.formats = device.getSurfaceFormatsKHR(mSurface);
  details.presentModes = device.getSurfacePresentModesKHR(mSurface);

  return details;
}

QueueFamilyIndices TruchasRender::findQueueFamilies(vk::PhysicalDevice device,
                                                    VkSurfaceKHR surface) {
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies =
      device.getQueueFamilyProperties();

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    presentSupport = device.getSurfaceSupportKHR(i, surface);

    if (queueFamily.queueCount > 0 && presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool TruchasRender::isDeviceSuitable(vk::PhysicalDevice device) {

  mIndices = findQueueFamilies(device, mSurface);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  return mIndices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool TruchasRender::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
  std::vector<vk::ExtensionProperties> availableExtensions =
      device.enumerateDeviceExtensionProperties(nullptr);

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

void TruchasRender::createLogicalDevice() {

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<int> uniqueQueueFamilies = {mIndices.graphicsFamily,
                                       mIndices.presentFamily};

  float queuePriority = 1.0f;
  for (int queueFamily : uniqueQueueFamilies) {

    vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1,
                                              &queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceFeatures deviceFeatures;

  deviceFeatures.fillModeNonSolid = true;
  deviceFeatures.depthBounds = true;
  deviceFeatures.wideLines = true;
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  vk::DeviceCreateInfo createInfo(
      {}, static_cast<uint32_t>(queueCreateInfos.size()),
      queueCreateInfos.data(), {}, {},
      static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(),
      &deviceFeatures);

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  mDevice = mPhysicalDevice.createDevice(createInfo, nullptr);

  mGraphicsQueue = mDevice.getQueue(mIndices.graphicsFamily, 0);
  mPresentQueue = mDevice.getQueue(mIndices.presentFamily, 0);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice);
  vkGetDeviceProcAddr =
      dl.getProcAddress<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
}

vk::SurfaceFormatKHR TruchasRender::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) {

  if (availableFormats.size() == 1 &&
      availableFormats[0].format == vk::Format::eUndefined) {
    return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
  }

  for (const auto &availableFormat : availableFormats) {

    if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {

      return availableFormat;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR TruchasRender::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> availablePresentModes) {
  vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    } else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
      bestMode = availablePresentMode;
    }
  }

  return bestMode;
}

vk::Extent2D TruchasRender::chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(mMainWindow, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));

    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

void TruchasRender::createSwapChain() {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(mPhysicalDevice);

  vk::SurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {

    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo(
      {}, mSurface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
      extent, 1, vk::ImageUsageFlagBits::eColorAttachment);

  uint32_t queueFamilyIndices[] = {
      static_cast<uint32_t>(mIndices.graphicsFamily),
      static_cast<uint32_t>(mIndices.presentFamily)};

  if (mIndices.isDifferent()) {

    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  mSwapchain = mDevice.createSwapchainKHR(createInfo, nullptr);

  mImages = mDevice.getSwapchainImagesKHR(mSwapchain);

  mFormat = surfaceFormat.format;
  mExtent = extent;
}

void TruchasRender::recreateSwapchain() {

  int width = 0, height = 0;
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(mMainWindow, &width, &height);
    glfwWaitEvents();
  }

  mDevice.waitIdle();

  cleanupSwapchain();

  createSwapChain();
  createImageViews();

  createRenderPass();
  createDescriptorSetLayout();
  createPipelineLayout();
  createDepthResources();
  createFramebuffers();

  preparePipelines();
}

void TruchasRender::createImageViews() {
  mImageViews.resize(mImages.size());

  for (decltype(mImages.size()) i = 0; i < mImages.size(); i++) {
    mImageViews[i] =
        createImageView(mImages[i], mFormat, vk::ImageAspectFlagBits::eColor);
  }
}

vk::ImageView TruchasRender::createImageView(vk::Image image, vk::Format format,
                                             vk::ImageAspectFlags aspectFlags) {
  vk::ImageViewCreateInfo createInfo = {};
  createInfo.image = image;
  createInfo.viewType = vk::ImageViewType::e2D; // VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = format;
  createInfo.components.r =
      vk::ComponentSwizzle::eIdentity; // VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = vk::ComponentSwizzle::eIdentity;
  createInfo.components.b = vk::ComponentSwizzle::eIdentity;
  createInfo.components.a = vk::ComponentSwizzle::eIdentity;
  createInfo.subresourceRange.aspectMask = aspectFlags;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  vk::ImageView imageView;

  vk::Result result = mDevice.createImageView(&createInfo, nullptr, &imageView);

  return imageView;
}

vk::Format
TruchasRender::findSupportedFormat(vk::PhysicalDevice const &PhysicalDevice,
                                   const std::vector<vk::Format> &candidates,
                                   vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features) {

  for (vk::Format format : candidates) {
    vk::FormatProperties props;

    props = PhysicalDevice.getFormatProperties(format);

    if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("Failed to find supported format!");
}

vk::Format
TruchasRender::findDepthFormat(vk::PhysicalDevice const &PhysicalDevice) {

  return findSupportedFormat(
      PhysicalDevice,
      {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
       vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void TruchasRender::createRenderPass() {

  // Color Attachment
  vk::AttachmentDescription colorAttachment = {};
  colorAttachment.format = mFormat;
  colorAttachment.samples = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  // Depth Buffer Attachment
  vk::AttachmentDescription depthAttachment = {};
  depthAttachment.format = findDepthFormat(mPhysicalDevice);
  depthAttachment.samples = vk::SampleCountFlagBits::e1;
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::SubpassDescription subpass = {};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  vk::SubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                             vk::AccessFlagBits::eColorAttachmentWrite;

  std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment,
                                                          depthAttachment};
  vk::RenderPassCreateInfo renderPassInfo = {};

  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (mDevice.createRenderPass(&renderPassInfo, nullptr, &mRenderPass) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create render pass!");
}

void TruchasRender::createDescriptorSetLayout() {

  vk::DescriptorSetLayoutBinding uboLayoutBinding(
      0, vk::DescriptorType::eUniformBuffer, 1,
      vk::ShaderStageFlagBits::eVertex, nullptr);

  std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

  vk::DescriptorSetLayoutCreateInfo layoutInfo({}, 1, &uboLayoutBinding);

  if (mDevice.createDescriptorSetLayout(&layoutInfo, nullptr,
                                        &this->mDescriptorSetLayout) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create descriptor set layout!");
}

void TruchasRender::createPipelineLayout() {

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 1, &mDescriptorSetLayout,
                                                  0, nullptr);

  if (mDevice.createPipelineLayout(&pipelineLayoutInfo, nullptr,
                                   &mPipelineLayout) != vk::Result::eSuccess)
    throw std::runtime_error("failed to create pipeline layout");
}

std::vector<char> TruchasRender::readFile(const std::string filename) {

  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

vk::ShaderModule
TruchasRender::createShaderModule(const std::vector<char> &code) {
  vk::ShaderModuleCreateInfo createInfo = {};

  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  vk::ShaderModule shaderModule;

  if (mDevice.createShaderModule(&createInfo, nullptr, &shaderModule) !=
      vk::Result::eSuccess) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

void TruchasRender::createCommandPool() {

  vk::CommandPoolCreateInfo commandPoolInfo(
      vk::CommandPoolCreateFlagBits::eTransient, mIndices.graphicsFamily);

  vk::Result result =
      mDevice.createCommandPool(&commandPoolInfo, nullptr, &mCommandPool);
}

uint32_t TruchasRender::findMemoryType(uint32_t typeFilter,
                                       vk::MemoryPropertyFlags properties) {

  vk::PhysicalDeviceMemoryProperties memProperties;
  memProperties = mPhysicalDevice.getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void TruchasRender::createImage(
    vk::PhysicalDevice const &PhysicalDevice, vk::Device const &Device,
    uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
    vk::Image &image, vk::DeviceMemory &imageMemory) {

  vk::ImageCreateInfo imageInfo(
      {}, vk::ImageType::e2D, format, {width, height, 1}, 1, 1,
      vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive);

  image = Device.createImage(imageInfo, nullptr);

  vk::MemoryRequirements memRequirements;
  memRequirements = Device.getImageMemoryRequirements(image);

  vk::MemoryAllocateInfo allocInfo(
      memRequirements.size,
      findMemoryType(memRequirements.memoryTypeBits, properties));
  imageMemory = Device.allocateMemory(allocInfo, nullptr);

  Device.bindImageMemory(image, imageMemory, 0);
}

void TruchasRender::createDepthResources() {
  vk::Format depthFormat = findDepthFormat(mPhysicalDevice);

  createImage(mPhysicalDevice, mDevice, mExtent.width, mExtent.height,
              depthFormat, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eDepthStencilAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage,
              depthImageMemory);

  depthImageView =
      createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

void TruchasRender::createBuffer(vk::DeviceSize &size,
                                 const vk::BufferUsageFlags &usage,
                                 const vk::MemoryPropertyFlags &properties,
                                 vk::Buffer &buffer,
                                 vk::DeviceMemory &bufferMemory) {

  vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);

  buffer = mDevice.createBuffer(bufferInfo, nullptr);

  vk::MemoryRequirements memRequirements;
  mDevice.getBufferMemoryRequirements(buffer, &memRequirements);

  vk::MemoryAllocateInfo allocInfo(
      memRequirements.size,
      findMemoryType(memRequirements.memoryTypeBits, properties));

  bufferMemory = mDevice.allocateMemory(allocInfo);
  mDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

vk::CommandBuffer TruchasRender::beginSingleTimeCommands(
    const vk::CommandBufferLevel &level,
    const vk::CommandBufferInheritanceInfo &inheritance) {

  // 3rd parameter is for Comm. Buffer Count
  vk::CommandBufferAllocateInfo allocInfo(mCommandPool, level, 1);

  std::vector<vk::CommandBuffer> commandBuffers =
      mDevice.allocateCommandBuffers(allocInfo);

  vk::CommandBufferBeginInfo beginInfo(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  if (level == vk::CommandBufferLevel::eSecondary) {
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
    beginInfo.pInheritanceInfo = &inheritance;
  }

  if (!commandBuffers.empty()) {
    commandBuffers[0].begin(beginInfo);
  } else {
    throw std::runtime_error("No Command Buffers in beginSingleTimeCommands()");
  }

  return commandBuffers[0];
}

void TruchasRender::endSingleTimeCommands(vk::CommandBuffer &commandBuffer) {

  commandBuffer.end();

  vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);

  mGraphicsQueue.submit(submitInfo, nullptr);
  mGraphicsQueue.waitIdle();
  mDevice.freeCommandBuffers(mCommandPool, commandBuffer);
}

void TruchasRender::transitionImageLayout(vk::Image &image, vk::Format format,
                                          vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout) {

  vk::CommandBuffer commandBuffer = beginSingleTimeCommands(
      vk::CommandBufferLevel::ePrimary, vk::CommandBufferInheritanceInfo());

  vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

  vk::ImageMemoryBarrier barrier({}, {}, oldLayout, newLayout,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 VK_QUEUE_FAMILY_IGNORED, image, range);

  vk::PipelineStageFlags sourceStage;
  vk::PipelineStageFlags destinationStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, 0, nullptr,
                                0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void TruchasRender::copyBufferToImage(vk::Buffer &buffer, vk::Image &image,
                                      uint32_t width, uint32_t height) {

  vk::CommandBuffer commandBuffer = beginSingleTimeCommands(
      vk::CommandBufferLevel::ePrimary, vk::CommandBufferInheritanceInfo());

  vk::ImageSubresourceLayers layers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);

  vk::BufferImageCopy region(0, 0, 0, layers, {0, 0, 0}, {width, height, 1});

  commandBuffer.copyBufferToImage(
      buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

void TruchasRender::createFramebuffers() {

  mFramebuffers.resize(mImageViews.size());

  for (decltype(mImageViews.size()) i = 0; i < mImageViews.size(); i++) {

    std::array<vk::ImageView, 2> attachments = {mImageViews[i], depthImageView};

    vk::FramebufferCreateInfo FramebufferInfo(
        {}, mRenderPass, static_cast<uint32_t>(attachments.size()),
        attachments.data(), mExtent.width, mExtent.height, 1);

    vk::Result result =
        mDevice.createFramebuffer(&FramebufferInfo, nullptr, &mFramebuffers[i]);
  }
}

void TruchasRender::createUniformBuffer() {

  vk::DeviceSize bufferSize = sizeof(ubo);

  mUniformBuffers.resize(mImages.size());
  mUniformBufferMemories.resize(mImages.size());

  for (size_t i = 0; i < mImages.size(); i++) {
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 mUniformBuffers[i], mUniformBufferMemories[i]);
  }
}

void TruchasRender::createDescriptorPool() {

  std::array<vk::DescriptorPoolSize, 1> poolSizes = {};
  poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(mImages.size());

  vk::DescriptorPoolCreateInfo poolInfo(
      {}, static_cast<uint32_t>(mImages.size()),
      static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

  mDescriptorPool = mDevice.createDescriptorPool(poolInfo, nullptr);
}

void TruchasRender::createDescriptorSets() {

  std::vector<vk::DescriptorSetLayout> layouts(mImages.size(),
                                               mDescriptorSetLayout);

  vk::DescriptorSetAllocateInfo allocInfo(
      mDescriptorPool, static_cast<uint32_t>(mImages.size()), layouts.data());

  mDescriptorSets.resize(mImages.size());

  mDescriptorSets = mDevice.allocateDescriptorSets(allocInfo);

  // Uniform Buffer

  for (size_t i = 0; i < mImages.size(); i++) {

    vk::DescriptorBufferInfo bufferInfo(mUniformBuffers[i], 0, sizeof(u));

    vk::WriteDescriptorSet descriptorWrites;

    descriptorWrites.dstSet = mDescriptorSets[i];
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = &bufferInfo;

    mDevice.updateDescriptorSets(1, &descriptorWrites, 0, nullptr);
  }
}

void TruchasRender::allocCommandBuffers() {

  mCommandBuffers.resize(mFramebuffers.size());

  vk::CommandBufferAllocateInfo allocInfo(mCommandPool,
                                          vk::CommandBufferLevel::ePrimary,
                                          (uint32_t)mCommandBuffers.size());

  mCommandBuffers = mDevice.allocateCommandBuffers(allocInfo);
}

void TruchasRender::createSyncObjects() {

  mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    mImageAvailableSemaphores[i] = mDevice.createSemaphore(semaphoreInfo);
    mRenderFinishedSemaphores[i] = mDevice.createSemaphore(semaphoreInfo);
    mInFlightFences[i] = mDevice.createFence(fenceInfo, nullptr);
  }
}

void TruchasRender::initImgui() {

  // Create Descriptor Pool

  vk::DescriptorPoolSize pool_sizes[] = {

      {vk::DescriptorType::eCombinedImageSampler, 1000},

  };

  vk::DescriptorPoolCreateInfo poolInfo(
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      1000 * IM_ARRAYSIZE(pool_sizes), (uint32_t)IM_ARRAYSIZE(pool_sizes),
      pool_sizes);

  mGuiDescriptorPool = mDevice.createDescriptorPool(poolInfo);

  // Create Window Surface

  VkResult err =
      glfwCreateWindowSurface(mInstance, mMainWindow, nullptr, &mImguiSurface);

  int w, h;
  glfwGetFramebufferSize(mMainWindow, &w, &h);
  ImGui_ImplVulkanH_Window *wd = &mImguiWindowData;

  // Setup Dear ImGui binding
  // IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.WantCaptureMouse = true;

  // Setup GLFW binding
  ImGui_ImplGlfw_InitForVulkan(mMainWindow, true);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = mInstance;
  init_info.PhysicalDevice = mPhysicalDevice;
  init_info.Device = mDevice;
  init_info.QueueFamily = mIndices.graphicsFamily;
  init_info.Queue = mGraphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = mGuiDescriptorPool;
  init_info.Subpass = 0;
  init_info.MinImageCount = static_cast<uint32_t>(mImageViews.size());
  init_info.ImageCount = static_cast<uint32_t>(mImageViews.size());
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = VK_NULL_HANDLE;
  init_info.CheckVkResultFn = VK_NULL_HANDLE;

  ImGui_ImplVulkan_Init(&init_info, mRenderPass);

  // Setup style
  ImGui::StyleColorsDark();

  // Upload Fonts
  {
    // Use any command queue

    vk::CommandBuffer command_buffer =
        beginSingleTimeCommands(vk::CommandBufferLevel::ePrimary, {});

    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    endSingleTimeCommands(command_buffer);
  }
}

void TruchasRender::createSketchPointPipeline() {

  vk::VertexInputBindingDescription BindingDescription(
      0, sizeof(Vertex), vk::VertexInputRate::eVertex);

  std::vector<vk::VertexInputBindingDescription> BindingDescriptions = {
      BindingDescription};

  std::vector<vk::VertexInputAttributeDescription> AttributeDescriptions(2);

  AttributeDescriptions[0].binding = 0;
  AttributeDescriptions[0].location = 0;
  AttributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
  AttributeDescriptions[0].offset = offsetof(Vertex, pos);

  AttributeDescriptions[1].binding = 0;
  AttributeDescriptions[1].location = 1;
  AttributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
  AttributeDescriptions[1].offset = offsetof(Vertex, col);

  vk::PipelineVertexInputStateCreateInfo VertexInputInfo(
      {}, static_cast<uint32_t>(BindingDescriptions.size()),
      BindingDescriptions.data(),
      static_cast<uint32_t>(AttributeDescriptions.size()),
      AttributeDescriptions.data());

  auto vertShaderCode = readFile(config::vert_shader_file_path);
  auto fragShaderCode = readFile(config::frag_shader_file_path);

  vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  vk::PipelineShaderStageCreateInfo VertShaderInfo(
      {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");
  vk::PipelineShaderStageCreateInfo FragShaderInfo(
      {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");

  vk::PipelineShaderStageCreateInfo ShaderStages[] = {VertShaderInfo,
                                                      FragShaderInfo};

  vk::PipelineInputAssemblyStateCreateInfo InputAssemblyInfo(
      {}, vk::PrimitiveTopology::ePointList, VK_FALSE);

  vk::PipelineRasterizationStateCreateInfo RasterizerInfo(
      {}, VK_FALSE, VK_FALSE, vk::PolygonMode::ePoint,
      vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, VK_FALSE);

  vk::Viewport ViewPort(0.0f, 0.0f, static_cast<float>(mExtent.width),
                        static_cast<float>(mExtent.height), 0.0f, 1.0f);

  vk::Rect2D Scissor({0, 0}, mExtent);

  vk::PipelineViewportStateCreateInfo ViewportInfo({}, 1, &ViewPort, 1,
                                                   &Scissor);

  vk::PipelineMultisampleStateCreateInfo MultisampleInfo(
      {}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE,
      VK_FALSE);

  vk::PipelineColorBlendAttachmentState ColorBlendAttachment(
      VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha,
      vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

  vk::PipelineColorBlendStateCreateInfo ColorBlendingInfo(
      {}, VK_FALSE, vk::LogicOp::eCopy, 1, &ColorBlendAttachment,
      {0.0f, 0.0f, 0.0f, 0.0f});

  vk::PipelineDepthStencilStateCreateInfo depthStencilInfo(
      {}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_TRUE, VK_FALSE, {}, {},
      0.0f, 1.0f);

  vk::GraphicsPipelineCreateInfo PipelineCreateInfo;

  PipelineCreateInfo.stageCount = 2;
  PipelineCreateInfo.pStages = ShaderStages;
  PipelineCreateInfo.pVertexInputState = &VertexInputInfo;
  PipelineCreateInfo.pInputAssemblyState = &InputAssemblyInfo;
  PipelineCreateInfo.pViewportState = &ViewportInfo;
  PipelineCreateInfo.pRasterizationState = &RasterizerInfo;
  PipelineCreateInfo.pMultisampleState = &MultisampleInfo;
  PipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
  PipelineCreateInfo.pColorBlendState = &ColorBlendingInfo;

  PipelineCreateInfo.renderPass = mRenderPass;
  PipelineCreateInfo.subpass = 0;

  PipelineCreateInfo.basePipelineIndex = -1;
  PipelineCreateInfo.layout = mPipelineLayout;

  Pipelines.SketchPoint =
      mDevice
          .createGraphicsPipeline(mPipelineCache, PipelineCreateInfo, nullptr)
          .value;

  mDevice.destroyShaderModule(vertShaderModule, nullptr);
  mDevice.destroyShaderModule(fragShaderModule, nullptr);
}

vk::Pipeline TruchasRender::getSketchPointPipeline() {
  return Pipelines.SketchPoint;
}

void TruchasRender::preparePipelines() { createSketchPointPipeline(); }

void TruchasRender::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                               vk::DeviceSize size) {

  vk::CommandBuffer commandBuffer = beginSingleTimeCommands(
      vk::CommandBufferLevel::ePrimary, vk::CommandBufferInheritanceInfo());

  vk::BufferCopy copyRegion;
  copyRegion.size = size;

  commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void TruchasRender::deleteBuffer(uint32_t id) {

  // mBuffers[id].isEmpty = true;

  std::map<uint32_t, Buffer>::iterator erase_iter = mBuffers.find(id);

  if (erase_iter != mBuffers.end()) {
    mDevice.destroyBuffer(mBuffers[id].mBuffer);
    mDevice.freeMemory(mBuffers[id].mMemory);
    mBuffers.erase(erase_iter);
  }
}

void TruchasRender::createCommandBuffers() {

  mDevice.resetCommandPool(mCommandPool, vk::CommandPoolResetFlags());

  for (size_t i = 0; i < mCommandBuffers.size(); i++) {

    vk::CommandBufferBeginInfo beginInfo(
        vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    mCommandBuffers[i].begin(beginInfo);

    vk::Rect2D renderArea({0, 0}, {mExtent.width, mExtent.height});

    std::array<float, 4> color = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].setColor(color);
    clearValues[1].depthStencil.depth = 1.0f;
    clearValues[1].depthStencil.stencil = 0;

    vk::RenderPassBeginInfo renderPassInfo(
        mRenderPass, mFramebuffers[i], renderArea,
        static_cast<uint32_t>(clearValues.size()), clearValues.data());

    mCommandBuffers[i].beginRenderPass(renderPassInfo,
                                       vk::SubpassContents::eInline);

    vk::DeviceSize offsets[] = {0};

    mCommandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          mPipelineLayout, 0, 1,
                                          &mDescriptorSets[i], 0, nullptr);

    for (const auto &buffer : mBuffers) {

      // if (buffer.second.isEmpty)
      //   continue;

      mCommandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics,
                                      Pipelines.SketchPoint);
      mCommandBuffers[i].bindVertexBuffers(0, 1, &buffer.second.mBuffer,
                                           offsets);
      mCommandBuffers[i].draw(buffer.second.mPointSize, 1, 0, 0);
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mCommandBuffers[i]);

    mCommandBuffers[i].endRenderPass();

    mCommandBuffers[i].end();
  }
}

void TruchasRender::updateUniformBuffer(uint32_t currentImage) {

  glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
  u.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), up);

  u.view = glm::lookAt(glm::vec3(0.0f, -10.0f, 0.0f),
                       glm::vec3(0.0f, 0.0f, 0.0f), up);

  u.proj =
      glm::perspective(glm::radians(45.0f),
                       mExtent.width / (float)mExtent.height, 0.001f, 100.0f);

  u.proj[1][1] *= -1;

  vk::MemoryMapFlags memMapFlags;

  void *data;
  data = mDevice.mapMemory(mUniformBufferMemories[currentImage], 0, sizeof(u),
                           memMapFlags);
  memcpy(data, &u, sizeof(u));
  mDevice.unmapMemory(mUniformBufferMemories[currentImage]);
}

void TruchasRender::drawFrame() {

  vk::Result result1 = mDevice.waitForFences(mInFlightFences[mCurrentFrame],
                                             VK_TRUE, UINT64_MAX);

  uint32_t imageIndex = 0;

  vk::Fence F;

  vk::Result result = mDevice.acquireNextImageKHR(
      mSwapchain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], F,
      &imageIndex);

  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapchain();
    return;
  } else if (result != vk::Result::eSuccess &&
             result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  updateUniformBuffer(imageIndex);

  vk::Semaphore waitSemaphore[] = {mImageAvailableSemaphores[mCurrentFrame]};
  vk::Semaphore signalSemaphore[] = {mRenderFinishedSemaphores[mCurrentFrame]};

  vk::PipelineStageFlags waitStages =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubmitInfo submitInfo(1, waitSemaphore, &waitStages, 1,
                            &mCommandBuffers[imageIndex], 1, signalSemaphore);

  mDevice.resetFences(mInFlightFences[mCurrentFrame]);

  mGraphicsQueue.submit(submitInfo, mInFlightFences[mCurrentFrame]);

  vk::PresentInfoKHR presentInfo(1, signalSemaphore, 1, &mSwapchain,
                                 &imageIndex);

  result = mPresentQueue.presentKHR(presentInfo);

  if (result == vk::Result::eErrorOutOfDateKHR ||
      result == vk::Result::eSuboptimalKHR || frameBufferResized ||
      result == vk::Result::eErrorIncompatibleDisplayKHR) {
    frameBufferResized = false;
    recreateSwapchain();
  } else if (result != vk::Result::eSuccess) {
    // std::cout << "Present Queue Failed\n";
    throw std::runtime_error("failed to present swap chain image!");
  }

  mPresentQueue.waitIdle();

  mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void TruchasRender::destroyPipelines() {

  mDevice.destroyPipeline(Pipelines.SketchPoint);
  mDevice.destroyPipeline(Pipelines.SketchLine);
  mDevice.destroyPipeline(Pipelines.SketchGrid);
  mDevice.destroyPipeline(mTextPipeline);
}

void TruchasRender::cleanupSwapchain() {

  for (auto &mFramebuffer : mFramebuffers)
    mDevice.destroyFramebuffer(mFramebuffer, nullptr);

  mDevice.destroyPipelineLayout(mPipelineLayout, nullptr);

  mDevice.destroyDescriptorSetLayout(mDescriptorSetLayout, nullptr);

  mDevice.destroyRenderPass(mRenderPass, nullptr);

  destroyPipelines();

  mDevice.destroyImage(depthImage);
  mDevice.freeMemory(depthImageMemory);
  mDevice.destroyImageView(depthImageView);

  for (auto &view : mImageViews)
    mDevice.destroyImageView(view, nullptr);

  mDevice.destroySwapchainKHR(mSwapchain, nullptr);
}

void TruchasRender::cleanup() {

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    mDevice.destroySemaphore(mImageAvailableSemaphores[i]);
    mDevice.destroySemaphore(mRenderFinishedSemaphores[i]);
    mDevice.destroyFence(mInFlightFences[i]);
  }

  destroyPipelines();

  for (auto &Buffer : mBuffers) {

    mDevice.destroyBuffer(Buffer.second.mBuffer);
    mDevice.freeMemory(Buffer.second.mMemory);
  }

  for (auto &framebuffer : mFramebuffers) {
    mDevice.destroyFramebuffer(framebuffer, nullptr);
  }

  for (auto &Buffer : mUniformBuffers) {
    mDevice.destroyBuffer(Buffer);
  }

  for (auto &memory : mUniformBufferMemories) {
    mDevice.freeMemory(memory);
  }

  mDevice.destroySampler(mTextureSampler);
  mDevice.destroyImageView(mTextureImageView);
  mDevice.destroyImage(mTextureImage);
  mDevice.freeMemory(mTextureMemory);

  mDevice.destroyImage(depthImage);
  mDevice.freeMemory(depthImageMemory);
  mDevice.destroyImageView(depthImageView);

  mDevice.destroyCommandPool(mCommandPool);
  mDevice.destroyDescriptorPool(mGuiDescriptorPool);
  mDevice.destroyDescriptorPool(mDescriptorPool);
  mDevice.destroyPipelineLayout(mPipelineLayout, nullptr);
  mDevice.destroyDescriptorSetLayout(mDescriptorSetLayout, nullptr);
  mDevice.destroy(mRenderPass, nullptr);

  for (auto &imageView : mImageViews) {
    mDevice.destroyImageView(imageView, nullptr);
  }

  mDevice.destroySwapchainKHR(mSwapchain, nullptr);

  vkDestroyDevice(mDevice, nullptr);
  vkDestroySurfaceKHR(mInstance, mImguiSurface, nullptr);
  vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
  vkDestroyInstance(mInstance, nullptr);

  glfwTerminate();
}

void TruchasRender::destroy() {

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  cleanup();
}

void TruchasRender::onNotify(int id, const RenderData &Renderables) {

  std::cout << "Render notified!" << std::endl;
  std::cout << Renderables << std::endl;

  std::vector<Vertex> Vertices;

  // if (!Renderables.Points.empty()) {

  //   for (const auto& p : Renderables.Points) {
  //     Vertices.push_back( { p, glm::vec3 {1.0f, 1.0f, 1.0f} } );
  //   }
  // }

  // if (Vertices.empty()) {
  //   deleteBuffer(id);
  //   return;
  // }

  // if (mBuffers.find(id) == mBuffers.end()) {
  //   createDeviceBuffer(id, Vertices, vk::BufferUsageFlagBits::eVertexBuffer);
  // }
  // else {
  //   updateDeviceBuffer(id, Vertices, vk::BufferUsageFlagBits::eVertexBuffer);
  // }
}

} // namespace TRUCHAS_APP_NAMESPACE
