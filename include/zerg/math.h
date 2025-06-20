#pragma once

#include <cmath>

namespace zerg {

constexpr double epsilon = 1e-9;
constexpr double lp_epsilon = 1e-6;

template <typename T>
bool IsValidData(T value) {
    return std::isfinite(value) && !std::isnan(value);
}

template <typename T1, typename T2>
bool FloatEqual(T1 a, T2 b, double epsilon_ = lp_epsilon) {
    if (IsValidData(a) && IsValidData(b)) {
        return std::abs(a - b) < epsilon_;
    } else if (std::isnan(a) && std::isnan(b)) {
        return true;
    } else if (!std::isfinite(a) && !std::isfinite(b)) {
        return true;
    } else {
        return false;
    }
}

/**
 * round_up(1.3456, 2) -> 1.35
 */
inline double round_up(double value, int decimal_places) {
    const double multiplier = std::pow(10.0, decimal_places);
    return static_cast<double>(std::lround(value * multiplier + .5)) / multiplier;
}
}
