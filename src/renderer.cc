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
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "SDL.h"
#include "model.h"
#include "utils.hpp"
#pragma warning(disable : 4127)

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
}

void Renderer::Loop() {
  std::clog << "----- Renderer::Loop -----" << std::endl;

  Vec3f light_dir(0.f, 0.f, -1.f);

  for (int i = 0; i < this->model_->GetFacesCount(); ++i) {
    std::vector<int> face = this->model_->GetFace(i);

    // Draw with line
    // for (int j = 0; j < 3; ++j) {
    //   Vec3f v0 = this->model_->GetVertex(face[j]);
    //   Vec3f v1 = this->model_->GetVertex(face[(j + 1) % 3]);
    //   int x0 = static_cast<int>((v0.x + 1) * WIDTH / 2.);
    //   int y0 = static_cast<int>((v0.y + 1) * HEIGHT / 2.);
    //   int x1 = static_cast<int>((v1.x + 1) * WIDTH / 2.);
    //   int y1 = static_cast<int>((v1.y + 1) * HEIGHT / 2.);

    //   DrawLine(x0, y0, x1, y1,
    //            SDL_MapRGB(this->surface_->format, 255, 255, 255));
    // }

    // Draw with triangle
    Vec2i screen_coords[3];
    Vec3f world_coords[3];
    for (int j = 0; j < 3; ++j) {
      Vec3f vertex = this->model_->GetVertex(face[j]);
      screen_coords[j] =
          Vec2i(static_cast<int>((vertex.x + 1.f) * WIDTH / 2.f),
                static_cast<int>((vertex.y + 1.f) * HEIGHT / 2.f));
      world_coords[j] = vertex;
    }

    Vec3f normal = (world_coords[2] - world_coords[0]) ^
                   (world_coords[1] - world_coords[0]);
    normal.Normalize();
    float intensity = normal * light_dir;
    if (intensity > 0) {
      DrawTriangle(screen_coords[0], screen_coords[1], screen_coords[2],
                   SDL_MapRGB(this->surface_->format,
                              static_cast<Uint8>(255 * intensity),
                              static_cast<Uint8>(255 * intensity),
                              static_cast<Uint8>(255 * intensity)));
    }
  }

  SDL_UpdateWindowSurface(this->window_);

  SDL_Event e;
  while (SDL_WaitEvent(&e) && e.type != SDL_QUIT) {
  }
}

void Renderer::Terminate() {
  std::clog << "----- Renderer::Terminate -----" << std::endl;

  if (this->model_) {
    delete this->model_;
  }
  SDL_DestroyWindow(this->window_);
  SDL_Quit();
}

void Renderer::LoadModel(const std::string& filename) {
  std::clog << "----- Renderer::LoadModel -----" << std::endl;

  this->model_ = new Model(filename);
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

void Renderer::DrawTriangle(Vec2i& v0, Vec2i& v1, Vec2i& v2, Uint32 pixel) {
  // Bounding Box
  int x_min = std::min(std::min(v0.x, v1.x), v2.x);
  int y_min = std::min(std::min(v0.y, v1.y), v2.y);
  int x_max = std::max(std::max(v0.x, v1.x), v2.x);
  int y_max = std::max(std::max(v0.y, v1.y), v2.y);

  for (int x = x_min; x <= x_max; ++x) {
    for (int y = y_min; y <= y_max; ++y) {
      if (InsideTriangle(x, y, v0, v1, v2)) {
        SetPixel(x, y, pixel);
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

void Renderer::SetPixel(int x, int y, Uint32 pixel) {
  // flip surface vertically
  y = HEIGHT - y;
  // avoid coordinate beyond surface
  if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) {
    return;
  }

  int bpp = this->surface_->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8* p =
      (Uint8*)this->surface_->pixels + y * this->surface_->pitch + x * bpp;

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
}  // namespace swr
