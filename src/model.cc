/**
 * @file model.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "model.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils.h"

namespace swr {
Model::Model(const std::string& filename) : vertices_(), faces_() {
  std::clog << "----- Model::Model -----" << std::endl;

  std::ifstream in_file_stream;
  in_file_stream.open(filename, std::ifstream::in);
  if (in_file_stream.fail()) {
    throw std::runtime_error("----- Error::OPEN_FILE_FAILURE -----");
  }

  std::string line;
  while (!in_file_stream.eof()) {
    std::getline(in_file_stream, line);
    std::istringstream in_string_stream(line.c_str());
    char trash;
    if (!line.compare(0, 2, "v ")) {
      in_string_stream >> trash;

      // vertex
      Vec3f vertex{};
      for (int i = 0; i < 3; ++i) {
        in_string_stream >> vertex.raw[i];
      }

      vertices_.push_back(vertex);
    } else if (!line.compare(0, 2, "f ")) {
      in_string_stream >> trash;

      int vertex_index, texture_index, normal_index;
      // face
      std::vector<int> face{};
      std::vector<int> texture_indices{};
      std::vector<int> normal_indices{};
      while (in_string_stream >> vertex_index >> trash >> texture_index >>
             trash >> normal_index) {
        // wavefront obj: all indices start at 1, not 0
        --vertex_index;
        face.push_back(vertex_index);
        --texture_index;
        texture_indices.push_back(texture_index);
        --normal_index;
        normal_indices.push_back(normal_index);
      }

      faces_.push_back(face);
      texture_indices_.push_back(texture_indices);
      normal_indices_.push_back(normal_indices);
    } else if (!line.compare(0, 3, "vt ")) {
      in_string_stream >> trash >> trash;

      // texture coordinates
      Vec2f texture_coords{};
      for (int i = 0; i < 2; ++i) {
        in_string_stream >> texture_coords.raw[i];
      }

      texture_coords_.push_back(texture_coords);
      in_string_stream >> trash;
    } else if (!line.compare(0, 3, "vn ")) {
      in_string_stream >> trash >> trash;

      // texture coordinates
      Vec3f normal_coords{};
      for (int i = 0; i < 3; ++i) {
        in_string_stream >> normal_coords.raw[i];
      }

      normal_coords_.push_back(normal_coords);
      in_string_stream >> trash;
    }
  }

  std::clog << "----- Model: #Vertices: " << vertices_.size()
            << ", #Faces: " << faces_.size() << " -----" << std::endl;
}

Model::~Model() { std::clog << "----- Model::~Model -----" << std::endl; }

int Model::GetVerticesCount() const {
  return static_cast<int>(vertices_.size());
}

int Model::GetFacesCount() const { return static_cast<int>(faces_.size()); }

Vec3f Model::GetVertex(int index) const { return vertices_[index]; }

std::vector<int> Model::GetFace(int index) const { return faces_[index]; }

std::vector<int> Model::GetTextureIndices(int index) const {
  return texture_indices_[index];
}

Vec2f Model::GetTextureCoords(int index) const {
  return texture_coords_[index];
}

std::vector<int> Model::GetNormalIndices(int index) const {
  return normal_indices_[index];
}
Vec3f Model::GetNormalCoords(int index) const { return normal_coords_[index]; }
}  // namespace swr
