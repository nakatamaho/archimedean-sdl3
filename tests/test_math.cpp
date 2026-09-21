#include "archview/math3d.hpp"

bool test_math_skeleton()
{
    const archview::Vec3 value{3.0, 4.0, 0.0};
    return value.finite() && value.length_squared() == 25.0;
}
