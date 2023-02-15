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

enum Matrix { MVP };

enum Vector { VERTEX, FRAGMENT, LIGHT, EYE, NORMAL, TANGENT, BITANGENT };

enum Texture {
  DIFFUSE_TEXTURE,
  NORMAL_TEXTURE,
  NORMAL_TANGENT_TEXTURE,
  SPECULAR_TEXTURE
};

class Shader {
 public:
  Shader() = default;
  virtual ~Shader() = default;

  void SetVec2i(Vec2i& vec);
  void SetVec3f(int name, Vec3f& vec);
  void SetMat4(int type, Mat4& mat);
  void SetTexture(int name, SDL_Surface* texture);

  virtual void Vertex(Vec3f& position);
  virtual void Fragment(Uint32& pixel);

 protected:
  Vec2i uv_;
  Vec3f eye_;
  Vec3f light_;
  Vec3f fragment_;
  Vec3f vertex_;
  Vec3f normal_;
  Vec3f tangent_;
  Vec3f bitangent_;
  Mat4 mvp_;
  SDL_Surface* diffuse_texture_;
  SDL_Surface* normal_texture_;
  SDL_Surface* normal_tangent_texture_;
  SDL_Surface* specular_texture_;
};

class PhongShader : public Shader {
 public:
  PhongShader() = default;
  virtual ~PhongShader() = default;

  void Fragment(Uint32& pixel) override;
};

class NormalMappingShader : public Shader {
 public:
  NormalMappingShader() = default;
  virtual ~NormalMappingShader() = default;

  void Fragment(Uint32& pixel) override;
};

Uint32 GetPixel(SDL_Surface* surface, int x, int y);

Vec3f Reflect(Vec3f& v, Vec3f& normal);

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_SHADER_H_
