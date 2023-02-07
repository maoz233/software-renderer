/**
 * @file shader.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-02-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "shader.h"

#include <algorithm>

#include "SDL.h"

#if _WIN32
#pragma warning(disable : 4127)
#endif

namespace swr {

Uint32 GetPixel(SDL_Surface* surface, int x, int y) {
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

Shader::Shader(Mat4 viewport, Mat4 projection, Mat4 view)
    : viewport_(viewport), projection_(projection), view_(view) {
  std::clog << "----- Shader::Shader -----" << std::endl;
}

Shader::~Shader() { std::clog << "----- Shader::~Shader -----" << std::endl; }

void Shader::Vertex(Vec3f& vertex, Vec3f& coord) {
  Vec4 homo_vertex{};
  homo_vertex[0][0] = vertex.x;
  homo_vertex[1][0] = vertex.y;
  homo_vertex[2][0] = vertex.z;
  homo_vertex[3][0] = 1.f;

  Vec4 v = this->viewport_ * this->projection_ * this->view_ * homo_vertex;
  coord = Vec3f(v[0][0] / v[3][0], v[1][0] / v[3][0], v[2][0] / v[3][0]);
}

bool Shader::Fragment(Vec3f& light, Vec3f& barycentric,
                      std::vector<Vec2f>& texture_coords,
                      std::vector<Vec3f>& normal_coords, SDL_Surface* texture,
                      Uint32& pixel) {
  std::vector<float> intensities(3);
  for (int i = 0; i < 3; ++i) {
    Vec4 homo_normal{};
    homo_normal[0][0] = normal_coords[i].x;
    homo_normal[1][0] = normal_coords[i].y;
    homo_normal[2][0] = normal_coords[i].z;
    homo_normal[3][0] = 1.f;

    Vec4 n = this->view_ * homo_normal;
    Vec3f normal_coord(n[0][0] / n[3][0], n[1][0] / n[3][0], n[2][0] / n[3][0]);

    intensities[i] =
        std::max(normal_coord.Normalize() * light.Normalize(), 0.f);
  }

  // Interpolated intensity
  float intensity = intensities[0] * barycentric.x +
                    intensities[1] * barycentric.y +
                    intensities[2] * barycentric.z;

  if (intensity > 0.f) {
    // Interpolated texture coordinates
    int u = static_cast<int>(std::round((texture_coords[0].u * barycentric.x +
                                         texture_coords[1].u * barycentric.y +
                                         texture_coords[2].u * barycentric.z) *
                                        texture->w));
    int v = static_cast<int>(std::round((texture_coords[0].v * barycentric.x +
                                         texture_coords[1].v * barycentric.y +
                                         texture_coords[2].v * barycentric.z) *
                                        texture->h));

    // R, G, B channel
    SDL_PixelFormat* format = texture->format;
    Uint32 p = GetPixel(texture, u, v);
    Uint8 R = static_cast<Uint8>(
        (((p & format->Rmask) >> format->Rshift) << format->Rloss) * intensity);
    Uint8 G = static_cast<Uint8>(
        (((p & format->Gmask) >> format->Gshift) << format->Gloss) * intensity);
    Uint8 B = static_cast<Uint8>(
        (((p & format->Bmask) >> format->Bshift) << format->Bloss) * intensity);

    pixel = SDL_MapRGB(format, R, G, B);
  }

  return intensity > 0.f;
}

}  // namespace swr
