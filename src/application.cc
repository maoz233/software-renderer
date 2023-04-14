/**
 * @file application.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 1.0
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "application.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#define SOFRWARE_RENDERER_INCLUDE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "renderer.h"

namespace swr {

Application::Application() {
  // create GLFW window
  CreateWindow();
  // create Vulkan instance
  CreateInstance();

  // create Vulkan debug messenger
#ifdef RAY_TRACING_ENABLE_VULKAN_VALIDATION_LAYER
  SetupDebugMessenger();
#endif

  // create window surface
  CreateWindowSurface();
  // pick physical device
  PickPhysicalDevice();
  // create logical device
  CreateLogicalDevice();
  // create swap chain
  CreateSwapChain();
  // create image views
  CreateImageViews();
  // create render pass
  CreateRenderPass();
  // create descriptor pool
  CreateDescriptorPool();
  // create framebuffers
  CreateFramebuffers();
  // create command pool
  CreateCommandPool();
  // create command buffers
  CreateCommandBuffers();
  // create fences and semaphores
  CreateSyncObjects();
  // setup Dear ImGui
  SetupImGui();

  // setup scene layer
  layer_ =
      new Renderer(physical_device_, device_, graphics_queue_, command_pool_);
}

Application::~Application() {
  // destroy scene layer
  delete layer_;

  // Dear ImGui cleanup
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // Vulkan cleanup
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
    vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
    vkDestroyFence(device_, in_flight_fences_[i], nullptr);
  }

  vkDestroyCommandPool(device_, command_pool_, nullptr);

  for (auto framebuffer : swap_chain_framebuffers_) {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }

  vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
  vkDestroyRenderPass(device_, render_pass_, nullptr);

  for (auto image_view : swap_chain_image_views_) {
    vkDestroyImageView(device_, image_view, nullptr);
  }

  vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
  vkDestroyDevice(device_, nullptr);
  vkDestroySurfaceKHR(instance_, surface_, nullptr);

#ifdef ENABLE_VULKAN_VALIDATION_LAYER
  DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
#endif

  vkDestroyInstance(instance_, nullptr);

  // GLFW cleanup
  glfwDestroyWindow(window_);
  glfwTerminate();
}

void Application::Run() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    // draw frame
    DrawFrame();
  }

  // wait for device idle
  vkDeviceWaitIdle(device_);
}

void Application::CreateWindow() {
  // setup GLFW error callback
  glfwSetErrorCallback(GLFWErrorCallback);

  // init GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Error::GLFW: Failed to initialize GLFW!");
  }

  // configure GLFW for Vulkan
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // create GLFW window
  window_ = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
  if (!window_) {
    throw std::runtime_error("Error::GLFW: Failed to create GLFW window!");
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);
}

void Application::CreateInstance() {
  // Vulkan instance info
  VkInstanceCreateInfo instance_info{};
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef __APPLE__
  instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  // Vulkan application info
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Ray Tracing";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  instance_info.pApplicationInfo = &app_info;

  // Vulkan instance required extensions
  std::vector<const char*> extensions = QueryVulkanInstanceExts();

  instance_info.enabledExtensionCount =
      static_cast<uint32_t>(extensions.size());
  instance_info.ppEnabledExtensionNames = extensions.data();

// Vulkan enabled layer
#ifdef RAY_TRACING_ENABLE_VULKAN_VALIDATION_LAYER
  std::vector<const char*> layers = QueryVulkanLayers();

  instance_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
  instance_info.ppEnabledLayerNames = layers.data();
#else
  instance_info.enabledLayerCount = 0;
#endif

  // create the Vulkan instance
  VkResult result = vkCreateInstance(&instance_info, nullptr, &instance_);
  CheckVulkanResult(result, "Failed to create Vulkan instance!");
}

void Application::SetupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
  messenger_info.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  messenger_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  messenger_info.pfnUserCallback = DebugCallBack;
  messenger_info.pUserData = nullptr;

  VkResult result = CreateDebugUtilsMessengerEXT(instance_, &messenger_info,
                                                 nullptr, &debug_messenger_);
  CheckVulkanResult(result, "Failed to create debug messenger!");
}

void Application::CreateWindowSurface() {
  glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
}

void Application::PickPhysicalDevice() {
  // query all physical devices
  uint32_t physical_device_cnt = 0;
  vkEnumeratePhysicalDevices(instance_, &physical_device_cnt, nullptr);
  if (!physical_device_cnt) {
    throw std::runtime_error(
        "Error::Vulkan: Failed to find GPUs width Vulkan support!");
  }

  std::vector<VkPhysicalDevice> physical_devices(physical_device_cnt);
  vkEnumeratePhysicalDevices(instance_, &physical_device_cnt,
                             physical_devices.data());

  // sort physical devices by evaluated score
  std::multimap<int, VkPhysicalDevice> candidates{};
  for (const auto& physical_device : physical_devices) {
    int score = EvaluatePhysicalDevice(physical_device, surface_);
    candidates.insert(std::make_pair(score, physical_device));
  }

  if (candidates.rbegin()->first > 0) {
    physical_device_ = candidates.rbegin()->second;
  }

  if (VK_NULL_HANDLE == physical_device_) {
    throw std::runtime_error("Error::Vulkan: Failed to find a suitable GPU!");
  }
}

void Application::CreateLogicalDevice() {
  VkDeviceCreateInfo device_info{};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  // query queue infos
  float queue_priority = 1.f;
  queue_families_ = QueryQueueFamilies(physical_device_, surface_);
  std::set<uint32_t> unique_queue_families{
      queue_families_.graphics_family.value(),
      queue_families_.present_family.value()};

  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  for (const auto& family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    queue_infos.push_back(queue_info);
  }

  device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
  device_info.pQueueCreateInfos = queue_infos.data();

  // physical device features
  VkPhysicalDeviceFeatures physical_device_features{};
  physical_device_features.samplerAnisotropy = VK_TRUE;

  device_info.pEnabledFeatures = &physical_device_features;

  //  physical device extensions
  std::vector<const char*> physical_device_extensions =
      QueryVulkanDeviceExts(physical_device_);

  device_info.enabledExtensionCount =
      static_cast<uint32_t>(physical_device_extensions.size());
  device_info.ppEnabledExtensionNames = physical_device_extensions.data();

  // physical device layers
#ifdef RAY_TRACING_ENABLE_VULKAN_VALIDATION_LAYER
  std::vector<const char*> physical_device_layers = QueryVulkanLayers();

  device_info.enabledLayerCount =
      static_cast<uint32_t>(physical_device_layers.size());
  device_info.ppEnabledLayerNames = physical_device_layers.data();
#else
  device_info.enabledLayerCount = 0;
#endif

  VkResult result =
      vkCreateDevice(physical_device_, &device_info, nullptr, &device_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create logical device!");

  // graphics queue
  vkGetDeviceQueue(device_, queue_families_.graphics_family.value(), 0,
                   &graphics_queue_);
  // present queue
  vkGetDeviceQueue(device_, queue_families_.present_family.value(), 0,
                   &present_queue_);
}

void Application::CreateSwapChain() {
  SwapChainSupportDetails swap_chain_support =
      QuerySwapChainSupport(physical_device_, surface_);
  VkSurfaceFormatKHR surface_format =
      ChooseSwapSurfaceFormat(swap_chain_support.formats);
  VkPresentModeKHR present_mode =
      ChooseSwapPresentMode(swap_chain_support.present_modes);
  VkExtent2D extent =
      ChooseSwapExtent(window_, swap_chain_support.capabilities);

  uint32_t image_cnt = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 &&
      image_cnt > swap_chain_support.capabilities.maxImageCount) {
    image_cnt = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swap_chain_info{};
  swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swap_chain_info.surface = surface_;
  swap_chain_info.minImageCount = image_cnt;
  swap_chain_info.imageFormat = surface_format.format;
  swap_chain_info.imageColorSpace = surface_format.colorSpace;
  swap_chain_info.imageExtent = extent;
  swap_chain_info.imageArrayLayers = 1;
  swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queue_family_indices[] = {queue_families_.graphics_family.value(),
                                     queue_families_.present_family.value()};
  if (queue_families_.graphics_family.value() !=
      queue_families_.present_family.value()) {
    swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swap_chain_info.queueFamilyIndexCount = 2;
    swap_chain_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain_info.queueFamilyIndexCount = 0;
    swap_chain_info.pQueueFamilyIndices = nullptr;
  }

  swap_chain_info.preTransform =
      swap_chain_support.capabilities.currentTransform;
  swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swap_chain_info.presentMode = present_mode;
  swap_chain_info.clipped = VK_TRUE;
  swap_chain_info.oldSwapchain = VK_NULL_HANDLE;

  VkResult result =
      vkCreateSwapchainKHR(device_, &swap_chain_info, nullptr, &swap_chain_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create swap chain!");

  // swap chain images
  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_cnt, nullptr);
  swap_chain_images_.resize(image_cnt);
  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_cnt,
                          swap_chain_images_.data());

  // swap chain format
  swap_chain_image_format_ = surface_format.format;

  // swap chain extent
  swap_chain_extent_ = extent;
}

void Application::CreateImageViews() {
  swap_chain_image_views_.resize(swap_chain_images_.size());

  for (size_t i = 0; i < swap_chain_images_.size(); ++i) {
    swap_chain_image_views_[i] = CreateImageView(device_, swap_chain_images_[i],
                                                 swap_chain_image_format_);
  }
}

void Application::CreateRenderPass() {
  // color attachment
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swap_chain_image_format_;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // subpass
  VkSubpassDescription subpass_desc{};
  subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_desc.colorAttachmentCount = 1;
  subpass_desc.pColorAttachments = &color_attachment_ref;

  // dependency
  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass_desc;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  VkResult result =
      vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create render pass!");
}

void Application::CreateDescriptorPool() {
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VkResult result =
      vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_);
  CheckVulkanResult(result, "Error:Vulkan: Failed to create descriptor pool!");
}

void Application::CreateFramebuffers() {
  swap_chain_framebuffers_.resize(swap_chain_image_views_.size());

  for (size_t i = 0; i < swap_chain_image_views_.size(); i++) {
    VkImageView attachments[] = {swap_chain_image_views_[i]};

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = swap_chain_extent_.width;
    framebuffer_info.height = swap_chain_extent_.height;
    framebuffer_info.layers = 1;

    VkResult result = vkCreateFramebuffer(device_, &framebuffer_info, nullptr,
                                          &swap_chain_framebuffers_[i]);
    CheckVulkanResult(result, "Error::Vulkan: Failed to create framebuffer!");
  }
}

void Application::CreateCommandPool() {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_families_.graphics_family.value();

  VkResult result =
      vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_);

  CheckVulkanResult(result, "Error::Vulkan: Failed to create command pool!");
}

void Application::CreateCommandBuffers() {
  command_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount =
      static_cast<uint32_t>(command_buffers_.size());

  VkResult result =
      vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data());
  CheckVulkanResult(result,
                    "Error::Vulkan: Failed to allocate command buffers!");
}

void Application::CreateSyncObjects() {
  image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
  render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkResult result = vkCreateSemaphore(device_, &semaphore_info, nullptr,
                                        &image_available_semaphores_[i]);
    CheckVulkanResult(result, "Error::Vulkan: Failed to create semaphores!");

    result = vkCreateSemaphore(device_, &semaphore_info, nullptr,
                               &render_finished_semaphores_[i]);
    CheckVulkanResult(result, "Error::Vulkan: Failed to create semaphores!");
  }

  in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkResult result =
        vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]);
    CheckVulkanResult(result, "Error::Vulkan: Failed to create fences!");
  }
}

void Application::SetupImGui() {
  // setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // enable Keyboard Controls
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  // setup Dear ImGui style
  ImGui::StyleColorsDark();

  // setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(window_, true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance_;
  init_info.PhysicalDevice = physical_device_;
  init_info.Device = device_;
  init_info.QueueFamily = queue_families_.graphics_family.value();
  init_info.Queue = graphics_queue_;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = descriptor_pool_;
  init_info.Subpass = 0;
  init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
  init_info.ImageCount = static_cast<uint32_t>(swap_chain_images_.size());
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = nullptr;

  ImGui_ImplVulkan_Init(&init_info, render_pass_);

  // load fonts
  io.Fonts->AddFontFromFileTTF(FONTS_FILEPATH, 15.0f);

  // upload font
  VkCommandBuffer command_buffer =
      BeginSingleTimeCommand(device_, command_pool_);
  ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
  EndSingleTimeCommand(device_, graphics_queue_, command_pool_, command_buffer);

  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Application::DrawFrame() {
  // wait for fence to be signaled
  vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE,
                  UINT64_MAX);

  // imgui: new frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
    window_flags |= ImGuiWindowFlags_NoBackground;
  }

  // imgui: dock space
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Docker", nullptr, window_flags);
  ImGui::PopStyleVar();

  ImGui::PopStyleVar(2);

  // submit the dockSpace
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("RayTracer");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
  }

  // imgui: menu bar
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  if (layer_) {
    layer_->OnUIRender();
  }

  ImGui::End();
  ImGui::Render();

  // imgui: update and render additional Platform Windows
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  // grab an image from swap chain, and then signaled image available
  // semaphore
  uint32_t image_index;
  VkResult result =
      vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX,
                            image_available_semaphores_[current_frame_],
                            VK_NULL_HANDLE, &image_index);

  if (VK_ERROR_OUT_OF_DATE_KHR == result) {
    RecreateSwapChain();
    return;
  } else if (VK_SUCCESS != result && VK_SUBOPTIMAL_KHR != result) {
    throw std::runtime_error(
        "----- Error::Vulkan: Failed to acquire swap chain image -----");
  }

  // only reset fence to unsignaled if we are submitting work
  vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);

  // make sure command buffer is ready to use
  vkResetCommandBuffer(command_buffers_[current_frame_], 0);
  // // create and record command buffer
  RecordCommandBuffer(command_buffers_[current_frame_], image_index);

  // wait image available semaphore, then signaled render finished semaphore
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers_[current_frame_];

  VkSemaphore signal_semaphores[] = {
      render_finished_semaphores_[current_frame_]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  // submit graphics queue, then signaled fence
  result = vkQueueSubmit(graphics_queue_, 1, &submit_info,
                         in_flight_fences_[current_frame_]);
  CheckVulkanResult(result,
                    "Error::Vulkan: Failed to submit draw command buffer!");

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swap_chains[] = {swap_chain_};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &image_index;

  present_info.pResults = nullptr;

  result = vkQueuePresentKHR(present_queue_, &present_info);

  if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result ||
      framebuffer_resized_) {
    RecreateSwapChain();
  } else {
    CheckVulkanResult(result, "Error::Vulkan: Failed to present image1");
  }

  current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::RecreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);

  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window_, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device_);

  CleanupSwapChain();

  CreateSwapChain();
  CreateImageViews();
  CreateFramebuffers();
}

void Application::CleanupSwapChain() {
  for (auto framebuffer : swap_chain_framebuffers_) {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }

  for (auto image_view : swap_chain_image_views_) {
    vkDestroyImageView(device_, image_view, nullptr);
  }

  vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
}

void Application::RecordCommandBuffer(VkCommandBuffer command_buffer,
                                      uint32_t image_index) {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = nullptr;

  VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
  CheckVulkanResult(result,
                    "Error::Vulkan: Failed to begin recording command buffer!");

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = render_pass_;
  render_pass_info.framebuffer = swap_chain_framebuffers_[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = swap_chain_extent_;

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(command_buffer, &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  // ImGui: render draw data
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

  vkCmdEndRenderPass(command_buffer);

  result = vkEndCommandBuffer(command_buffer);
  CheckVulkanResult(result, "Error::Vulkan: Failed to record command buffer!");
}

void Application::SetFramebufferResized(bool resized) {
  framebuffer_resized_ = resized;
}

inline bool QueueFamilies::IsCompleted() {
  return graphics_family.has_value() && present_family.has_value();
}

inline bool SwapChainSupportDetails::IsAdequate() {
  return capabilities.minImageCount > 0 && !formats.empty() &&
         !present_modes.empty();
}

void Application::GLFWErrorCallback(const int code, const char* description) {
  std::cerr << "Error::GLFW:\n\tCode: " << code
            << "\n\tDescription: " << description << "\n";
}

void Application::CheckVulkanResult(const int result, const char* error_msg) {
  if (VK_SUCCESS != result) {
    throw std::runtime_error(error_msg);
  }
}

std::vector<const char*> Application::QueryVulkanInstanceExts() {
  std::vector<const char*> required_exts{};

  // GLFW required extensions
  uint32_t glfw_ext_cnt = 0;
  const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_cnt);
  for (uint32_t i = 0; i < glfw_ext_cnt; ++i) {
    required_exts.push_back(glfw_exts[i]);
  }

#ifdef __APPLE__
  required_exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  required_exts.push_back("VK_KHR_get_physical_device_properties2");
#endif

  required_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // available instance extensions
  uint32_t available_extension_cnt = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_cnt,
                                         nullptr);
  std::vector<VkExtensionProperties> available_extensions(
      available_extension_cnt);
  vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_cnt,
                                         available_extensions.data());

  // compare
  std::set<std::string> extensions(required_exts.begin(), required_exts.end());
  for (const auto& extension : available_extensions) {
    extensions.erase(extension.extensionName);
  }

  if (!extensions.empty()) {
    throw std::runtime_error(
        "Error::Vulkan: Error::Vulkan: Find not supported instance "
        "extension(s)!");
  }

  return required_exts;
}

std::vector<const char*> Application::QueryVulkanLayers() {
  std::vector<const char*> required_layers{};

  // add validation layer
  required_layers.push_back("VK_LAYER_KHRONOS_validation");

  // available instance layers
  uint32_t available_layer_cnt = 0;
  vkEnumerateInstanceLayerProperties(&available_layer_cnt, nullptr);
  std::vector<VkLayerProperties> available_layers(available_layer_cnt);
  vkEnumerateInstanceLayerProperties(&available_layer_cnt,
                                     available_layers.data());

  // compare
  std::set<std::string> layers(required_layers.begin(), required_layers.end());
  for (const auto& layer : available_layers) {
    layers.erase(layer.layerName);
  }

  if (!layers.empty()) {
    throw std::runtime_error("Error::Vulkan: Find not supported layer(s)!");
  }

  return required_layers;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallBack(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
  if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::clog << "----- Validation Layer: "
              << "\n\tSeverity: " << message_severity
              << "\n\tType: " << message_type
              << "\n\tMessage: " << callback_data->pMessage
              << "\n\tUser Data Address: " << user_data << "\n";
  }

  return VK_FALSE;
}

VkResult Application::CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_messenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");

  if (func) {
    return func(instance, create_info, allocator, debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void Application::DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* allocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");

  if (func) {
    func(instance, debug_messenger, allocator);
  } else {
    throw std::runtime_error(
        "Error::Vualkan: Failed to destroy debug utils messenger!");
  }
}

int Application::EvaluatePhysicalDevice(const VkPhysicalDevice& physical_device,
                                        const VkSurfaceKHR& surface) {
  int score = 0;

  // physical device properties
  VkPhysicalDeviceProperties device_properties{};
  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == device_properties.deviceType) {
    score += 1000;
  }
  score += device_properties.limits.maxImageDimension2D;

  // physical device features
  VkPhysicalDeviceFeatures device_features{};
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);
  if (!device_features.geometryShader) {
    score -= 1000;
  }

  // physical device queue family support
  QueueFamilies queue_faimlies = QueryQueueFamilies(physical_device, surface);
  if (!queue_faimlies.IsCompleted()) {
    score = -1000;
  }

  // physical device extension support
  std::vector<const char*> required_exts{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef __APPLE__
  required_exts.push_back("VK_KHR_portability_subset");
#endif

  uint32_t available_ext_cnt = 0;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &available_ext_cnt, nullptr);
  std::vector<VkExtensionProperties> available_exts(available_ext_cnt);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &available_ext_cnt, available_exts.data());

  // compare
  std::set<std::string> extensions(required_exts.begin(), required_exts.end());
  for (const auto& extension : available_exts) {
    extensions.erase(extension.extensionName);
  }

  if (!extensions.empty()) {
    score -= 1000;
  }

  // physical device swap chain support
  SwapChainSupportDetails swap_chain_support =
      QuerySwapChainSupport(physical_device, surface);

  if (!swap_chain_support.IsAdequate()) {
    score -= 1000;
  }

  std::clog << "Physical Device: " << device_properties.deviceName << "\n";

  return score;
}

QueueFamilies Application::QueryQueueFamilies(
    const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface) {
  QueueFamilies indices{};

  // queue families
  uint32_t queue_family_cnt = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_cnt,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_cnt);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_cnt,
                                           queue_families.data());

  // get index of required queue family
  for (uint32_t i = 0; i < queue_family_cnt; ++i) {
    if (queue_families[i].queueCount > 0 &&
        (VK_QUEUE_GRAPHICS_BIT & queue_families[i].queueFlags)) {
      indices.graphics_family = i;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface,
                                         &present_support);
    if (queue_families[i].queueCount > 0 && present_support) {
      indices.present_family = i;
    }

    if (indices.IsCompleted()) {
      break;
    }
  }

  return indices;
}

std::vector<const char*> Application::QueryVulkanDeviceExts(
    const VkPhysicalDevice& physical_device) {
  std::vector<const char*> required_exts{};

  // swap chain extension
  required_exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#ifdef __APPLE__
  required_exts.push_back("VK_KHR_portability_subset");
#endif

  // available device extensions
  uint32_t available_ext_cnt = 0;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &available_ext_cnt, nullptr);
  std::vector<VkExtensionProperties> available_exts(available_ext_cnt);
  vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &available_ext_cnt, available_exts.data());

  // compare
  std::set<std::string> extensions(required_exts.begin(), required_exts.end());
  for (const auto& extension : available_exts) {
    extensions.erase(extension.extensionName);
  }

  if (!extensions.empty()) {
    throw std::runtime_error(
        "Error::Vulkan: Error::Vulkan: Find not supported device "
        "extension(s)!");
  }

  return required_exts;
}

SwapChainSupportDetails Application::QuerySwapChainSupport(
    const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface) {
  SwapChainSupportDetails details{};

  // capabilities: min/max number of images in swap chain, min/max width and
  // height of images
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &details.capabilities);

  // formats: pixel format, color space
  uint32_t format_cnt = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_cnt,
                                       nullptr);
  if (0 != format_cnt) {
    details.formats.resize(format_cnt);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_cnt,
                                         details.formats.data());
  }

  // present modes
  uint32_t present_mode_cnt = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                            &present_mode_cnt, nullptr);
  if (0 != present_mode_cnt) {
    details.present_modes.resize(present_mode_cnt);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                              &present_mode_cnt,
                                              details.present_modes.data());
  }

  return details;
}

VkSurfaceFormatKHR Application::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available_formats) {
  for (const auto& available : available_formats) {
    if (available.format == VK_FORMAT_R8G8B8A8_UNORM &&
        available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available;
    }
  }

  return available_formats[0];
}

VkPresentModeKHR Application::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& available_present_modes) {
  for (const auto& available : available_present_modes) {
    if (available == VK_PRESENT_MODE_MAILBOX_KHR) {
      return available;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapExtent(
    GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height)};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actual_extent;
  }
}

VkImageView Application::CreateImageView(const VkDevice& device,
                                         const VkImage& image,
                                         const VkFormat& format) {
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkImageView image_view;

  VkResult result = vkCreateImageView(device, &view_info, nullptr, &image_view);
  CheckVulkanResult(result, "Error::Vulkan: Failed to create image view!");

  return image_view;
}

VkCommandBuffer Application::BeginSingleTimeCommand(
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

void Application::EndSingleTimeCommand(const VkDevice& device,
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

void Application::FramebufferResizeCallback(GLFWwindow* window, int width,
                                            int height) {
  if (width && height) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->SetFramebufferResized(true);
  }
}

}  // namespace swr
