/**
 * @file image.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 1.0
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "image.h"

#include <stdexcept>

#define SOFTWARE_RENDERER_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

#define SOFRWARE_RENDERER_INCLUDE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace swr {

Image::Image(uint32_t width, uint32_t height, VkPhysicalDevice& physical_device,
             VkDevice& device, VkQueue& graphics_queue,
             VkCommandPool& command_pool, const void* data)
    : width_{width},
      height_{height},
      physical_device_{physical_device},
      device_{device},
      graphics_queue_{graphics_queue},
      command_pool_{command_pool} {
  // create image
  CreateTextureImage();
  // create image view
  CreateTextureImageView();
  // create sampler
  CreateTextureSampler();
  // create descritor set
  CreateDescriptorSet();

  if (data) {
    SetData(data);
  }
}

Image::~Image() {
  vkDestroySampler(device_, texture_sampler_, nullptr);
  vkDestroyImageView(device_, texture_image_view_, nullptr);
  vkDestroyImage(device_, texture_image_, nullptr);
  vkFreeMemory(device_, texture_image_memory_, nullptr);
}

void Image::CreateTextureImage() {
  // create image
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = VK_FORMAT_R8G8B8A8_UNORM;

  // image extent
  image_info.extent.width = width_;
  image_info.extent.height = height_;
  image_info.extent.depth = 1;

  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkResult result =
      vkCreateImage(device_, &image_info, nullptr, &texture_image_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create image!");

  VkMemoryRequirements mem_requirements{};
  vkGetImageMemoryRequirements(device_, texture_image_, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      FindMemoryType(physical_device_, mem_requirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  result =
      vkAllocateMemory(device_, &alloc_info, nullptr, &texture_image_memory_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to allocate image memory!");

  result = vkBindImageMemory(device_, texture_image_, texture_image_memory_, 0);
  CheckVulkanResult(result, "Error::Vulkan: Failed to bind image memory!");
}

void Image::CreateTextureImageView() {
  // create image view
  VkImageViewCreateInfo image_view_info{};
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.image = texture_image_;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_info.subresourceRange.levelCount = 1;
  image_view_info.subresourceRange.layerCount = 1;

  VkResult result = vkCreateImageView(device_, &image_view_info, nullptr,
                                      &texture_image_view_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create image view");
}

void Image::CreateTextureSampler() {
  // create sampler
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.minLod = -1000;
  sampler_info.maxLod = 1000;
  sampler_info.maxAnisotropy = 1.0f;

  VkResult result =
      vkCreateSampler(device_, &sampler_info, nullptr, &texture_sampler_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create sampler!");
}

void Image::CreateDescriptorSet() {
  descriptor_set_ =
      ImGui_ImplVulkan_AddTexture(texture_sampler_, texture_image_view_,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Image::SetData(const void* data) {
  size_t image_size = width_ * height_ * 4;

  // create staging buffer
  CreateBuffer(physical_device_, device_, image_size,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               staging_buffer_, staging_buffer_memory_);

  VkMemoryRequirements mem_requirements{};
  vkGetBufferMemoryRequirements(device_, staging_buffer_, &mem_requirements);

  // map data to memory
  void* map;
  vkMapMemory(device_, staging_buffer_memory_, 0, image_size, 0, &map);
  memcpy(map, data, static_cast<size_t>(image_size));
  VkMappedMemoryRange range[1]{};
  range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range[0].memory = staging_buffer_memory_;
  range[0].size = mem_requirements.size;
  vkFlushMappedMemoryRanges(device_, 1, range);
  vkUnmapMemory(device_, staging_buffer_memory_);

  TransitionImageLayout(device_, graphics_queue_, command_pool_, texture_image_,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  CopyBufferToImage(device_, graphics_queue_, command_pool_, staging_buffer_,
                    texture_image_, static_cast<uint32_t>(width_),
                    static_cast<uint32_t>(height_));
  TransitionImageLayout(device_, graphics_queue_, command_pool_, texture_image_,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

uint8_t* Image::GetData() {
  size_t image_size = width_ * height_ * 4;

  void* data;
  vkMapMemory(device_, staging_buffer_memory_, 0, image_size, 0, &data);

  return reinterpret_cast<uint8_t*>(data);
}

void Image::Resize(uint32_t width, uint32_t height) {
  if (texture_image_ && width_ == width && height_ == height) {
    return;
  }

  width_ = width;
  height_ = height;

  vkDestroySampler(device_, texture_sampler_, nullptr);
  vkDestroyImageView(device_, texture_image_view_, nullptr);
  vkDestroyImage(device_, texture_image_, nullptr);
  vkFreeMemory(device_, texture_image_memory_, nullptr);
  vkDestroyBuffer(device_, staging_buffer_, nullptr);
  vkFreeMemory(device_, staging_buffer_memory_, nullptr);

  CreateTextureImage();
  CreateTextureImageView();
  CreateTextureSampler();
}

uint32_t Image::GetWidth() const { return width_; }

uint32_t Image::GetHeight() const { return height_; }

VkDescriptorSet Image::GetDescritorSet() const { return descriptor_set_; }

VkCommandBuffer Image::BeginSingleTimeCommand(
    const VkDevice& device, const VkCommandPool& command_pool) {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

void Image::EndSingleTimeCommand(const VkDevice& device,
                                 const VkQueue& graphics_queue,
                                 const VkCommandPool& command_pool,
                                 VkCommandBuffer& command_buffer) {
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

uint32_t Image::FindMemoryType(const VkPhysicalDevice& physical_device,
                               uint32_t type_filter,
                               VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags &
                                   properties) == properties) {
      return i;
    }
  }

  CheckVulkanResult(0, "Error::Vulkan: Failed to find suitable memory type!");

  return 0;
}

void Image::CreateBuffer(const VkPhysicalDevice& physical_device,
                         const VkDevice& device, const VkDeviceSize size,
                         const VkBufferUsageFlags usage,
                         const VkMemoryPropertyFlags properties,
                         VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create buffer!");

  VkMemoryRequirements mem_requirements{};
  vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = FindMemoryType(
      physical_device, mem_requirements.memoryTypeBits, properties);

  result = vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory);
  CheckVulkanResult(result, "Error::Vulkan: Failed to allocate buffer memory!");

  vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void Image::TransitionImageLayout(const VkDevice& device,
                                  const VkQueue& graphics_queue,
                                  const VkCommandPool& command_pool,
                                  VkImage image, VkImageLayout old_layout,
                                  VkImageLayout new_layout) {
  VkCommandBuffer command_buffer = BeginSingleTimeCommand(device, command_pool);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument(
        "Error::Vulkan: Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  EndSingleTimeCommand(device, graphics_queue, command_pool, command_buffer);
}

void Image::CopyBufferToImage(const VkDevice& device,
                              const VkQueue& graphics_queue,
                              const VkCommandPool& command_pool,
                              VkBuffer buffer, VkImage image, uint32_t width,
                              uint32_t height) {
  VkCommandBuffer command_buffer = BeginSingleTimeCommand(device, command_pool);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(command_buffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  EndSingleTimeCommand(device, graphics_queue, command_pool, command_buffer);
}

void Image::CheckVulkanResult(const int result, const char* error_msg) {
  if (VK_SUCCESS != result) {
    throw std::runtime_error(error_msg);
  }
}

}  // namespace swr
