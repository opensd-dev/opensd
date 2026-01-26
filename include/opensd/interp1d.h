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

  double operator()(double xq) const {
    if (xq <= x.front()) return y.front();
    if (xq >= x.back())  return y.back();

    auto it = std::upper_bound(x.begin(), x.end(), xq);
    int i = std::distance(x.begin(), it) - 1;

    double t = (xq - x[i]) / (x[i+1] - x[i]);
    return y[i] + t * (y[i+1] - y[i]);
  }
};
