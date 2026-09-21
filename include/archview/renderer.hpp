#pragma once

#include "archview/math3d.hpp"
#include "archview/polyhedron.hpp"

#include <string>

namespace archview {

class Renderer {
public:
    [[nodiscard]] bool run(
        const Model& model,
        const std::string& solid_id,
        int width,
        int height,
        const Vec3& rotation_axis,
        double speed_degrees,
        std::string& error
    ) const;
};

}  // namespace archview
