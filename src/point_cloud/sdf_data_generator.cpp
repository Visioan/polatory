// Copyright (c) 2016, GSI and The Polatory Authors.

#include <polatory/point_cloud/sdf_data_generator.hpp>

#include <tuple>
#include <vector>

#include <polatory/common/exception.hpp>
#include <polatory/point_cloud/kdtree.hpp>

namespace polatory {
namespace point_cloud {

sdf_data_generator::sdf_data_generator(
  const geometry::points3d& points,
  const geometry::vectors3d& normals,
  double min_distance,
  double max_distance,
  double multiplication)
  : points_(points)
  , normals_(normals) {
  if (points.rows() != normals.rows())
    throw common::invalid_argument("points.rows() == normals.rows()");

  if (min_distance > max_distance)
    throw common::invalid_argument("min_distance <= max_distance");

  if (multiplication <= 1.0 || multiplication > 3.0)
    throw common::invalid_argument("1.0 < ratio <= 3.0");

  kdtree tree(points, true);

  std::vector<index_t> nn_indices;
  std::vector<double> nn_distances;

  auto n_points = static_cast<index_t>(points.rows());
  auto n_reduced_points = static_cast<index_t>(((multiplication - 1.0) / 2.0) * n_points);
  auto n_max_sdf_points = n_points + 2 * n_reduced_points;
  auto n_sdf_points = n_points;

  sdf_points_ = geometry::points3d(n_max_sdf_points, 3);
  sdf_points_.topRows(n_points) = points_;
  sdf_values_ = common::valuesd::Zero(n_max_sdf_points);

  for (index_t i = 0; i < n_reduced_points; i++) {
    auto p = points.row(i);
    auto n = normals.row(i);

    if (n == geometry::vector3d::Zero())
      continue;

    auto d = max_distance;
    geometry::point3d q = p + d * n;

    std::tie(nn_indices, nn_distances) = tree.knn_search(q, 1);
    auto i_nearest = nn_indices[0];

    while (i_nearest != i) {
      auto p_nearest = points.row(i_nearest);

      d = 0.99 * (p_nearest - p).norm() / 2.0;
      q = p + d * n;

      if (d < min_distance)
        break;

      std::tie(nn_indices, nn_distances) = tree.knn_search(q, 1);
      i_nearest = nn_indices[0];
    }

    if (d < min_distance)
      continue;

    sdf_points_.row(n_sdf_points) = q;
    sdf_values_(n_sdf_points) = d;
    n_sdf_points++;
  }

  for (index_t i = 0; i < n_reduced_points; i++) {
    auto p = points.row(i);
    auto n = normals.row(i);

    if (n == geometry::vector3d::Zero())
      continue;

    auto d = max_distance;
    geometry::point3d q = p - d * n;

    std::tie(nn_indices, nn_distances) = tree.knn_search(q, 1);
    auto i_nearest = nn_indices[0];

    while (i_nearest != i) {
      auto p_nearest = points.row(i_nearest);

      d = 0.99 * (p_nearest - p).norm() / 2.0;
      q = p - d * n;

      if (d < min_distance)
        break;

      std::tie(nn_indices, nn_distances) = tree.knn_search(q, 1);
      i_nearest = nn_indices[0];
    }

    if (d < min_distance)
      continue;

    sdf_points_.row(n_sdf_points) = q;
    sdf_values_(n_sdf_points) = -d;
    n_sdf_points++;
  }

  sdf_points_.conservativeResize(n_sdf_points, 3);
  sdf_values_.conservativeResize(n_sdf_points);
}

const geometry::points3d& sdf_data_generator::sdf_points() const {
  return sdf_points_;
}

const common::valuesd& sdf_data_generator::sdf_values() const {
  return sdf_values_;
}

}  // namespace point_cloud
}  // namespace polatory
