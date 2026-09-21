#include "archview/renderer.hpp"

#include <cmath>

namespace {

bool close(const double left, const double right)
{
    return std::abs(left - right) <= 1.0e-12;
}

}  // namespace

bool test_viewer_state()
{
    archview::ViewerState state;
    state.advance(1.0);
    if (!close(state.angle_radians, 30.0 * 3.141592653589793 / 180.0)) {
        return false;
    }
    state.apply(archview::ViewerAction::TogglePause, 13);
    const double paused_angle = state.angle_radians;
    state.advance(1.0);
    if (!close(state.angle_radians, paused_angle)) {
        return false;
    }
    state.apply(archview::ViewerAction::TogglePause, 13);

    state.apply(archview::ViewerAction::NextSolid, 13);
    state.apply(archview::ViewerAction::PreviousSolid, 13);
    if (state.solid_index != 0) {
        return false;
    }
    state.apply(archview::ViewerAction::PreviousSolid, 13);
    if (state.solid_index != 12) {
        return false;
    }

    state.apply(archview::ViewerAction::AxisX, 13);
    state.apply(archview::ViewerAction::AzimuthIncrease, 13);
    state.apply(archview::ViewerAction::ElevationDecrease, 13);
    if (!state.axis.finite() || std::abs(state.axis.length() - 1.0) > 1.0e-12) {
        return false;
    }

    state.apply(archview::ViewerAction::SpeedDecrease, 13);
    state.apply(archview::ViewerAction::Reverse, 13);
    if (!close(state.speed_degrees, -15.0)) {
        return false;
    }
    for (int index = 0; index < 30; ++index) {
        state.apply(archview::ViewerAction::SpeedDecrease, 13);
    }
    if (!close(state.speed_degrees, -360.0)) {
        return false;
    }
    for (int index = 0; index < 60; ++index) {
        state.apply(archview::ViewerAction::SpeedIncrease, 13);
    }
    if (!close(state.speed_degrees, 360.0)) {
        return false;
    }

    for (int index = 0; index < 40; ++index) {
        state.apply(archview::ViewerAction::ZoomOut, 13);
    }
    if (state.zoom < 0.5) {
        return false;
    }
    for (int index = 0; index < 80; ++index) {
        state.apply(archview::ViewerAction::ZoomIn, 13);
    }
    if (state.zoom > 2.0) {
        return false;
    }

    state.apply(archview::ViewerAction::ToggleWireframe, 13);
    state.apply(archview::ViewerAction::ToggleLighting, 13);
    if (!state.wireframe || state.lighting) {
        return false;
    }

    state.solid_index = 5;
    state.apply(archview::ViewerAction::ResetOrientation, 13);
    if (!close(state.angle_radians, 0.0)) {
        return false;
    }
    state.apply(archview::ViewerAction::ResetView, 13);
    return state.solid_index == 5 && close(state.speed_degrees, 30.0)
        && state.axis.x == 0.0 && state.axis.y == 1.0 && state.axis.z == 0.0
        && !state.paused && !state.wireframe && state.lighting
        && close(state.zoom, 1.0);
}
