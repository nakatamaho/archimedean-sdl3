#include "archview/math3d.hpp"

#include <stdexcept>

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

double Vec3::length() const noexcept
{
    return std::sqrt(length_squared());
}

Vec3 Vec3::normalized() const
{
    const double magnitude = length();
    if (!finite() || magnitude <= 1.0e-12) {
        throw std::domain_error("cannot normalize a non-finite or zero vector");
    }
    return *this * (1.0 / magnitude);
}

double dot(const Vec3& left, const Vec3& right) noexcept
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3 cross(const Vec3& left, const Vec3& right) noexcept
{
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x,
    };
}

Vec3 operator+(const Vec3& left, const Vec3& right) noexcept
{
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3 operator-(const Vec3& left, const Vec3& right) noexcept
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3 operator*(const Vec3& value, const double scalar) noexcept
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

Vec3 operator*(const double scalar, const Vec3& value) noexcept
{
    return value * scalar;
}

Mat3 Mat3::identity() noexcept
{
    Mat3 result{};
    result.values = {{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};
    return result;
}

Mat3 Mat3::rotation_axis_angle(const Vec3& axis, const double radians)
{
    const Vec3 unit_axis = axis.normalized();
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const double one_minus_cosine = 1.0 - cosine;
    const double x = unit_axis.x;
    const double y = unit_axis.y;
    const double z = unit_axis.z;

    Mat3 result{};
    result.values = {{
        {
            cosine + x * x * one_minus_cosine,
            x * y * one_minus_cosine - z * sine,
            x * z * one_minus_cosine + y * sine,
        },
        {
            y * x * one_minus_cosine + z * sine,
            cosine + y * y * one_minus_cosine,
            y * z * one_minus_cosine - x * sine,
        },
        {
            z * x * one_minus_cosine - y * sine,
            z * y * one_minus_cosine + x * sine,
            cosine + z * z * one_minus_cosine,
        },
    }};
    return result;
}

Vec3 Mat3::operator*(const Vec3& value) const noexcept
{
    return {
        values[0][0] * value.x + values[0][1] * value.y + values[0][2] * value.z,
        values[1][0] * value.x + values[1][1] * value.y + values[1][2] * value.z,
        values[2][0] * value.x + values[2][1] * value.y + values[2][2] * value.z,
    };
}

std::optional<ProjectedPoint> perspective_project(
    const Vec3& view_point,
    const double width,
    const double height,
    const double vertical_fov_radians,
    const double near_plane
)
{
    if (!view_point.finite() || !is_finite(width) || !is_finite(height)
        || !is_finite(vertical_fov_radians) || !is_finite(near_plane)
        || width <= 0.0 || height <= 0.0 || near_plane <= 0.0
        || vertical_fov_radians <= 0.0 || vertical_fov_radians >= 3.141592653589793) {
        return std::nullopt;
    }
    if (view_point.z <= near_plane) {
        return std::nullopt;
    }

    const double tangent = std::tan(vertical_fov_radians * 0.5);
    if (!is_finite(tangent) || tangent <= 0.0) {
        return std::nullopt;
    }
    const double aspect = width / height;
    const double normalized_x = view_point.x / (view_point.z * tangent * aspect);
    const double normalized_y = view_point.y / (view_point.z * tangent);
    return ProjectedPoint{
        (normalized_x * 0.5 + 0.5) * width,
        (0.5 - normalized_y * 0.5) * height,
    };
}

}  // namespace archview
