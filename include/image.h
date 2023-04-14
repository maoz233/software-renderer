/**
 * @file image.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 1.0
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_IMAGE_H_
#define SOFTWARE_RENDERER_INCLUDE_IMAGE_H_

#include <vulkan/vulkan.h>

namespace swr {

class Image {
 public:
  Image() = delete;
  Image(uint32_t width, uint32_t height, VkPhysicalDevice& physical_device,
        VkDevice& device, VkQueue& graphics_queue, VkCommandPool& command_pool,
        const void* data = nullptr);
  ~Image();

  void CreateTextureImage();
  void CreateTextureImageView();
  void CreateTextureSampler();
  void CreateDescriptorSet();

  void SetData(const void* data);
  uint8_t* GetData();
  void Resize(uint32_t width, uint32_t height);

  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  VkDescriptorSet GetDescritorSet() const;

  // begin single time command
  static VkCommandBuffer BeginSingleTimeCommand(
      const VkDevice& device, const VkCommandPool& command_pool);

  // end single time command
  static void EndSingleTimeCommand(const VkDevice& device,
                                   const VkQueue& graphics_queue,
                                   const VkCommandPool& command_pool,
                                   VkCommandBuffer& command_buffer);

  // find memory types
  static uint32_t FindMemoryType(const VkPhysicalDevice& physical_device,
                                 uint32_t type_filter,
                                 VkMemoryPropertyFlags properties);

  // create buffer
  static void CreateBuffer(const VkPhysicalDevice& physical_device,
                           const VkDevice& device, const VkDeviceSize size,
                           const VkBufferUsageFlags usage,
                           const VkMemoryPropertyFlags properties,
                           VkBuffer& buffer, VkDeviceMemory& buffer_memory);

  // image transition
  static void TransitionImageLayout(const VkDevice& device,
                                    const VkQueue& graphics_queue,
                                    const VkCommandPool& command_pool,
                                    VkImage image, VkImageLayout old_layout,
                                    VkImageLayout new_layout);

  // copy buffer to image
  static void CopyBufferToImage(const VkDevice& device,
                                const VkQueue& graphics_queue,
                                const VkCommandPool& command_pool,
                                VkBuffer buffer, VkImage image, uint32_t width,
                                uint32_t height);

  // Vulkan result validator
  static void CheckVulkanResult(const int result, const char* error_msg);

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  VkPhysicalDevice& physical_device_;
  VkDevice& device_;
  VkQueue& graphics_queue_;
  VkCommandPool& command_pool_;

  VkImage texture_image_ = VK_NULL_HANDLE;
  VkBuffer staging_buffer_;
  VkDeviceMemory staging_buffer_memory_;
  VkDeviceMemory texture_image_memory_ = VK_NULL_HANDLE;
  VkImageView texture_image_view_ = VK_NULL_HANDLE;
  VkSampler texture_sampler_ = VK_NULL_HANDLE;

  VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
};

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_IMAGE_H_
