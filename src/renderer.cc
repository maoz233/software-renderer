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
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "SDL.h"
#include "SDL_image.h"
#include "model.h"
#include "utils.h"

#if _WIN32
#pragma warning(disable : 4127)
#endif

namespace swr {
Renderer::Renderer() {
  std::clog << "----- Renderer::Render -----" << std::endl;
}

Renderer::~Renderer() {
  std::clog << "----- Renderer::~Renderer -----" << std::endl;
}

void Renderer::Init() {
  std::clog << "----- Renderer::Init -----" << std::endl;

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error("----- Error::SDL_INIT_FAILURE -----");
  }

  CreateWindow(TITLE, WIDTH, HEIGHT);
  CreateSurface();
  // Initialize zbuffer
  this->zbuffer_ = new std::vector<int>(HEIGHT * WIDTH, INT_MIN);
  LoadModel();
  LoadDiffuseTexture();
}

void Renderer::Loop() {
  std::clog << "----- Renderer::Loop -----" << std::endl;

  Vec3f light_dir(0.f, 0.f, -1.f);

  Mat4 model = Mat4::Identity();

  Vec3f eye(0.f, 0.f, 3.f);
  Vec3f center(0.0f, 0.0f, 0.f);
  Mat4 view = LookAt(eye, center);

  float near = -1.f;
  float far = -100.f;
  float fov = 45.f;
  float aspect_ratio = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
  Mat4 projection = PerspectiveProject(near, far, fov, aspect_ratio);

  Mat4 viewport =
      Viewport(static_cast<float>(WIDTH), static_cast<float>(HEIGHT));

  bool line_is_primitive = false;
  SDL_Event e;
  while (SDL_WaitEvent(&e) && e.type != SDL_QUIT) {
    auto pre = std::chrono::system_clock::now().time_since_epoch();

    switch (e.type) {
      case SDL_KEYDOWN:
        if (e.key.keysym.scancode == SDL_SCANCODE_F) {
          line_is_primitive = !line_is_primitive;

          SDL_LockSurface(this->surface_);
          SDL_memset(this->surface_->pixels, 0,
                     this->surface_->h * this->surface_->pitch);
          SDL_UnlockSurface(this->surface_);
          SDL_UpdateWindowSurface(this->window_);
        }
        break;
      default:
        break;
    }

    // Put rendering logic here
    for (int i = 0; i < this->model_->GetFacesCount(); ++i) {
      std::vector<int> face = this->model_->GetFace(i);
      std::vector<int> normal_indices = this->model_->GetNormalIndices(i);
      std::vector<int> texture_indices = this->model_->GetTextureIndices(i);

      // Tranformation
      std::vector<Vec3f> screen_coords(3);
      for (int j = 0; j < 3; ++j) {
        Vec3f vertex = this->model_->GetVertex(face[j]);
        Vec4 homo_vertex{};
        homo_vertex[0][0] = vertex.x;
        homo_vertex[1][0] = vertex.y;
        homo_vertex[2][0] = vertex.z;
        homo_vertex[3][0] = 1.f;
        Vec4 v = viewport * projection * view * model * homo_vertex;
        screen_coords[j] =
            Vec3f(v[0][0] / v[3][0], v[1][0] / v[3][0], v[2][0] / v[3][0]);
      }

      if (line_is_primitive) {
        for (int j = 0; j < 3; ++j) {
          int x0 = static_cast<int>(std::round(screen_coords[j].x));
          int y0 = static_cast<int>(std::round(screen_coords[j].y));
          int x1 = static_cast<int>(std::round(screen_coords[(j + 1) % 3].x));
          int y1 = static_cast<int>(std::round(screen_coords[(j + 1) % 3].y));
          Uint32 pixel = SDL_MapRGB(this->surface_->format, 255, 255, 255);
          DrawLine(x0, y0, x1, y1, pixel);
        }
      } else {
        std::vector<Vec3f> normal_coords(3);
        std::vector<Vec2f> texture_coords(3);
        std::vector<float> intensities(3);
        for (int j = 0; j < 3; ++j) {
          Vec3f normal = this->model_->GetNormalCoords(normal_indices[j]);
          Vec4 homo_normal{};
          homo_normal[0][0] = normal.x;
          homo_normal[1][0] = normal.y;
          homo_normal[2][0] = normal.z;
          homo_normal[3][0] = 1.f;
          Vec4 n = view * model * homo_normal;
          normal_coords[j] =
              Vec3f(n[0][0] / n[3][0], n[1][0] / n[3][0], n[2][0] / n[3][0]);

          texture_coords[j] =
              this->model_->GetTextureCoords(texture_indices[j]);

          intensities[j] = normal_coords[j].Normalize() * light_dir;
        }

        DrawTriangle(screen_coords, texture_coords, intensities);
      }
    }

    SDL_UpdateWindowSurface(this->window_);

    auto cur = std::chrono::system_clock::now().time_since_epoch();
    auto delta =
        std::chrono::duration_cast<std::chrono::milliseconds>(cur - pre)
            .count();
    float fps = 1000.f / delta;

    std::clog << "----- Delta Time: " << delta << ", FPS: " << fps << "-----"
              << std::endl;
  }
}

void Renderer::Terminate() {
  std::clog << "----- Renderer::Terminate -----" << std::endl;

  if (this->zbuffer_) {
    delete this->zbuffer_;
  }
  if (this->model_) {
    delete this->model_;
  }
  SDL_DestroyWindow(this->window_);
  SDL_Quit();
}

void Renderer::LoadModel(const std::string& filename) {
  std::clog << "----- Renderer::LoadModel -----" << std::endl;

  this->model_ = new Model(filename);
  if (!this->model_) {
    throw std::runtime_error("----- Error::LOAD_MODEL_FAILURE -----");
  }
}

void Renderer::LoadDiffuseTexture(const std::string& filename) {
  std::clog << "----- Renderer::LoadTexture -----" << std::endl;

  SDL_Surface* texture = IMG_Load(filename.c_str());
  this->diffuse_texture_ =
      SDL_ConvertSurface(texture, this->surface_->format, 0);
  if (!this->diffuse_texture_) {
    throw std::runtime_error("----- Error::LOAD_DIFFUSE_TEXTURE_FAILURE -----");
  }
}

void Renderer::CreateWindow(const std::string& title, const int& width,
                            const int& height) {
  std::clog << "----- Renderer::CreateWindow -----" << std::endl;

  this->window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, width, height,
                                   static_cast<Uint32>(0));

  if (!this->window_) {
    throw std::runtime_error("----- Error::SDL_WINDOW_CREATION_FAILURE -----");
  }
}

void Renderer::CreateSurface() {
  std::clog << "----- Renderer::CreateSurface -----" << std::endl;

  this->surface_ = SDL_GetWindowSurface(this->window_);

  if (!this->surface_) {
    throw std::runtime_error("----- Error::SDL_SURFACE_CREATION_FAILURE -----");
  }
}

void Renderer::DrawTriangle(std::vector<Vec3f>& screen_coords,
                            std::vector<Vec2f>& texture_coords,
                            std::vector<float> intensities) {
  // Bounding Box
  int x_min = static_cast<int>(std::round(std::min(
      std::min(screen_coords[0].x, screen_coords[1].x), screen_coords[2].x)));
  int y_min = static_cast<int>(std::round(std::min(
      std::min(screen_coords[0].y, screen_coords[1].y), screen_coords[2].y)));
  int x_max = static_cast<int>(std::round(std::max(
      std::max(screen_coords[0].x, screen_coords[1].x), screen_coords[2].x)));
  int y_max = static_cast<int>(std::round(std::max(
      std::max(screen_coords[0].y, screen_coords[1].y), screen_coords[2].y)));

  for (int x = x_min; x <= x_max; ++x) {
    for (int y = y_min; y <= y_max; ++y) {
      // avoid coordinate beyond surface
      if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) {
        continue;
      }

      Vec3f bc =
          Barycentric(static_cast<float>(x), static_cast<float>(y),
                      screen_coords[0], screen_coords[1], screen_coords[2]);
      if (bc.x < 1e-5 || bc.y < 1e-5 || bc.z < 1e-5) {
        continue;
      }

      // Interpolate z index
      int z = static_cast<int>(std::round(screen_coords[0].z * bc.x +
                                          screen_coords[1].z * bc.y +
                                          screen_coords[2].z * bc.z));

      if ((*(this->zbuffer_))[x + y * WIDTH] < z) {
        // Update z index
        (*(this->zbuffer_))[x + y * WIDTH] = z;

        // Interpolate intensity
        float intensity = intensities[0] * bc.x + intensities[1] * bc.y +
                          intensities[2] * bc.z;

        if (intensity > 0.f) {
          // Interpolate texture coordinates
          int u = static_cast<int>(std::round((texture_coords[0].u * bc.x +
                                               texture_coords[1].u * bc.y +
                                               texture_coords[2].u * bc.z) *
                                              this->diffuse_texture_->w));
          int v = static_cast<int>(std::round((texture_coords[0].v * bc.x +
                                               texture_coords[1].v * bc.y +
                                               texture_coords[2].v * bc.z) *
                                              this->diffuse_texture_->h));

          Uint32 pixel = GetPixel(this->diffuse_texture_, u, v);
          SDL_PixelFormat* format = this->diffuse_texture_->format;
          Uint8 R = static_cast<Uint8>(
              (((pixel & format->Rmask) >> format->Rshift) << format->Rloss) *
              intensity);
          Uint8 G = static_cast<Uint8>(
              (((pixel & format->Gmask) >> format->Gshift) << format->Gloss) *
              intensity);
          Uint8 B = static_cast<Uint8>(
              (((pixel & format->Bmask) >> format->Bshift) << format->Bloss) *
              intensity);

          SetPixel(this->surface_, x, y, SDL_MapRGB(format, R, G, B));
        }
      }
    }
  }
}

void Renderer::DrawLine(int x0, int y0, int x1, int y1, Uint32 pixel) {
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
      SetPixel(this->surface_, y, x, pixel);
    } else {
      SetPixel(this->surface_, x, y, pixel);
    }

    err_2 += d_err_2;
    if (err_2 > d_x) {
      y += y_step;
      err_2 -= d_x * 2;
    }
  }
}

void Renderer::SetPixel(SDL_Surface* surface, int x, int y, Uint32 pixel) {
  // flip surface vertically
  y = HEIGHT - y;

  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
  switch (bpp) {
    case 1:
      *p = static_cast<Uint8>(pixel);
      break;

    case 2:
      *(Uint16*)p = static_cast<Uint16>(pixel);
      break;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
      } else {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
      }
      break;

    case 4:
      *(Uint32*)p = pixel;
      break;
  }
}

Uint32 Renderer::GetPixel(SDL_Surface* surface, int x, int y) {
  // flip surface vertically
  y = surface->h - y;
  // avoid coordinate beyond surface
  if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
    return SDL_MapRGB(surface->format, 255, 255, 255);
  }

  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp) {
    case 1:
      return *p;

    case 2:
      return *(Uint16*)p;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
      return *(Uint32*)p;

    default:
      return 0; /* shouldn't happen, but avoids warnings */
  }
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
}  // namespace swr
