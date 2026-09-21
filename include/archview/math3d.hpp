#pragma once

#include <cmath>

namespace archview {

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    [[nodiscard]] bool finite() const noexcept;
    [[nodiscard]] double length_squared() const noexcept;
};

[[nodiscard]] bool is_finite(double value) noexcept;

}  // namespace archview
