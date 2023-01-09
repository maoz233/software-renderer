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

#include "SDL.h"
#include "model.h"

namespace swr {
const int WIDTH = 800;
const int HEIGHT = 600;
const std::string TITLE("Software Renderer");
const std::string MODEL_FILENAME("../../obj/african_head.obj");

class Renderer {
 public:
  Renderer();
  ~Renderer();

  void Init();
  void Loop();
  void Terminate();
  void LoadModel(const std::string& filename = MODEL_FILENAME);

  void CreateWindow(const std::string& title, const int& width,
                    const int& height);
  void CreateSurface();

  // Bresenham's line algorithm
  void DrawLine(int x0, int y0, int x1, int y1, Uint32 pixel);

  void SetPixel(int x, int y, Uint32 pixel);

 private:
  SDL_Window* window_;
  SDL_Surface* surface_;
  Model* model_;
};
}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_RENDERER_H_
