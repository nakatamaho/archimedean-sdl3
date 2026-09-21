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
    if (!state.ico_motion || state.help_visible) {
        return false;
    }
    state.advance(1.0);
    if (!close(state.angle_radians, 30.0 * 3.141592653589793 / 180.0)) {
        return false;
    }
    const archview::Vec3 ico_rotated = state.orientation * archview::Vec3{1.0, 0.0, 0.0};
    if (std::abs(ico_rotated.z) <= 1.0e-12) {
        return false;
    }
    state.apply(archview::ViewerAction::TogglePause, 18);
    const double paused_angle = state.angle_radians;
    state.advance(1.0);
    if (!close(state.angle_radians, paused_angle)) {
        return false;
    }
    state.apply(archview::ViewerAction::TogglePause, 18);

    state.apply(archview::ViewerAction::NextSolid, 18);
    state.apply(archview::ViewerAction::PreviousSolid, 18);
    if (state.solid_index != 0) {
        return false;
    }
    state.apply(archview::ViewerAction::PreviousSolid, 18);
    if (state.solid_index != 17) {
        return false;
    }

    state.apply(archview::ViewerAction::AxisX, 18);
    if (state.ico_motion) {
        return false;
    }
    state.apply(archview::ViewerAction::AzimuthIncrease, 18);
    state.apply(archview::ViewerAction::ElevationDecrease, 18);
    if (!state.axis.finite() || std::abs(state.axis.length() - 1.0) > 1.0e-12) {
        return false;
    }

    state.apply(archview::ViewerAction::SpeedDecrease, 18);
    state.apply(archview::ViewerAction::Reverse, 18);
    if (!close(state.speed_degrees, -15.0)) {
        return false;
    }
    for (int index = 0; index < 30; ++index) {
        state.apply(archview::ViewerAction::SpeedDecrease, 18);
    }
    if (!close(state.speed_degrees, -360.0)) {
        return false;
    }
    for (int index = 0; index < 60; ++index) {
        state.apply(archview::ViewerAction::SpeedIncrease, 18);
    }
    if (!close(state.speed_degrees, 360.0)) {
        return false;
    }

    for (int index = 0; index < 40; ++index) {
        state.apply(archview::ViewerAction::ZoomOut, 18);
    }
    if (state.zoom < 0.5) {
        return false;
    }
    for (int index = 0; index < 80; ++index) {
        state.apply(archview::ViewerAction::ZoomIn, 18);
    }
    if (state.zoom > 2.0) {
        return false;
    }

    state.apply(archview::ViewerAction::ToggleWireframe, 18);
    state.apply(archview::ViewerAction::ToggleLighting, 18);
    if (!state.wireframe || state.lighting) {
        return false;
    }

    state.apply(archview::ViewerAction::ToggleHelp, 18);
    if (!state.help_visible) {
        return false;
    }
    state.apply(archview::ViewerAction::ToggleHelp, 18);
    if (state.help_visible) {
        return false;
    }
    state.apply(archview::ViewerAction::ToggleIcoMotion, 18);
    if (!state.ico_motion) {
        return false;
    }
    state.apply(archview::ViewerAction::ToggleIcoMotion, 18);
    if (state.ico_motion) {
        return false;
    }

    state.solid_index = 5;
    state.apply(archview::ViewerAction::ResetOrientation, 18);
    if (!close(state.angle_radians, 0.0)) {
        return false;
    }
    state.apply(archview::ViewerAction::ResetView, 18);
    return state.solid_index == 5 && close(state.speed_degrees, 30.0)
        && state.axis.x == 0.0 && state.axis.y == 1.0 && state.axis.z == 0.0
        && !state.paused && state.ico_motion && !state.help_visible
        && !state.wireframe && state.lighting && close(state.zoom, 1.0);
}
