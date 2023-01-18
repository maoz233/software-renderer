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

#include "SDL.h"
#include "model.h"
#include "utils.hpp"

namespace swr {
const int WIDTH = 800;
const int HEIGHT = 600;
const std::string TITLE("Software Renderer");
#if _WIN32
const std::string MODEL_FILENAME("../../obj/african_head.obj");
const std::string DIFFUSE_TEXTURE_FILENAME(
    "../../obj/african_head_diffuse.tga");
#endif
#if __APPLE__
const std::string MODEL_FILENAME("../obj/african_head.obj");
const std::string DIFFUSE_TEXTURE_FILENAME("../obj/african_head_diffuse.tga");
#endif

class Renderer {
 public:
  Renderer();
  ~Renderer();

  void Init();
  void Loop();
  void Terminate();

  void LoadModel(const std::string& filename = MODEL_FILENAME);
  void LoadDiffuseTexture(
      const std::string& filename = DIFFUSE_TEXTURE_FILENAME);

  void CreateWindow(const std::string& title, const int& width,
                    const int& height);
  void CreateSurface();

  void DrawTriangle(std::vector<Vec3f>& screen_coords,
                    std::vector<Vec2f>& texture_coords, float intensity);
  // Bresenham's line algorithm
  void DrawLine(int x0, int y0, int x1, int y1, Uint32 pixel);

  void SetPixel(SDL_Surface* surface, int x, int y, Uint32 pixel);

  Uint32 GetPixel(SDL_Surface* surface, int x, int y);

  static bool InsideTriangle(int x, int y, Vec2i& v0, Vec2i& v1, Vec2i& v2);
  // Barycentric Coordinates
  static Vec3f Barycentric(float x, float y, Vec3f& v0, Vec3f& v1, Vec3f& v2);

 private:
  SDL_Window* window_;
  SDL_Surface* surface_;
  Model* model_;
  std::vector<int>* zbuffer_;
  SDL_Surface* diffuse_texture_;
};
}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_RENDERER_H_
