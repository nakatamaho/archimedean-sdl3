#include "archview/renderer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace archview {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kCameraDistance = 6.0;
constexpr double kVerticalFov = 45.0 * kPi / 180.0;
constexpr double kNearPlane = 0.1;
constexpr double kAmbient = 0.25;
constexpr double kDiffuse = 0.75;
const Vec3 kLightDirection{-0.45, -0.65, -1.0};

struct RenderFace {
    double depth{0.0};
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
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
    std::vector<RenderFace>& output,
    std::string& error,
    const bool lighting_enabled
)
{
    std::vector<Vec3> view_vertices;
    view_vertices.reserve(solid.vertices.size());
    for (const Vec3& vertex : solid.vertices) {
        view_vertices.push_back(
            rotation * vertex + Vec3{0.0, 0.0, kCameraDistance}
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
    const bool lighting_enabled
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

bool Renderer::run(
    const Model& model,
    const std::string& solid_id,
    const int width,
    const int height,
    const Vec3& rotation_axis,
    const double speed_degrees,
    std::string& error,
    const bool lighting_enabled
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
    const Solid& solid = find_solid(model, solid_id);
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

    const Vec3 axis = rotation_axis.normalized();
    const Uint64 start_counter = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    bool running = true;
    bool success = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN
                       && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        int pixel_width = width;
        int pixel_height = height;
        if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height)
            || pixel_width <= 0 || pixel_height <= 0) {
            error = sdl_error("SDL_GetWindowSizeInPixels failed");
            success = false;
            break;
        }
        const Uint64 now = SDL_GetPerformanceCounter();
        const double elapsed =
            frequency == 0
                ? 0.0
                : static_cast<double>(now - start_counter)
                      / static_cast<double>(frequency);
        const double angle =
            speed_degrees * kPi / 180.0 * elapsed;
        const Mat3 rotation = Mat3::rotation_axis_angle(axis, angle);
        if (!render_frame(
                renderer,
                solid,
                rotation,
                pixel_width,
                pixel_height,
                error,
                lighting_enabled
            )) {
            success = false;
            break;
        }
        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return success;
}

}  // namespace archview
