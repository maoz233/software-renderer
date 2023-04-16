/**
 * @file renderer.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <execution>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define SOFTWARE_RENDERER_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

#define SOFTWARE_RENDERER_INCLUDE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define SOFTWARE_RENDERER_INCLUDE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "image.h"
#include "model.h"
#include "shader.h"
#include "utils.h"

#if _WIN32
#pragma warning(disable : 4127)
#endif

namespace swr {

#if _WIN32
const std::string MODEL_FILENAME = "../../obj/diablo3/diablo3_pose.obj";
const std::string DIFFUSE_TEXTURE_FILENAME =
    "../../obj/diablo3/diablo3_pose_diffuse.tga";
const std::string NORMAL_TEXTURE_FILENAME =
    "../../obj/diablo3/diablo3_pose_nm.tga";
const std::string NORMAL_TANGENT_TEXTURE_FILENAME =
    "../../obj/diablo3/diablo3_pose_nm_tangent.tga";
const std::string SPECULAR_TEXTURE_FILENAME =
    "../../obj/diablo3/diablo3_pose_spec.tga";
#endif
#if __APPLE__
const std::string MODEL_FILENAME = "../obj/diablo3/diablo3_pose.obj";
const std::string DIFFUSE_TEXTURE_FILENAME =
    "../obj/diablo3/diablo3_pose_diffuse.tga";
const std::string NORMAL_TEXTURE_FILENAME =
    "../obj/diablo3/diablo3_pose_nm.tga";
const std::string NORMAL_TANGENT_TEXTURE_FILENAME =
    "../obj/diablo3/diablo3_pose_nm_tangent.tga";
const std::string SPECULAR_TEXTURE_FILENAME =
    "../obj/diablo3/diablo3_pose_spec.tga";
#endif

Renderer::Renderer(VkPhysicalDevice& physical_device, VkDevice& device,
                   VkQueue& graphics_queue, VkCommandPool& command_pool)
    : physical_device_{physical_device},
      device_{device},
      graphics_queue_{graphics_queue},
      command_pool_{command_pool} {
  std::clog << "----- Renderer::Renderer -----" << std::endl;

  LoadModel(MODEL_FILENAME);
  LoadTexture(Texture::DIFFUSE_TEXTURE, DIFFUSE_TEXTURE_FILENAME);
  LoadTexture(Texture::NORMAL_TEXTURE, NORMAL_TEXTURE_FILENAME);
  LoadTexture(Texture::NORMAL_TANGENT_TEXTURE, NORMAL_TANGENT_TEXTURE_FILENAME);
  LoadTexture(Texture::SPECULAR_TEXTURE, SPECULAR_TEXTURE_FILENAME);
}

Renderer::~Renderer() {
  std::clog << "----- Renderer::~Renderer -----" << std::endl;

  delete[] surface_data_;
  delete[] surface_;
  delete model_;
  delete zbuffer_;
  delete[] diffuse_texture_;
  delete[] normal_texture_;
  delete[] normal_tangent_texture_;
  delete[] specular_texture_;
  delete shader_;
}

void Renderer::OnUIRender() {
  std::clog << "----- Renderer::OnUIRender -----" << std::endl;

  // imgui: scene viewport
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
  ImGui::Begin("Scene");

  width_ = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
  height_ = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

  // imgui: draw image
  if (surface_ && width_ && height_) {
    ImGui::Image(surface_->GetDescritorSet(),
                 {static_cast<float>(width_), static_cast<float>(height_)});
  }

  ImGui::End();
  ImGui::PopStyleVar();

  ImGui::ShowDemoWindow();

  // imgui viewport: settings
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
  ImGui::Begin("Settings");

  // imgui child window: statistics
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
  window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
  window_flags |= ImGuiWindowFlags_MenuBar;
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.f, 5.f));
  ImGui::BeginChild("Statistics", ImVec2(0.f, 100.f), true, window_flags);

  if (ImGui::BeginMenuBar()) {
    ImGui::BeginMenu("Statistics", false);
    ImGui::EndMenuBar();
  }

  // imgui text: time
  ImGui::Text("Time: %.2fms", delta_time_);
  // imgui text: fps
  ImGui::Text("FPS: %.2f", delta_time_ ? 1000.f / delta_time_ : 0.f);
  // imgui text: scene extent detail
  ImGui::Text("Scene: %d * %d", width_, height_);

  ImGui::EndChild();

  //  imgui child window: render
  ImGui::BeginChild("Render", ImVec2(0.f, 220.f), true, window_flags);

  if (ImGui::BeginMenuBar()) {
    ImGui::BeginMenu("Render", false);
    ImGui::EndMenuBar();
  }

  // imgui: primitive mode radio
  ImGui::Text("Primitive Mode:");
  ImGui::Indent();
  ImGui::RadioButton("Frame", &primitive_mode_, 0);
  ImGui::RadioButton("Triangle", &primitive_mode_, 1);
  ImGui::Unindent();

  if (pre_primitive_mode_ != primitive_mode_) {
    need_reset_ = true;
    pre_primitive_mode_ = primitive_mode_;
  }

  // imgui: shading mode radio
  ImGui::Text("Shading Mode:");
  ImGui::Indent();
  ImGui::RadioButton("Diffuse", &shading_mode_, 0);
  ImGui::RadioButton("Phong", &shading_mode_, 1);
  ImGui::RadioButton("Normal Mapping", &shading_mode_, 2);
  ImGui::Unindent();

  if (pre_shading_mode != shading_mode_) {
    need_reset_ = true;
    pre_shading_mode = shading_mode_;
  }

  // imgui: render button
  if (ImGui::Button(pause_ ? "Render" : "Pause")) {
    pause_ = !pause_;
  }

  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  ImGui::End();
  ImGui::PopStyleVar();

  if (!pause_) {
    Render();
  }
}

void Renderer::Render() {
  std::clog << "----- Renderer::Render -----" << std::endl;

  // begin time
  auto begin = std::chrono::high_resolution_clock::now();

  if (!surface_ || width_ != surface_->GetWidth() ||
      height_ != surface_->GetHeight()) {
    // create new image
    surface_ = new Image(width_, height_, physical_device_, device_,
                         graphics_queue_, command_pool_);

    need_reset_ = true;
  }

  if (need_reset_) {
    need_reset_ = false;

    // clear previous image data
    delete[] surface_data_;
    // allocate new image data
    surface_data_ = new uint32_t[width_ * height_];
    memset(surface_data_, 0, width_ * height_ * sizeof(uint32_t));

    // clear previous zbuffer data
    delete zbuffer_;
    // allocate new zbuffer
    zbuffer_ = new std::vector<int>(height_ * width_, INT_MIN);
  }

  Vec3f light(0.f, 0.f, 1.f);

  Vec3f eye(0.f, 0.f, 3.f);
  Vec3f center(0.0f, 0.0f, 0.f);
  Mat4 view = LookAt(eye, center);

  float near = -1.f;
  float far = -100.f;
  float fov = 45.f;
  float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
  Mat4 projection = PerspectiveProject(near, far, fov, aspect_ratio);

  Mat4 viewport =
      Viewport(static_cast<float>(width_), static_cast<float>(height_));

  // Shader
  switch (shading_mode_) {
    case 1:
      shader_ = new PhongShader();
      break;
    case 2:
      shader_ = new NormalMappingShader();
      break;
    default:
      shader_ = new Shader();
      break;
  }

  // Uniform data for shader
  shader_->SetVec3f(Vector::EYE, eye);
  shader_->SetVec3f(Vector::LIGHT, light);

  Mat4 mvp = viewport * projection * view;
  shader_->SetMat4(Matrix::MVP, mvp);

  shader_->SetTexture(Texture::DIFFUSE_TEXTURE, diffuse_texture_);
  shader_->SetTexture(Texture::NORMAL_TEXTURE, normal_texture_);
  shader_->SetTexture(Texture::NORMAL_TANGENT_TEXTURE, normal_tangent_texture_);
  shader_->SetTexture(Texture::SPECULAR_TEXTURE, specular_texture_);

  // Put rendering logic here
  for (int i = 0; i < model_->GetFacesCount(); ++i) {
    std::vector<int> face = model_->GetFace(i);
    std::vector<int> normal_indices = model_->GetNormalIndices(i);
    std::vector<int> texture_indices = model_->GetTextureIndices(i);

    // Transformation
    std::vector<Vec3f> screen_coords(3);
    std::vector<Vec3f> vertex_coords(3);
    for (int j = 0; j < 3; ++j) {
      Vec3f vertex = model_->GetVertex(face[j]);
      vertex_coords.push_back(vertex);
      shader_->SetVec3f(Vector::VERTEX, vertex);
      Vec3f postion{};
      shader_->Vertex(postion);
      screen_coords[j] = postion;
    }

    if (primitive_mode_ == 0) {
      for (int j = 0; j < 3; ++j) {
        int x0 = static_cast<int>(std::round(screen_coords[j].x));
        int y0 = static_cast<int>(std::round(screen_coords[j].y));
        int x1 = static_cast<int>(std::round(screen_coords[(j + 1) % 3].x));
        int y1 = static_cast<int>(std::round(screen_coords[(j + 1) % 3].y));
        uint32_t pixel = GetColor(Vec3f(255.f, 255.f, 255.f));
        DrawLine(x0, y0, x1, y1, pixel);
      }
    } else {
      std::vector<Vec3f> normal_coords(3);
      std::vector<Vec2f> texture_coords(3);
      for (int j = 0; j < 3; ++j) {
        normal_coords[j] = model_->GetNormalCoords(normal_indices[j]);
        texture_coords[j] = model_->GetTextureCoords(texture_indices[j]);
      }

      DrawTriangle(screen_coords, vertex_coords, normal_coords, texture_coords);
    }
  }

  // set image data
  surface_->SetData(surface_data_);

  // end time
  auto end = std::chrono::high_resolution_clock::now();

  // delta time
  delta_time_ =
      std::chrono::duration_cast<std::chrono::duration<float, std::micro>>(
          end - begin)
          .count() /
      1000.f;
}

void Renderer::LoadModel(const std::string& filename) {
  std::clog << "----- Renderer::LoadModel -----" << std::endl;

  model_ = new Model(filename);
  if (!model_) {
    throw std::runtime_error("----- Error::LOAD_MODEL_FAILURE -----");
  }
}

void Renderer::LoadTexture(int type, const std::string& filename) {
  std::clog << "----- Renderer::LoadTexture -----" << std::endl;

  int width, height, channels;
  uint8_t* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

  if (!data) {
    throw std::runtime_error("----- Error::LOAD_TEXTURE_FAILURE -----");
  }

  Image* texture = new Image(width, height, physical_device_, device_,
                             graphics_queue_, command_pool_, data);

  switch (type) {
    case Texture::DIFFUSE_TEXTURE:
      diffuse_texture_ = texture;
      break;
    case Texture::NORMAL_TEXTURE:
      normal_texture_ = texture;
      break;
    case Texture::NORMAL_TANGENT_TEXTURE:
      normal_tangent_texture_ = texture;
      break;
    case Texture::SPECULAR_TEXTURE:
      specular_texture_ = texture;
      break;
    default:
      break;
  }
}

void Renderer::DrawTriangle(std::vector<Vec3f>& screen_coords,
                            std::vector<Vec3f>& vertex_coords,
                            std::vector<Vec3f>& normal_coords,
                            std::vector<Vec2f>& texture_coords) {
  // Bounding Box
  int x_min = static_cast<int>(std::round(std::min(
      std::min(screen_coords[0].x, screen_coords[1].x), screen_coords[2].x)));
  int y_min = static_cast<int>(std::round(std::min(
      std::min(screen_coords[0].y, screen_coords[1].y), screen_coords[2].y)));
  int x_max = static_cast<int>(std::round(std::max(
      std::max(screen_coords[0].x, screen_coords[1].x), screen_coords[2].x)));
  int y_max = static_cast<int>(std::round(std::max(
      std::max(screen_coords[0].y, screen_coords[1].y), screen_coords[2].y)));

  // TBN Matrix
  Vec3f edge1 = screen_coords[1] - screen_coords[0];
  Vec3f edge2 = screen_coords[2] - screen_coords[0];

  // Backface culling
  Vec3f clockwise = edge1 ^ edge2;
  if (clockwise.z < 0.f) {
    return;
  }

  Vec2f delta_uv1 = texture_coords[1] - texture_coords[0];
  Vec2f delta_uv2 = texture_coords[2] - texture_coords[0];
  float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
  Vec3f tagent{
      f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x),
      f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y),
      f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z),
  };
  Vec3f bitangent{
      f * (-delta_uv2.x * edge1.x + delta_uv1.x * edge2.x),
      f * (-delta_uv2.x * edge1.y + delta_uv1.x * edge2.y),
      f * (-delta_uv2.x * edge1.z + delta_uv1.x * edge2.z),
  };
  shader_->SetVec3f(Vector::TANGENT, tagent);
  shader_->SetVec3f(Vector::BITANGENT, bitangent);

  std::vector<int> horizontal_iterator{};
  for (int x = x_min; x <= x_max; ++x) {
    horizontal_iterator.push_back(x);
  }

  std::vector<int> vertical_iterator{};
  for (int y = y_min; y <= y_max; ++y) {
    vertical_iterator.push_back(y);
  }

  std::for_each(
      std::execution::par, horizontal_iterator.begin(),
      horizontal_iterator.end(), [&](int x) {
        std::for_each(
            std::execution::par, vertical_iterator.begin(),
            vertical_iterator.end(), [&](int y) {
              // Avoid coordinate beyond surface
              if (x < 0 || y < 0 || x >= static_cast<int>(width_) ||
                  y >= static_cast<int>(height_)) {
                return;
              }

              Vec3f bc = Barycentric(static_cast<float>(x),
                                     static_cast<float>(y), screen_coords[0],
                                     screen_coords[1], screen_coords[2]);
              if (bc.x < 1e-5 || bc.y < 1e-5 || bc.z < 1e-5) {
                return;
              }

              // Interpolated z index
              int z = static_cast<int>(std::round(screen_coords[0].z * bc.x +
                                                  screen_coords[1].z * bc.y +
                                                  screen_coords[2].z * bc.z));

              // Depth test
              if ((*(zbuffer_))[x + y * width_] >= z) {
                return;
              }

              // Update z index
              (*(zbuffer_))[x + y * width_] = z;

              // Interpolated fragment coordinates
              Vec3f fragment_position{
                  (vertex_coords[0].x * bc.x + vertex_coords[1].x * bc.y +
                   vertex_coords[2].x * bc.z),
                  (vertex_coords[0].y * bc.x + vertex_coords[1].y * bc.y +
                   vertex_coords[2].y * bc.z),
                  (vertex_coords[0].z * bc.x + vertex_coords[1].z * bc.y +
                   vertex_coords[2].z * bc.z),
              };
              shader_->SetVec3f(Vector::FRAGMENT, fragment_position);

              // Interpolated normal vectors
              Vec3f normal{
                  (normal_coords[0].x * bc.x + normal_coords[1].x * bc.y +
                   normal_coords[2].x * bc.z),
                  (normal_coords[0].y * bc.x + normal_coords[1].y * bc.y +
                   normal_coords[2].y * bc.z),
                  (normal_coords[0].z * bc.x + normal_coords[1].z * bc.y +
                   normal_coords[2].z * bc.z),
              };
              shader_->SetVec3f(Vector::NORMAL, normal);

              // Interpolated texture coordinates
              int u = static_cast<int>(std::round(
                  (texture_coords[0].u * bc.x + texture_coords[1].u * bc.y +
                   texture_coords[2].u * bc.z) *
                  diffuse_texture_->GetWidth()));
              int v = static_cast<int>(std::round(
                  (texture_coords[0].v * bc.x + texture_coords[1].v * bc.y +
                   texture_coords[2].v * bc.z) *
                  diffuse_texture_->GetHeight()));
              Vec2i uv{u, v};
              shader_->SetVec2i(uv);

              uint32_t pixel = 0;
              shader_->Fragment(pixel);

              SetPixel(x, y, pixel);
            });
      });
  // for (int x = x_min; x <= x_max; ++x) {
  //   for (int y = y_min; y <= y_max; ++y) {
  //     // Avoid coordinate beyond surface
  //     if (x < 0 || y < 0 || x >= static_cast<int>(width_) ||
  //         y >= static_cast<int>(height_)) {
  //       continue;
  //     }

  //     Vec3f bc =
  //         Barycentric(static_cast<float>(x), static_cast<float>(y),
  //                     screen_coords[0], screen_coords[1], screen_coords[2]);
  //     if (bc.x < 1e-5 || bc.y < 1e-5 || bc.z < 1e-5) {
  //       continue;
  //     }

  //     // Interpolated z index
  //     int z = static_cast<int>(std::round(screen_coords[0].z * bc.x +
  //                                         screen_coords[1].z * bc.y +
  //                                         screen_coords[2].z * bc.z));

  //     // Depth test
  //     if ((*(zbuffer_))[x + y * width_] >= z) {
  //       continue;
  //     }

  //     // Update z index
  //     (*(zbuffer_))[x + y * width_] = z;

  //     // Interpolated fragment coordinates
  //     Vec3f fragment_position{
  //         (vertex_coords[0].x * bc.x + vertex_coords[1].x * bc.y +
  //          vertex_coords[2].x * bc.z),
  //         (vertex_coords[0].y * bc.x + vertex_coords[1].y * bc.y +
  //          vertex_coords[2].y * bc.z),
  //         (vertex_coords[0].z * bc.x + vertex_coords[1].z * bc.y +
  //          vertex_coords[2].z * bc.z),
  //     };
  //     shader_->SetVec3f(Vector::FRAGMENT, fragment_position);

  //     // Interpolated normal vectors
  //     Vec3f normal{
  //         (normal_coords[0].x * bc.x + normal_coords[1].x * bc.y +
  //          normal_coords[2].x * bc.z),
  //         (normal_coords[0].y * bc.x + normal_coords[1].y * bc.y +
  //          normal_coords[2].y * bc.z),
  //         (normal_coords[0].z * bc.x + normal_coords[1].z * bc.y +
  //          normal_coords[2].z * bc.z),
  //     };
  //     shader_->SetVec3f(Vector::NORMAL, normal);

  //     // Interpolated texture coordinates
  //     int u = static_cast<int>(
  //         std::round((texture_coords[0].u * bc.x + texture_coords[1].u * bc.y
  //         +
  //                     texture_coords[2].u * bc.z) *
  //                    diffuse_texture_->GetWidth()));
  //     int v = static_cast<int>(
  //         std::round((texture_coords[0].v * bc.x + texture_coords[1].v * bc.y
  //         +
  //                     texture_coords[2].v * bc.z) *
  //                    diffuse_texture_->GetHeight()));
  //     Vec2i uv{u, v};
  //     shader_->SetVec2i(uv);

  //     uint32_t pixel = 0;
  //     shader_->Fragment(pixel);

  //     SetPixel(x, y, pixel);
  //   }
  // }
}

void Renderer::DrawLine(int x0, int y0, int x1, int y1, uint32_t pixel) {
  bool steep = false;

  // Ensure slope less than 1
  if (std::abs(x1 - x0) < std::abs(y1 - y0)) {
    std::swap(x0, y0);
    std::swap(x1, y1);
    steep = true;
  }

  // Ensure left-to-right
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int y = y0;
  int d_x = x1 - x0;
  int d_err_2 = 2 * std::abs(y1 - y0);
  int err_2 = 0;
  int y_step = y1 > y0 ? 1 : -1;

  for (int x = x0; x <= x1; ++x) {
    if (steep) {
      SetPixel(y, x, pixel);
    } else {
      SetPixel(x, y, pixel);
    }

    err_2 += d_err_2;
    if (err_2 > d_x) {
      y += y_step;
      err_2 -= d_x * 2;
    }
  }
}

void Renderer::SetPixel(int x, int y, uint32_t pixel) {
  y = height_ - y;
  surface_data_[(y - 1) * width_ + x] = pixel;
}

bool Renderer::InsideTriangle(int x, int y, Vec2i& v0, Vec2i& v1, Vec2i& v2) {
  Vec3i v0_v1(v1.x - v0.x, v1.y - v0.y, 0);
  Vec3i v0_p(x - v0.x, y - v0.y, 0);
  Vec3i v1_v2(v2.x - v1.x, v2.y - v1.y, 0);
  Vec3i v1_p(x - v1.x, y - v1.y, 0);
  Vec3i v2_v0(v0.x - v2.x, v0.y - v2.y, 0);
  Vec3i v2_p(x - v2.x, y - v2.y, 0);

  Vec3i z0 = v0_v1 ^ v0_p;
  Vec3i z1 = v1_v2 ^ v1_p;
  Vec3i z2 = v2_v0 ^ v2_p;
  if (z0.z == 0 || z1.z == 0 || z2.z == 0) {
    return true;
  }

  return ((z0.z > 0) == (z1.z > 0)) && ((z0.z > 0) == (z2.z > 0));
}

Vec3f Renderer::Barycentric(float x, float y, Vec3f& v0, Vec3f& v1, Vec3f& v2) {
  Vec3f u = Vec3f(v2.x - v0.x, v1.x - v0.x, v0.x - x) ^
            Vec3f(v2.y - v0.y, v1.y - v0.y, v0.y - y);

  return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

uint32_t Renderer::GetColor(const Vec3f& color) {
  uint32_t R = static_cast<uint32_t>(color.x);
  uint32_t G = static_cast<uint32_t>(color.y);
  uint32_t B = static_cast<uint32_t>(color.z);

  return (255 << 24) | (B << 16) | (G << 8) | R;
}

}  // namespace swr
