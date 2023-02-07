/**
 * @file shader.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-02-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_SHADER_H_
#define SOFTWARE_RENDERER_INCLUDE_SHADER_H_
#include "SDL.h"
#include "utils.h"

namespace swr {
Uint32 GetPixel(SDL_Surface* surface, int x, int y);

class Shader {
 public:
  Shader(Mat4 viewport, Mat4 projection, Mat4 view);
  virtual ~Shader();

  virtual void Vertex(Vec3f& vertex, Vec3f& coord);
  virtual bool Fragment(Vec3f& light, Vec3f& barycentric,
                        std::vector<Vec2f>& texture_coords,
                        std::vector<Vec3f>& normal_coords, SDL_Surface* texture,
                        Uint32& pixel);

 protected:
  Mat4 viewport_;
  Mat4 projection_;
  Mat4 view_;
};

class BlinnPhongShader : public Shader {
 public:
  BlinnPhongShader(Mat4 viewport, Mat4 projection, Mat4 view);
  ~BlinnPhongShader();

  void Vertex(Vec3f& vertex, Vec3f& coord) override;
  bool Fragment(Vec3f& light, Vec3f& barycentric,
                std::vector<Vec2f>& texture_coords,
                std::vector<Vec3f>& normal_coords, SDL_Surface* texture,
                Uint32& pixel) override;
};

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_SHADER_H_
