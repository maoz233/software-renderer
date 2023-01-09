/**
 * @file utils.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "utils.h"

namespace swr {
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

}  // namespace swr
