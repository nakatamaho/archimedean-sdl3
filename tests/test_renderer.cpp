#include "archview/renderer.hpp"

#include <cmath>

namespace {

bool close(const double left, const double right)
{
    return std::abs(left - right) <= 1.0e-12;
}

}  // namespace

bool test_renderer_colors()
{
    const archview::FaceColor triangle = archview::polygon_base_color(3);
    const archview::FaceColor square = archview::polygon_base_color(4);
    if (close(triangle.red, square.red)
        && close(triangle.green, square.green)
        && close(triangle.blue, square.blue)) {
        return false;
    }

    const archview::Vec3 light{0.0, 0.0, 1.0};
    const double lit = archview::lambert_intensity(light, light, true);
    const double unlit = archview::lambert_intensity(light, light, false);
    const double shadowed = archview::lambert_intensity(
        {0.0, 0.0, -1.0}, light, true
    );
    if (!close(lit, 1.0) || !close(unlit, 1.0) || !close(shadowed, 0.25)) {
        return false;
    }

    const archview::FaceColor shaded = archview::shade_face(
        4, light, light, true
    );
    const archview::FaceColor unshaded = archview::shade_face(
        4, light, light, false
    );
    return close(shaded.red, unshaded.red)
        && close(shaded.green, unshaded.green)
        && close(shaded.blue, unshaded.blue)
        && close(shaded.alpha, 1.0)
        && archview::shade_face(4, {0.0, 0.0, -1.0}, light, true).blue
               < unshaded.blue;
}
