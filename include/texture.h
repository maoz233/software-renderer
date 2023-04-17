/**
 * @file texture.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_TEXTURE_H_
#define SOFTWARE_RENDERER_INCLUDE_TEXTURE_H_

#include <cstdint>

namespace swr {

class Texture {
 public:
  Texture() = delete;
  Texture(int width, int height, uint8_t* data);
  ~Texture();

  int GetWidth() const;
  int GetHeight() const;
  uint8_t* GetData() const;

 private:
  int width_;
  int height_;
  uint8_t* data_;
};

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_TEXTURE_H_
