#pragma once

#include "archview/math3d.hpp"
#include "archview/polyhedron.hpp"

#include <cstddef>
#include <string>

namespace archview {

struct FaceColor {
    double red{0.0};
    double green{0.0};
    double blue{0.0};
    double alpha{1.0};
};

[[nodiscard]] FaceColor polygon_base_color(std::size_t side_count) noexcept;
[[nodiscard]] double lambert_intensity(
    const Vec3& normal,
    const Vec3& light_direction,
    bool lighting_enabled
) noexcept;
[[nodiscard]] FaceColor shade_face(
    std::size_t side_count,
    const Vec3& normal,
    const Vec3& light_direction,
    bool lighting_enabled
) noexcept;

class Renderer {
public:
    [[nodiscard]] bool run(
        const Model& model,
        const std::string& solid_id,
        int width,
        int height,
        const Vec3& rotation_axis,
        double speed_degrees,
        std::string& error,
        bool lighting_enabled = true
    ) const;
};

}  // namespace archview
