/**
 * @file texture.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "texture.h"

#include <cstdint>

namespace swr {

Texture::Texture(int width, int height, uint8_t* data)
    : width_{width}, height_{height}, data_{data} {}

Texture::~Texture() { delete[] data_; }

int Texture::GetWidth() const { return width_; }

int Texture::GetHeight() const { return height_; }

uint8_t* Texture::GetData() const { return data_; }

}  // namespace swr
