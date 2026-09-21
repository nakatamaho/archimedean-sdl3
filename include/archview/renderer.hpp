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

enum class ViewerAction {
    Quit,
    TogglePause,
    NextSolid,
    PreviousSolid,
    AxisX,
    AxisY,
    AxisZ,
    AzimuthDecrease,
    AzimuthIncrease,
    ElevationDecrease,
    ElevationIncrease,
    SpeedDecrease,
    SpeedIncrease,
    Reverse,
    ResetOrientation,
    ToggleWireframe,
    ToggleLighting,
    ZoomIn,
    ZoomOut,
    ResetView,
};

struct ViewerState {
    std::size_t solid_index{0};
    double angle_radians{0.0};
    double speed_degrees{30.0};
    Vec3 axis{0.0, 1.0, 0.0};
    bool paused{false};
    bool wireframe{false};
    bool lighting{true};
    double zoom{1.0};
    bool quit{false};

    void apply(ViewerAction action, std::size_t solid_count);
    void advance(double elapsed_seconds) noexcept;
    void reset_orientation() noexcept;
    void reset_view() noexcept;
};

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
