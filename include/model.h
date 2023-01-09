/**
 * @file model.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_MODEL_H_
#define SOFTWARE_RENDERER_INCLUDE_MODEL_H_
#include <string>
#include <vector>

#include "utils.hpp"

namespace swr {
class Model {
 public:
  Model(const std::string& filename);
  ~Model();

  int GetVerticesCount() const;
  int GetFacesCount() const;
  Vec3f GetVertex(int index) const;
  std::vector<int> GetFace(int index) const;

 private:
  std::vector<Vec3f> vertices_;
  std::vector<std::vector<int> > faces_;
};
}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_MODEL_H_
