// Copyright (c) 2016, GSI and The Polatory Authors.

#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <polatory/common/exception.hpp>
#include <polatory/geometry/point3d.hpp>
#include <polatory/isosurface/types.hpp>
#include <polatory/numeric/roundtrip_string.hpp>

namespace polatory {
namespace isosurface {

class surface {
public:
  using vertices_type = std::vector<geometry::point3d>;
  using faces_type = std::vector<face>;

  surface(const vertices_type& vertices, const faces_type& faces) {
    std::vector<vertex_index> vi_map(vertices.size(), -1);
    face face_to_add;

    for (auto& face : faces) {
      for (auto i = 0; i < 3; i++) {
        auto& vi = vi_map[face[i]];
        if (vi == -1) {
          vi = vertices_.size();
          vertices_.push_back(vertices[face[i]]);
        }
        face_to_add[i] = vi;
      }
      faces_.push_back(face_to_add);
    }
  }

  void export_obj(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (!ofs)
      throw common::io_error("Could not open file '" + filename + "'.");

    for (auto& v : vertices_) {
      ofs << "v "
          << numeric::to_string(v[0]) << ' '
          << numeric::to_string(v[1]) << ' '
          << numeric::to_string(v[2]) << '\n';
    }

    for (auto& f : faces_) {
      ofs << "f " << f[0] + 1 << ' ' << f[1] + 1 << ' ' << f[2] + 1 << '\n';
    }
  }

  const faces_type& faces() const {
    return faces_;
  }

  const vertices_type& vertices() const {
    return vertices_;
  }

private:
  vertices_type vertices_;
  faces_type faces_;
};

}  // namespace isosurface
}  // namespace polatory
