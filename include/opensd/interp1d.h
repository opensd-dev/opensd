#pragma once
#include <vector>
#include <algorithm>

class Interp1D {
public:
  std::vector<double> x, y;

  Interp1D() = default;
  Interp1D(const std::vector<double>& x_,
           const std::vector<double>& y_)
    : x(x_), y(y_) {}

  double operator()(double xq) const
  {
    const size_t n = x.size();

    if (n < 2) {
      throw std::runtime_error("Interp1D: need at least 2 points");
    }

    // Left extrapolation
    if (xq <= x.front()) {
      const double dx = x[1] - x[0];
      const double dy = y[1] - y[0];
      return y[0] + (xq - x[0]) * dy / dx;
    }

    // Right extrapolation
    if (xq >= x.back()) {
      const double dx = x[n-1] - x[n-2];
      const double dy = y[n-1] - y[n-2];
      return y[n-1] + (xq - x[n-1]) * dy / dx;
    }

    // Interior interpolation
    auto it = std::upper_bound(x.begin(), x.end(), xq);
    size_t i = std::distance(x.begin(), it) - 1;

    const double dx = x[i+1] - x[i];
    const double dy = y[i+1] - y[i];

    return y[i] + (xq - x[i]) * dy / dx;
  }
};
