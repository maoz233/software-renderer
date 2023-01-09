/**
 * @file utils.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_UTILS_H_
#define SOFTWARE_RENDERER_INCLUDE_UTILS_H_

#pragma warning(disable : 4201)

#include <iostream>

namespace swr {
template <typename T>
struct Vec2 {
  union {
    struct {
      T x, y;
    };
    struct {
      T u, v;
    };
    T raw[2];
  };

  Vec2();
  Vec2(T x_, T y_);

  inline Vec2<T> operator+(const Vec2<T>& v) const;
  inline Vec2<T> operator-(const Vec2<T>& v) const;
  inline Vec2<T> operator*(float f) const;

  template <typename>
  friend std::ostream& operator<<(std::ostream& out, Vec2<T>& v);
};

template <typename T>
Vec2<T>::Vec2() : x(0), y(0) {}

template <typename T>
Vec2<T>::Vec2(T x_, T y_) : x(x_), y(y_) {}

template <typename T>
Vec2<T> Vec2<T>::operator+(const Vec2<T>& v) const {
  return Vec2<T>(x + v.x, y + v.y);
}

template <typename T>
Vec2<T> Vec2<T>::operator-(const Vec2<T>& v) const {
  return Vec2<T>(x - v.x, y - v.y);
}

template <typename T>
Vec2<T> Vec2<T>::operator*(float f) const {
  return Vec2<T>(x * f, y * f);
}

template <typename T>
std::ostream& operator<<(std::ostream& out, Vec2<T>& v) {
  out << "(" << v.x << ", " << v.y << ")\n";

  return out;
}

typedef Vec2<int> Vec2i;
typedef Vec2<float> Vec2f;

template <typename T>
struct Vec3 {
  union {
    struct {
      T x, y, z;
    };
    struct {
      T ivert, iuv, inorm;
    };
    T raw[3];
  };

  Vec3();
  Vec3(T x_, T y_, T z_);

  // Cross product
  inline Vec3<T> operator^(const Vec3<T>& v) const;
  inline Vec3<T> operator+(const Vec3<T>& v) const;
  inline Vec3<T> operator-(const Vec3<T>& v) const;
  inline Vec3<T> operator*(float f) const;
  // Dot product
  inline T operator*(const Vec3<T>& v) const;

  template <typename>
  friend std::ostream& operator<<(std::ostream& out, Vec3<T>& v);
};

template <typename T>
Vec3<T>::Vec3() : x(0), y(0), z(0) {}

template <typename T>
Vec3<T>::Vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

template <typename T>
Vec3<T> Vec3<T>::operator^(const Vec3<T>& v) const {
  return Vec3<T>(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

template <typename T>
Vec3<T> Vec3<T>::operator+(const Vec3<T>& v) const {
  return Vec3<T>(x + v.x, y + v.y, z + v.z);
}

template <typename T>
Vec3<T> Vec3<T>::operator-(const Vec3<T>& v) const {
  return Vec3<T>(x - v.x, y - v.y, z - v.z);
}

template <typename T>
Vec3<T> Vec3<T>::operator*(float f) const {
  return Vec3<T>(x * f, y * f, z * f);
}

template <typename T>
T Vec3<T>::operator*(const Vec3<T>& v) const {
  return x * v.x + y * v.y + z * v.z;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, Vec3<T>& v) {
  out << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";

  return out;
}

typedef Vec3<int> Vec3i;
typedef Vec3<float> Vec3f;
}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_UTILS_H_
