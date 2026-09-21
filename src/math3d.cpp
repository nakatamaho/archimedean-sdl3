#include "archview/math3d.hpp"

namespace archview {

bool is_finite(const double value) noexcept
{
    return std::isfinite(value);
}

bool Vec3::finite() const noexcept
{
    return is_finite(x) && is_finite(y) && is_finite(z);
}

double Vec3::length_squared() const noexcept
{
    return x * x + y * y + z * z;
}

}  // namespace archview
