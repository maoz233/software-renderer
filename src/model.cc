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

#include "utils.hpp"

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

      this->vertices_.push_back(vertex);
    } else if (!line.compare(0, 2, "f ")) {
      in_string_stream >> trash;

      int i_trash, index;
      in_string_stream >> i_trash;

      // face
      std::vector<int> face;
      while (in_string_stream >> index >> trash >> i_trash >> trash >>
             i_trash) {
        // wavefront obj: all indices start at 1, not 0
        --index;
        face.push_back(index);
      }

      this->faces_.push_back(face);
    }
  }

  std::clog << "----- Model: #Vertices: " << this->vertices_.size()
            << ", #Faces: " << this->faces_.size() << " -----" << std::endl;
}

Model::~Model() { std::clog << "----- Model::~Model -----" << std::endl; }

int Model::GetVerticesCount() const {
  return static_cast<int>(this->vertices_.size());
}

int Model::GetFacesCount() const {
  return static_cast<int>(this->faces_.size());
}

Vec3f Model::GetVertex(int index) const { return this->vertices_[index]; }

std::vector<int> Model::GetFace(int index) const { return this->faces_[index]; }
}  // namespace swr
