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

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "SDL.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4201)

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

  DrawLine(80, 60, 560, 420, SDL_MapRGB(this->surface_->format, 255, 255, 255));
  DrawLine(80, 100, 600, 100, SDL_MapRGB(this->surface_->format, 255, 0, 0));
  DrawLine(560, 420, 80, 60, SDL_MapRGB(this->surface_->format, 255, 0, 0));

  SDL_UpdateWindowSurface(this->window_);

  SDL_Event e;
  while (SDL_WaitEvent(&e) && e.type != SDL_QUIT) {
  }
}

void Renderer::Terminate() {
  std::clog << "----- Renderer::Terminate -----" << std::endl;

  SDL_DestroyWindow(this->window_);
  SDL_Quit();
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
}  // namespace swr
