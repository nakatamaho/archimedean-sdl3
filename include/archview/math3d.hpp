#pragma once

#include <array>
#include <cmath>
#include <optional>

namespace archview {

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    [[nodiscard]] bool finite() const noexcept;
    [[nodiscard]] double length_squared() const noexcept;
    [[nodiscard]] double length() const noexcept;
    [[nodiscard]] Vec3 normalized() const;
};

[[nodiscard]] bool is_finite(double value) noexcept;
[[nodiscard]] double dot(const Vec3& left, const Vec3& right) noexcept;
[[nodiscard]] Vec3 cross(const Vec3& left, const Vec3& right) noexcept;
[[nodiscard]] Vec3 operator+(const Vec3& left, const Vec3& right) noexcept;
[[nodiscard]] Vec3 operator-(const Vec3& left, const Vec3& right) noexcept;
[[nodiscard]] Vec3 operator*(const Vec3& value, double scalar) noexcept;
[[nodiscard]] Vec3 operator*(double scalar, const Vec3& value) noexcept;

struct Mat3 {
    std::array<std::array<double, 3>, 3> values{};

    [[nodiscard]] static Mat3 identity() noexcept;
    [[nodiscard]] static Mat3 rotation_axis_angle(
        const Vec3& axis,
        double radians
    );
    [[nodiscard]] Vec3 operator*(const Vec3& value) const noexcept;
};

struct ProjectedPoint {
    double x{0.0};
    double y{0.0};
};

[[nodiscard]] std::optional<ProjectedPoint> perspective_project(
    const Vec3& view_point,
    double width,
    double height,
    double vertical_fov_radians,
    double near_plane
);

}  // namespace archview
