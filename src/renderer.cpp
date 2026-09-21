#include "archview/renderer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace archview {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kCameraDistance = 14.0;
constexpr double kVerticalFov = 45.0 * kPi / 180.0;
constexpr double kNearPlane = 0.1;
constexpr double kAmbient = 0.25;
constexpr double kDiffuse = 0.75;
constexpr double kSpeedStep = 15.0;
constexpr double kMinimumSpeed = -360.0;
constexpr double kMaximumSpeed = 360.0;
constexpr double kMinimumZoom = 0.5;
constexpr double kMaximumZoom = 2.0;
constexpr double kAxisEditRadians = 5.0 * kPi / 180.0;
constexpr double kTitleUpdateSeconds = 0.1;
const Vec3 kLightDirection{-0.45, -0.65, -1.0};

struct RenderFace {
    double depth{0.0};
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    std::vector<SDL_FPoint> outline;
};

Vec3 face_normal(const Face& face, const std::vector<Vec3>& vertices)
{
    Vec3 normal{};
    for (std::size_t index = 0; index < face.indices.size(); ++index) {
        normal = normal + cross(
            vertices[face.indices[index]],
            vertices[face.indices[(index + 1) % face.indices.size()]]
        );
    }
    return normal;
}

Vec3 face_center(const Face& face, const std::vector<Vec3>& vertices)
{
    Vec3 center{};
    for (const std::size_t index : face.indices) {
        center = center + vertices[index];
    }
    return center * (1.0 / static_cast<double>(face.indices.size()));
}

double clamp_unit(const double value) noexcept
{
    return std::max(0.0, std::min(1.0, value));
}

double safe_dot_unit(const Vec3& left, const Vec3& right) noexcept
{
    const double left_length = left.length();
    const double right_length = right.length();
    if (!left.finite() || !right.finite() || !is_finite(left_length)
        || !is_finite(right_length) || left_length <= 1.0e-12
        || right_length <= 1.0e-12) {
        return 0.0;
    }
    return dot(left, right) / (left_length * right_length);
}

bool build_faces(
    const Solid& solid,
    const Mat3& rotation,
    double width,
    double height,
    double camera_distance,
    std::vector<RenderFace>& output,
    std::string& error,
    const bool lighting_enabled
)
{
    std::vector<Vec3> view_vertices;
    view_vertices.reserve(solid.vertices.size());
    for (const Vec3& vertex : solid.vertices) {
        view_vertices.push_back(
            rotation * vertex + Vec3{0.0, 0.0, camera_distance}
        );
    }

    for (const Face& face : solid.faces) {
        const Vec3 normal = face_normal(face, view_vertices);
        const Vec3 center = face_center(face, view_vertices);
        if (normal.length() <= 1.0e-12 || dot(normal, center) >= 0.0) {
            continue;
        }

        std::vector<ProjectedPoint> projected;
        projected.reserve(face.indices.size());
        bool projectable = true;
        for (const std::size_t index : face.indices) {
            const auto point = perspective_project(
                view_vertices[index], width, height, kVerticalFov, kNearPlane
            );
            if (!point.has_value()) {
                projectable = false;
                break;
            }
            projected.push_back(*point);
        }
        if (!projectable) {
            continue;
        }

        RenderFace render_face;
        render_face.depth = center.z;
        const FaceColor face_color = shade_face(
            face.indices.size(), normal, kLightDirection, lighting_enabled
        );
        const SDL_FColor color{
            static_cast<float>(face_color.red),
            static_cast<float>(face_color.green),
            static_cast<float>(face_color.blue),
            static_cast<float>(face_color.alpha),
        };
        render_face.outline.reserve(projected.size() + 1);
        for (const ProjectedPoint& point : projected) {
            render_face.outline.push_back({
                static_cast<float>(point.x), static_cast<float>(point.y)
            });
        }
        render_face.outline.push_back(render_face.outline.front());
        for (std::size_t index = 1; index + 1 < projected.size(); ++index) {
            const int base = static_cast<int>(render_face.vertices.size());
            const auto add_vertex = [&](const ProjectedPoint& point) {
                render_face.vertices.push_back(SDL_Vertex{
                    SDL_FPoint{static_cast<float>(point.x), static_cast<float>(point.y)},
                    color,
                    SDL_FPoint{0.0f, 0.0f},
                });
            };
            add_vertex(projected[0]);
            add_vertex(projected[index]);
            add_vertex(projected[index + 1]);
            render_face.indices.push_back(base);
            render_face.indices.push_back(base + 1);
            render_face.indices.push_back(base + 2);
        }
        if (!render_face.vertices.empty()) {
            output.push_back(std::move(render_face));
        }
    }

    std::stable_sort(
        output.begin(),
        output.end(),
        [](const RenderFace& left, const RenderFace& right) {
            return left.depth > right.depth;
        }
    );
    if (output.empty()) {
        error = "renderer produced no visible faces";
        return false;
    }
    return true;
}

bool render_frame(
    SDL_Renderer* renderer,
    const Solid& solid,
    const Mat3& rotation,
    int width,
    int height,
    std::string& error,
    const double camera_distance,
    const bool lighting_enabled,
    const bool wireframe,
    const bool help_visible
)
{
    if (!SDL_SetRenderDrawColor(renderer, 12, 16, 24, SDL_ALPHA_OPAQUE)
        || !SDL_RenderClear(renderer)) {
        error = std::string("SDL clear failed: ") + SDL_GetError();
        return false;
    }

    std::vector<RenderFace> faces;
    if (!build_faces(
            solid,
            rotation,
            static_cast<double>(width),
            static_cast<double>(height),
            camera_distance,
            faces,
            error,
            lighting_enabled
        )) {
        return false;
    }
    for (const RenderFace& face : faces) {
        if (!SDL_RenderGeometry(
                renderer,
                nullptr,
                face.vertices.data(),
                static_cast<int>(face.vertices.size()),
                face.indices.data(),
                static_cast<int>(face.indices.size())
            )) {
            error = std::string("SDL_RenderGeometry failed: ") + SDL_GetError();
            return false;
        }
    }
    if (wireframe) {
        if (!SDL_SetRenderDrawColor(renderer, 8, 12, 20, SDL_ALPHA_OPAQUE)) {
            error = std::string("SDL_SetRenderDrawColor failed: ") + SDL_GetError();
            return false;
        }
        for (const RenderFace& face : faces) {
            if (!SDL_RenderLines(
                    renderer,
                    face.outline.data(),
                    static_cast<int>(face.outline.size())
                )) {
                error = std::string("SDL_RenderLines failed: ") + SDL_GetError();
                return false;
            }
        }
    }
    if (help_visible) {
        constexpr std::array<const char*, 15> kHelpLines{
            "Controls (H to close)",
            "I: toggle X11 ico-style motion/custom axis",
            "Space: pause/resume rotation",
            "N/PageDown: next solid",
            "P/PageUp: previous solid",
            "1/2/3: select X/Y/Z custom axis",
            "Arrow keys: edit custom axis",
            "[ / ]: decrease/increase speed",
            "Backspace: reverse rotation",
            "R: reset orientation",
            "W: toggle wireframe",
            "L: toggle Lambert lighting",
            "+/-: zoom in/out",
            "Home: reset view",
            "Esc: quit",
        };
        const float panel_x = 12.0f;
        const float panel_y = 12.0f;
        const float panel_width = std::max(
            1.0f,
            std::min(520.0f, static_cast<float>(width) - 24.0f)
        );
        const float panel_height = static_cast<float>(
            kHelpLines.size() * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 24
        );
        const SDL_FRect panel{
            panel_x, panel_y, panel_width, panel_height
        };
        if (!SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)
            || !SDL_SetRenderDrawColor(renderer, 4, 7, 12, 230)
            || !SDL_RenderFillRect(renderer, &panel)
            || !SDL_SetRenderDrawColor(renderer, 240, 244, 252, SDL_ALPHA_OPAQUE)) {
            error = std::string("SDL help overlay background failed: ")
                + SDL_GetError();
            return false;
        }
        float text_y = panel_y + 12.0f;
        for (const char* line : kHelpLines) {
            if (!SDL_RenderDebugText(renderer, panel_x + 12.0f, text_y, line)) {
                error = std::string("SDL help overlay text failed: ")
                    + SDL_GetError();
                return false;
            }
            text_y += static_cast<float>(SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);
        }
        if (!SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE)) {
            error = std::string("SDL help overlay blend reset failed: ")
                + SDL_GetError();
            return false;
        }
    }
    if (!SDL_RenderPresent(renderer)) {
        error = std::string("SDL_RenderPresent failed: ") + SDL_GetError();
        return false;
    }
    return true;
}

std::string sdl_error(const char* action)
{
    return std::string(action) + ": " + SDL_GetError();
}

bool action_for_key(const SDL_Keycode key, ViewerAction& action)
{
    switch (key) {
    case SDLK_ESCAPE:
        action = ViewerAction::Quit;
        return true;
    case SDLK_SPACE:
        action = ViewerAction::TogglePause;
        return true;
    case SDLK_H:
        action = ViewerAction::ToggleHelp;
        return true;
    case SDLK_I:
        action = ViewerAction::ToggleIcoMotion;
        return true;
    case SDLK_N:
    case SDLK_PAGEDOWN:
        action = ViewerAction::NextSolid;
        return true;
    case SDLK_P:
    case SDLK_PAGEUP:
        action = ViewerAction::PreviousSolid;
        return true;
    case SDLK_1:
        action = ViewerAction::AxisX;
        return true;
    case SDLK_2:
        action = ViewerAction::AxisY;
        return true;
    case SDLK_3:
        action = ViewerAction::AxisZ;
        return true;
    case SDLK_LEFT:
        action = ViewerAction::AzimuthDecrease;
        return true;
    case SDLK_RIGHT:
        action = ViewerAction::AzimuthIncrease;
        return true;
    case SDLK_DOWN:
        action = ViewerAction::ElevationDecrease;
        return true;
    case SDLK_UP:
        action = ViewerAction::ElevationIncrease;
        return true;
    case SDLK_LEFTBRACKET:
        action = ViewerAction::SpeedDecrease;
        return true;
    case SDLK_RIGHTBRACKET:
        action = ViewerAction::SpeedIncrease;
        return true;
    case SDLK_BACKSPACE:
        action = ViewerAction::Reverse;
        return true;
    case SDLK_R:
        action = ViewerAction::ResetOrientation;
        return true;
    case SDLK_W:
        action = ViewerAction::ToggleWireframe;
        return true;
    case SDLK_L:
        action = ViewerAction::ToggleLighting;
        return true;
    case SDLK_PLUS:
    case SDLK_EQUALS:
        action = ViewerAction::ZoomIn;
        return true;
    case SDLK_MINUS:
        action = ViewerAction::ZoomOut;
        return true;
    case SDLK_HOME:
        action = ViewerAction::ResetView;
        return true;
    default:
        return false;
    }
}

std::string window_title(const Solid& solid, const ViewerState& state)
{
    std::ostringstream title;
    title << solid.id << " | speed=" << std::fixed << std::setprecision(1)
          << state.speed_degrees << " deg/s | axis=("
          << std::setprecision(2) << state.axis.x << "," << state.axis.y
          << "," << state.axis.z << ") | "
          << "motion=" << (state.ico_motion ? "ico" : "axis") << " | "
          << (state.paused ? "paused" : "running");
    return title.str();
}

}  // namespace

FaceColor polygon_base_color(const std::size_t side_count) noexcept
{
    switch (side_count) {
    case 3:
        return {0.90, 0.30, 0.24, 1.0};
    case 4:
        return {0.24, 0.60, 0.95, 1.0};
    case 5:
        return {0.28, 0.78, 0.42, 1.0};
    case 6:
        return {0.92, 0.58, 0.18, 1.0};
    case 8:
        return {0.66, 0.38, 0.86, 1.0};
    case 10:
        return {0.16, 0.78, 0.78, 1.0};
    default:
        return {0.62, 0.64, 0.68, 1.0};
    }
}

double lambert_intensity(
    const Vec3& normal,
    const Vec3& light_direction,
    const bool lighting_enabled
) noexcept
{
    if (!lighting_enabled) {
        return 1.0;
    }
    const double cosine = std::max(0.0, safe_dot_unit(normal, light_direction));
    return clamp_unit(kAmbient + kDiffuse * cosine);
}

FaceColor shade_face(
    const std::size_t side_count,
    const Vec3& normal,
    const Vec3& light_direction,
    const bool lighting_enabled
) noexcept
{
    const FaceColor base = polygon_base_color(side_count);
    const double intensity = lambert_intensity(
        normal, light_direction, lighting_enabled
    );
    return {
        clamp_unit(base.red * intensity),
        clamp_unit(base.green * intensity),
        clamp_unit(base.blue * intensity),
        base.alpha,
    };
}

void ViewerState::advance(const double elapsed_seconds) noexcept
{
    if (paused || !is_finite(elapsed_seconds) || elapsed_seconds < 0.0
        || !is_finite(speed_degrees)) {
        return;
    }
    const double angle_delta =
        speed_degrees * kPi / 180.0 * elapsed_seconds;
    angle_radians += angle_delta;
    if (!is_finite(angle_radians)) {
        angle_radians = 0.0;
    } else {
        angle_radians = std::fmod(angle_radians, 2.0 * kPi);
    }
    if (ico_motion) {
        const Mat3 x_step = Mat3::rotation_axis_angle(
            {1.0, 0.0, 0.0}, angle_delta
        );
        const Mat3 y_step = Mat3::rotation_axis_angle(
            {0.0, 1.0, 0.0}, angle_delta
        );
        orientation = x_step * y_step * orientation;
    } else {
        orientation = Mat3::rotation_axis_angle(axis, angle_radians);
    }
}

void ViewerState::reset_orientation() noexcept
{
    angle_radians = 0.0;
    orientation = Mat3::identity();
}

void ViewerState::reset_view() noexcept
{
    angle_radians = 0.0;
    orientation = Mat3::identity();
    speed_degrees = 30.0;
    axis = {0.0, 1.0, 0.0};
    paused = false;
    ico_motion = true;
    wireframe = false;
    lighting = true;
    zoom = 1.0;
    quit = false;
}

void ViewerState::apply(
    const ViewerAction action,
    const std::size_t solid_count
)
{
    const auto rotate_axis = [this](const Vec3& edit_axis, const double radians) {
        axis = Mat3::rotation_axis_angle(edit_axis, radians) * axis;
        axis = axis.normalized();
        ico_motion = false;
        orientation = Mat3::rotation_axis_angle(axis, angle_radians);
    };
    const auto select_axis = [this](const Vec3 selected_axis) {
        axis = selected_axis;
        ico_motion = false;
        orientation = Mat3::rotation_axis_angle(axis, angle_radians);
    };

    switch (action) {
    case ViewerAction::Quit:
        quit = true;
        break;
    case ViewerAction::TogglePause:
        paused = !paused;
        break;
    case ViewerAction::ToggleHelp:
        help_visible = !help_visible;
        break;
    case ViewerAction::ToggleIcoMotion:
        ico_motion = !ico_motion;
        if (!ico_motion) {
            orientation = Mat3::rotation_axis_angle(axis, angle_radians);
        }
        break;
    case ViewerAction::NextSolid:
        if (solid_count > 0) {
            solid_index = (solid_index + 1) % solid_count;
        }
        break;
    case ViewerAction::PreviousSolid:
        if (solid_count > 0) {
            solid_index = (solid_index + solid_count - 1) % solid_count;
        }
        break;
    case ViewerAction::AxisX:
        select_axis({1.0, 0.0, 0.0});
        break;
    case ViewerAction::AxisY:
        select_axis({0.0, 1.0, 0.0});
        break;
    case ViewerAction::AxisZ:
        select_axis({0.0, 0.0, 1.0});
        break;
    case ViewerAction::AzimuthDecrease:
        rotate_axis({0.0, 1.0, 0.0}, -kAxisEditRadians);
        break;
    case ViewerAction::AzimuthIncrease:
        rotate_axis({0.0, 1.0, 0.0}, kAxisEditRadians);
        break;
    case ViewerAction::ElevationDecrease: {
        Vec3 tangent = cross({0.0, 1.0, 0.0}, axis);
        if (tangent.length() <= 1.0e-12) {
            tangent = {1.0, 0.0, 0.0};
        } else {
            tangent = tangent.normalized();
        }
        rotate_axis(tangent, -kAxisEditRadians);
        break;
    }
    case ViewerAction::ElevationIncrease: {
        Vec3 tangent = cross({0.0, 1.0, 0.0}, axis);
        if (tangent.length() <= 1.0e-12) {
            tangent = {1.0, 0.0, 0.0};
        } else {
            tangent = tangent.normalized();
        }
        rotate_axis(tangent, kAxisEditRadians);
        break;
    }
    case ViewerAction::SpeedDecrease:
        speed_degrees = std::max(kMinimumSpeed, speed_degrees - kSpeedStep);
        break;
    case ViewerAction::SpeedIncrease:
        speed_degrees = std::min(kMaximumSpeed, speed_degrees + kSpeedStep);
        break;
    case ViewerAction::Reverse:
        speed_degrees = -speed_degrees;
        break;
    case ViewerAction::ResetOrientation:
        reset_orientation();
        break;
    case ViewerAction::ToggleWireframe:
        wireframe = !wireframe;
        break;
    case ViewerAction::ToggleLighting:
        lighting = !lighting;
        break;
    case ViewerAction::ZoomIn:
        zoom = std::min(kMaximumZoom, zoom * 1.1);
        break;
    case ViewerAction::ZoomOut:
        zoom = std::max(kMinimumZoom, zoom / 1.1);
        break;
    case ViewerAction::ResetView:
        reset_view();
        break;
    }
}

bool Renderer::run(
    const Model& model,
    const std::string& solid_id,
    const int width,
    const int height,
    const Vec3& rotation_axis,
    const double speed_degrees,
    std::string& error,
    const bool lighting_enabled,
    const bool initial_ico_motion
 ) const
{
    if (width <= 0 || height <= 0) {
        error = "window dimensions must be positive";
        return false;
    }
    if (!rotation_axis.finite() || rotation_axis.length() <= 1.0e-12
        || !is_finite(speed_degrees)) {
        error = "renderer rotation parameters are invalid";
        return false;
    }
    if (model.solids.empty()) {
        error = "renderer model contains no solids";
        return false;
    }
    (void)find_solid(model, solid_id);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        error = sdl_error("SDL_Init failed");
        return false;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Archimedean SDL3 viewer",
        width,
        height,
        SDL_WINDOW_RESIZABLE
    );
    if (window == nullptr) {
        error = sdl_error("SDL_CreateWindow failed");
        SDL_Quit();
        return false;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        error = sdl_error("SDL_CreateRenderer failed");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    ViewerState state;
    state.speed_degrees = std::max(
        kMinimumSpeed, std::min(kMaximumSpeed, speed_degrees)
    );
    state.axis = rotation_axis.normalized();
    state.lighting = lighting_enabled;
    state.ico_motion = initial_ico_motion;
    const auto selected = std::find_if(
        model.solids.begin(),
        model.solids.end(),
        [&](const Solid& solid) { return solid.id == solid_id; }
    );
    state.solid_index = static_cast<std::size_t>(
        std::distance(model.solids.begin(), selected)
    );

    if (!SDL_SetWindowTitle(
            window, window_title(model.solids[state.solid_index], state).c_str()
        )) {
        error = sdl_error("SDL_SetWindowTitle failed");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    Uint64 previous_counter = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    bool success = true;
    double title_elapsed = kTitleUpdateSeconds;
    while (!state.quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.apply(ViewerAction::Quit, model.solids.size());
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                ViewerAction action;
                if (action_for_key(event.key.key, action)
                    && (action != ViewerAction::ToggleHelp || !event.key.repeat)) {
                    state.apply(action, model.solids.size());
                }
            }
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        const double elapsed = frequency == 0
            ? 0.0
            : static_cast<double>(now - previous_counter)
                / static_cast<double>(frequency);
        previous_counter = now;
        state.advance(elapsed);
        title_elapsed += elapsed;
        if (state.quit) {
            break;
        }

        int pixel_width = width;
        int pixel_height = height;
        if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height)
            || pixel_width <= 0 || pixel_height <= 0) {
            error = sdl_error("SDL_GetWindowSizeInPixels failed");
            success = false;
            break;
        }
        const double camera_distance = kCameraDistance / state.zoom;
        if (!render_frame(
                renderer,
                model.solids[state.solid_index],
                state.orientation,
                pixel_width,
                pixel_height,
                error,
                camera_distance,
                state.lighting,
                state.wireframe,
                state.help_visible
            )) {
            success = false;
            break;
        }
        if (title_elapsed >= kTitleUpdateSeconds) {
            if (!SDL_SetWindowTitle(
                    window,
                    window_title(model.solids[state.solid_index], state).c_str()
                )) {
                error = sdl_error("SDL_SetWindowTitle failed");
                success = false;
                break;
            }
            title_elapsed = 0.0;
        }
        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return success;
}

}  // namespace archview
