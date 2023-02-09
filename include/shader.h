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
#include <vector>

#include "SDL.h"
#include "utils.h"

namespace swr {

enum Matrix { VIEWPORT, PROJECTION, VIEW };

enum Vector { LIGHT, NORMAL, COORD };

enum Texture { DIFFUSE_TEXTURE, NORMAL_TEXTURE };

struct VertexUniform {
  Vec3f vertex;
  Mat4 viewport;
  Mat4 projection;
  Mat4 view;

  void SetVec3f(Vec3f& vec);
  void SetMat4(int name, Mat4& mat);
};

struct FragmentUniform {
  float intensity;
  Vec2i uv;
  Vec3f light;
  Vec3f normal;
  Vec3f fragment_coord;
  SDL_Surface* diffuse_texture;

  void SetVec2i(Vec2i& vec);
  void SetVec3f(int name, Vec3f& vec);
  void SetTexture(int name, SDL_Surface* texture);
};

class Shader {
 public:
  Shader() = default;
  virtual ~Shader() = default;

  virtual void Vertex(VertexUniform& uniform, Vec3f& position);
  virtual void Fragment(FragmentUniform& uniform, Uint32& pixel);

 protected:
  Mat4 viewport_;
  Mat4 projection_;
  Mat4 view_;
};

class BlinnPhongShader : public Shader {
 public:
  BlinnPhongShader() = default;
  virtual ~BlinnPhongShader() = default;

  void Fragment(FragmentUniform& uniform, Uint32& pixel) override;
};

Uint32 GetPixel(SDL_Surface* surface, int x, int y);

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_SHADER_H_
