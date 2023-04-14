/**
 * @file application.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 1.0
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_APPLICATION_H_
#define SOFTWARE_RENDERER_INCLUDE_APPLICATION_H_

#include <cstdint>
#include <optional>
#include <vector>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "layer.h"

namespace swr {

#ifdef _DEBUG
#define ENABLE_VULKAN_VALIDATION_LAYER
#endif

const int WIDTH = 1500;
const int HEIGHT = 762;
const char* const TITLE = "Software Renderer";

#ifdef _WIN32
const char* const FONTS_FILEPATH{"../../fonts/Roboto-Medium.ttf"};
#else
const char* const FONTS_FILEPATH{"../fonts/Roboto-Medium.ttf"};
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilies {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  inline bool IsCompleted();
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;

  inline bool IsAdequate();
};

class Application {
 public:
  Application();
  ~Application();

  void Run();

  // create GLFW window
  void CreateWindow();
  // create Vulkan instance
  void CreateInstance();
  // setup Vulkan debug messenger
  void SetupDebugMessenger();
  // create window surface
  void CreateWindowSurface();
  // pick physical device
  void PickPhysicalDevice();
  // create logical device
  void CreateLogicalDevice();
  // create swap chain
  void CreateSwapChain();
  // create image view
  void CreateImageViews();
  // create render pass
  void CreateRenderPass();
  // create descritor pool
  void CreateDescriptorPool();
  // create frame buffers
  void CreateFramebuffers();
  // create command pool
  void CreateCommandPool();
  // create command buffers
  void CreateCommandBuffers();
  // create fences and semaphores
  void CreateSyncObjects();
  // setup ImGui
  void SetupImGui();

  // draw frame
  void DrawFrame();

  // recreate swapchain
  void RecreateSwapChain();
  // clean swap chain
  void CleanupSwapChain();
  // record command buffer
  void RecordCommandBuffer(VkCommandBuffer command_buffer,
                           uint32_t image_index);

  // set framebuffer resized
  void SetFramebufferResized(bool resized);

  // GLFW error callback
  static void GLFWErrorCallback(const int code, const char* description);

  // Vulkan result validator
  static void CheckVulkanResult(const int result, const char* error_msg);

  // Vulkan instance required extensions
  static std::vector<const char*> QueryVulkanInstanceExts();

  // Vulkan instance required layers
  static std::vector<const char*> QueryVulkanLayers();

  // Vulakn debug messenger callback
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  DebugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                void* user_data);

  // Vulkan debug messenger create and destroy
  static VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT* create_info,
      const VkAllocationCallbacks* allocator,
      VkDebugUtilsMessengerEXT* debug_messenger);
  static void DestroyDebugUtilsMessengerEXT(
      VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
      const VkAllocationCallbacks* allocator);

  // evaluate GPU
  static int EvaluatePhysicalDevice(const VkPhysicalDevice& physical_device,
                                    const VkSurfaceKHR& surface);

  // queue faimilies
  static QueueFamilies QueryQueueFamilies(
      const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface);

  //  device required extensions
  static std::vector<const char*> QueryVulkanDeviceExts(
      const VkPhysicalDevice& physical_device);

  // swap chain support details
  static SwapChainSupportDetails QuerySwapChainSupport(
      const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface);

  // swap surface format
  static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& available_formats);

  // presentation mode
  static VkPresentModeKHR ChooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& available_present_modes);

  // swap extent
  static VkExtent2D ChooseSwapExtent(
      GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

  // create image view
  static VkImageView CreateImageView(const VkDevice& device,
                                     const VkImage& image,
                                     const VkFormat& format);

  // begin single time command
  static VkCommandBuffer BeginSingleTimeCommand(
      const VkDevice& device, const VkCommandPool& command_pool);

  // end single time command
  static void EndSingleTimeCommand(const VkDevice& device,
                                   const VkQueue& graphics_queue,
                                   const VkCommandPool& command_pool,
                                   VkCommandBuffer& command_buffer);

  // frame buffer resize callback
  static void FramebufferResizeCallback(GLFWwindow* window, int width,
                                        int height);

 private:
  GLFWwindow* window_ = nullptr;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  QueueFamilies queue_families_;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;

  VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swap_chain_images_;
  VkFormat swap_chain_image_format_;
  VkExtent2D swap_chain_extent_;

  std::vector<VkImageView> swap_chain_image_views_;

  VkRenderPass render_pass_ = VK_NULL_HANDLE;

  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  std::vector<VkFramebuffer> swap_chain_framebuffers_;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers_;

  std::vector<VkSemaphore> image_available_semaphores_;
  std::vector<VkSemaphore> render_finished_semaphores_;
  std::vector<VkFence> in_flight_fences_;

  int current_frame_ = 0;
  bool framebuffer_resized_ = false;

  Layer* layer_ = nullptr;
};

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_APPLICATION_H_
