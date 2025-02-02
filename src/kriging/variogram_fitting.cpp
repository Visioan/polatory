// Copyright (c) 2016, GSI and The Polatory Authors.

#include <polatory/kriging/variogram_fitting.hpp>

#include <iostream>

#include <ceres/ceres.h>

#include <polatory/common/exception.hpp>

namespace polatory {
namespace kriging {

namespace {

struct residual {
  residual(model *model,
    index_t n_pairs, double distance, double gamma, const weight_function& weight_fn)
    : model_(model)
    , n_pairs_(n_pairs)
    , distance_(distance)
    , gamma_(gamma)
    , weight_fn_(weight_fn) {}

  bool operator()(const double *const *param_blocks, double *residuals) const {
    auto params = param_blocks[0];
    auto sill = params[0] + params[1];
    model_->set_parameters(std::vector<double>(params, params + model_->num_parameters()));
    auto model_gamma = sill - model_->rbf().evaluate_untransformed(distance_);
    residuals[0] = weight_fn_(n_pairs_, distance_, model_gamma) * (gamma_ - model_gamma);

    return true;
  }

private:
  model *model_;
  const index_t n_pairs_;
  const double distance_;
  const double gamma_;
  const weight_function& weight_fn_;
};

ceres::CostFunction* create_cost_function(model *model,
  index_t n_pairs, double distance, double gamma, const weight_function& weight_fn) {
  auto cost_fn = new ceres::DynamicNumericDiffCostFunction<residual>(
    new residual(model, n_pairs, distance, gamma, weight_fn));
  cost_fn->AddParameterBlock(model->num_parameters());
  cost_fn->SetNumResiduals(1);
  return cost_fn;
}

}  // namespace

variogram_fitting::variogram_fitting(const empirical_variogram& emp_variog, const model& model,
  const weight_function& weight_fn) {
  polatory::model model2(model);

  auto n_params = model2.num_parameters();
  params_ = model2.parameters();

  auto& bin_distance = emp_variog.bin_distance();
  auto& bin_gamma = emp_variog.bin_gamma();
  auto& bin_num_pairs = emp_variog.bin_num_pairs();
  auto n_bins = static_cast<index_t>(bin_num_pairs.size());

  if (n_bins < n_params)
    throw common::invalid_argument("n_bins >= n_params");

  ceres::Problem problem;
  for (index_t i = 0; i < n_bins; i++) {
    if (bin_num_pairs[i] == 0)
      continue;

    auto cost_fn = create_cost_function(&model2, bin_num_pairs[i], bin_distance[i], bin_gamma[i], weight_fn);
    problem.AddResidualBlock(cost_fn, nullptr, params_.data());
  }

  auto lower_bounds = model2.parameter_lower_bounds();
  auto upper_bounds = model2.parameter_upper_bounds();
  for (auto i = 0; i < n_params; i++) {
    problem.SetParameterLowerBound(params_.data(), i, lower_bounds[i]);
    problem.SetParameterUpperBound(params_.data(), i, upper_bounds[i]);
  }

  ceres::Solver::Options options;
  options.linear_solver_type = ceres::DENSE_QR;
  options.max_num_iterations = 32;

  ceres::Solver::Summary summary;
  Solve(options, &problem, &summary);
  std::cout << summary.BriefReport() << std::endl;
}

const std::vector<double>& variogram_fitting::parameters() const {
  return params_;
}

}  // namespace kriging
}  // namespace polatory
