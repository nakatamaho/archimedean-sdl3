#include "archview/math3d.hpp"

bool test_math_and_projection()
{
    const archview::Vec3 value{3.0, 4.0, 0.0};
    const archview::Vec3 normalized = value.normalized();
    if (!value.finite() || value.length_squared() != 25.0
        || std::abs(normalized.length() - 1.0) > 1.0e-12) {
        return false;
    }
    const archview::Mat3 rotation =
        archview::Mat3::rotation_axis_angle({0.0, 0.0, 1.0}, 3.141592653589793 / 2.0);
    const archview::Vec3 rotated = rotation * archview::Vec3{1.0, 0.0, 0.0};
    if (std::abs(rotated.x) > 1.0e-12 || std::abs(rotated.y - 1.0) > 1.0e-12) {
        return false;
    }
    const archview::Mat3 composed =
        archview::Mat3::rotation_axis_angle(
            {1.0, 0.0, 0.0}, 3.141592653589793 / 2.0
        )
        * archview::Mat3::rotation_axis_angle(
            {0.0, 1.0, 0.0}, 3.141592653589793 / 2.0
        );
    const archview::Vec3 composed_vector = composed * archview::Vec3{0.0, 0.0, 1.0};
    if (std::abs(composed_vector.x - 1.0) > 1.0e-12
        || std::abs(composed_vector.y) > 1.0e-12
        || std::abs(composed_vector.z) > 1.0e-12) {
        return false;
    }
    const auto projected = archview::perspective_project(
        {0.0, 0.0, 5.0}, 1000.0, 800.0, 45.0 * 3.141592653589793 / 180.0, 0.1
    );
    if (!projected.has_value()
        || std::abs(projected->x - 500.0) > 1.0e-12
        || std::abs(projected->y - 400.0) > 1.0e-12) {
        return false;
    }
    return !archview::perspective_project(
        {0.0, 0.0, 0.05}, 1000.0, 800.0, 45.0 * 3.141592653589793 / 180.0, 0.1
    ).has_value();
}
