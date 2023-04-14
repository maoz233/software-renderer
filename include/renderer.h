/**
 * @file renderer.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_RENDERER_H_
#define SOFTWARE_RENDERER_INCLUDE_RENDERER_H_
#include <string>
#include <vector>

#include "image.h"
#include "layer.h"
#include "model.h"
#include "shader.h"
#include "utils.h"

namespace swr {

class Renderer : public Layer {
 public:
  Renderer() = default;
  Renderer(VkPhysicalDevice& physical_device, VkDevice& device,
           VkQueue& graphics_queue, VkCommandPool& command_pool);
  ~Renderer();

  virtual void OnUIRender() override;

  void Render();

  void LoadModel(const std::string& filename);
  void LoadTexture(int type, const std::string& filename);

  void DrawTriangle(std::vector<Vec3f>& screen_coords,
                    std::vector<Vec3f>& vertex_coords,
                    std::vector<Vec3f>& normal_coords,
                    std::vector<Vec2f>& texture_coords);
  // Bresenham's line algorithm
  void DrawLine(int x0, int y0, int x1, int y1, uint32_t pixel);

  // set pixel
  void SetPixel(int x, int y, uint32_t pixel);

  static bool InsideTriangle(int x, int y, Vec2i& v0, Vec2i& v1, Vec2i& v2);
  // Barycentric Coordinates
  static Vec3f Barycentric(float x, float y, Vec3f& v0, Vec3f& v1, Vec3f& v2);

  // RGB to RGBA in hexadecimal
  static uint32_t GetColor(const Vec3f& color);

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  uint32_t* surface_data_ = nullptr;
  Image* surface_ = nullptr;
  Model* model_;
  std::vector<int>* zbuffer_ = nullptr;
  Image* diffuse_texture_ = nullptr;
  Image* normal_texture_ = nullptr;
  Image* normal_tangent_texture_ = nullptr;
  Image* specular_texture_ = nullptr;
  Shader* shader_;

  VkPhysicalDevice& physical_device_;
  VkDevice& device_;
  VkQueue& graphics_queue_;
  VkCommandPool& command_pool_;

  float delta_time_ = 0.f;
};
}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_RENDERER_H_
