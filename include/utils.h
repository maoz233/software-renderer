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

#if _WIN32
#pragma warning(disable : 4201)
#endif
#if __APPLE__
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace swr {
const float PI = 3.14159f;

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
  inline T operator[](int index) const;

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
T Vec2<T>::operator[](int index) const {
  assert(index >= 0 && index < 2);

  return this->raw[index];
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
  inline T operator[](int index) const;
  // Dot product
  inline T operator*(const Vec3<T>& v) const;

  // Norm
  inline float Norm();
  // Normalize
  inline Vec3<T>& Normalize(T l = 1);

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
T Vec3<T>::operator[](int index) const {
  assert(index >= 0 && index < 3);

  return this->raw[index];
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
float Vec3<T>::Norm() {
  return std::sqrt(x * x + y * y + z * z);
}

template <typename T>
Vec3<T>& Vec3<T>::Normalize(T l) {
  *this = *this * (l / Norm());

  return *this;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, Vec3<T>& v) {
  out << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";

  return out;
}

typedef Vec3<int> Vec3i;
typedef Vec3<float> Vec3f;

template <int M, int N>
struct Mat {
  std::vector<std::vector<float> > m;
  int rows, cols;

  Mat<M, N>();

  std::vector<float>& operator[](int index);
  template <int O>
  Mat<M, N>& operator*(Mat<N, O>& mat) const;

  template <typename>
  friend std::ostream& operator<<(std::ostream& out, Mat<M, N>& mat);

  Mat<M, N>& Transpose() const;
  Mat<M, N>& Inverse() const;

  static Mat<M, N> Identity();
};

template <int M, int N>
Mat<M, N>::Mat()
    : m(std::vector<std::vector<float> >(M, std::vector<float>(N, 0.f))),
      rows(M),
      cols(N) {}

template <int M, int N>
std::vector<float>& Mat<M, N>::operator[](int index) {
  assert(index >= 0 && index < M);

  return this->m[index];
}

template <int M, int N>
template <int O>
Mat<M, N>& Mat<M, N>::operator*(Mat<N, O>& mat) const {
  Mat<M, O> product{};

  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < O; ++j) {
      for (int k = 0; k < N; ++k) {
        product[i][j] += this->m[i][k] * mat[k][i];
      }
    }
  }

  return mat;
}

template <int M, int N>
std::ostream& operator<<(std::ostream& out, Mat<M, N>& mat) {
  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < N; ++j) {
      out << mat.m[i][j] << ", ";
    }
    out << std::endl;
  }

  return out;
}

template <int M, int N>
Mat<M, N>& Mat<M, N>::Transpose() const {
  Mat<N, M> mat{};

  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < N; ++j) {
      mat[j][i] = this->m[i][j];
    }
  }

  return mat;
}

template <int M, int N>
Mat<M, N>& Mat<M, N>::Inverse() const {
  assert(M == N);

  Mat<M, N * 2> mat{};

  // Copy original matrix
  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < N; ++j) {
      mat[i][j] = this->m[i][j];
    }
  }
  // Add a row with each element is 1
  for (int i = 0; i < M; ++i) {
    mat[i][i + N] = 1.f;
  }
  // First pass
  for (int i = 0; i < M - 1; ++i) {
    // Normalize the first row
    for (int j = mat.cols - 1; j >= 0; --j) {
      mat[i][j] /= mat[i][i];
    }

    for (int k = i + 1; k < M; ++k) {
      float coefficient = mat[k][i];
      for (int j = 0; j < mat.cols; ++j) {
        mat[k][j] -= mat[i][j] * coefficient;
      }
    }
  }
  // Normalize the last row
  for (int j = mat.cols - 1; j >= 0; --j) {
    mat[M - 1][j] /= mat[M - 1][M - 1];
  }
  // Second pass
  for (int i = M - 1; i >= 0; --i) {
    for (int k = i - 1; k >= 0; --k) {
      float coefficient = mat[k][i];
      for (int j = 0; j < mat.cols; ++j) {
        mat[k][j] -= mat[i][j] * coefficient;
      }
    }
  }
  // Cut the identity matrix back
  Mat<M, N> truncated{};
  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < N; ++j) {
      truncated[i][j] = mat[i][j + N];
    }
  }

  return truncated;
}

template <int M, int N>
Mat<M, N> Mat<M, N>::Identity() {
  assert(M == N);

  Mat<M, N> E{};
  for (int i = 0; i < M; ++i) {
    for (int j = 0; j < N; ++j) {
      if (i == j) {
        E[i][j] = 1.f;
      } else {
        E[i][j] = 0.f;
      }
    }
  }

  return E;
}

typedef Mat<4, 4> Mat4;
typedef Mat<1, 4> Vec4;

Mat4& LookAt(Vec3f& origin, Vec3f& target);

Mat4& Project(float near, float far, float theta);
}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_UTILS_H_
