#include "archview/embedded_model.hpp"
#include "archview/math3d.hpp"
#include "archview/polyhedron.hpp"
#include "archview/renderer.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

namespace {

struct Options {
    std::string data_path{"data/archimedean.json"};
    std::string solid_id{"truncated_icosahedron"};
    double speed_degrees{30.0};
    archview::Vec3 axis{0.0, 1.0, 0.0};
    bool ico_motion{true};
    int width{1000};
    int height{800};
    bool selftest{false};
    bool use_embedded_data{true};
};

void print_help(const char* program)
{
    std::cout
        << "Usage: " << program << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --data PATH       Load an external Archimedean JSON file\n"
        << "  --solid ID        Select a solid\n"
        << "  --speed DEG/S     Set angular speed\n"
        << "  --axis X,Y,Z      Set custom rotation axis (disables ico motion)\n"
        << "  --width PIXELS    Set window width\n"
        << "  --height PIXELS   Set window height\n"
        << "  --selftest        Run the headless self-test\n"
        << "  --help            Show this help\n";
}

bool parse_axis(const std::string& text, archview::Vec3& axis)
{
    std::stringstream stream(text);
    char comma1 = 0;
    char comma2 = 0;
    if (!(stream >> axis.x >> comma1 >> axis.y >> comma2 >> axis.z)
        || comma1 != ',' || comma2 != ',' || !stream.eof()) {
        return false;
    }
    return axis.finite() && axis.length() > 1.0e-12;
}

bool parse_options(int argc, char** argv, Options& options)
{
    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (argument == "--help") {
            print_help(argv[0]);
            return false;
        }
        if (argument == "--selftest") {
            options.selftest = true;
            continue;
        }
        if (index + 1 >= argc) {
            throw std::invalid_argument("missing value for " + argument);
        }
        const std::string value(argv[++index]);
        try {
            if (argument == "--data") {
                options.data_path = value;
                options.use_embedded_data = false;
            } else if (argument == "--solid") {
                options.solid_id = value;
            } else if (argument == "--speed") {
                options.speed_degrees = std::stod(value);
            } else if (argument == "--axis") {
                if (!parse_axis(value, options.axis)) {
                    throw std::invalid_argument("axis must have the form X,Y,Z");
                }
                options.ico_motion = false;
            } else if (argument == "--width") {
                options.width = std::stoi(value);
            } else if (argument == "--height") {
                options.height = std::stoi(value);
            } else {
                throw std::invalid_argument("unknown option " + argument);
            }
        } catch (const std::exception& error) {
            throw std::invalid_argument(
                "invalid value for " + argument + ": " + error.what()
            );
        }
    }
    if (!std::isfinite(options.speed_degrees)
        || options.width <= 0 || options.height <= 0) {
        throw std::invalid_argument("speed, width, and height must be finite and positive");
    }
    options.axis = options.axis.normalized();
    return true;
}

archview::Model load_selected_model(const Options& options)
{
    if (options.use_embedded_data) {
        return archview::load_model_text(
            archview::kEmbeddedModelJson,
            "embedded canonical model"
        );
    }
    return archview::load_model_file(options.data_path);
}

}  // namespace

int main(int argc, char** argv)
{
    try {
        Options options;
        if (!parse_options(argc, argv, options)) {
            return 0;
        }
        if (options.selftest) {
            const archview::Model model = load_selected_model(options);
            const archview::Solid& solid =
                archview::find_solid(model, options.solid_id);
            const archview::Mat3 rotation =
                archview::Mat3::rotation_axis_angle(options.axis, 0.5);
            const archview::Vec3 rotated = rotation * solid.vertices.front();
            const auto projected = archview::perspective_project(
                rotated + archview::Vec3{0.0, 0.0, 5.0},
                static_cast<double>(options.width),
                static_cast<double>(options.height),
                45.0 * 3.141592653589793 / 180.0,
                0.1
            );
            if (!projected.has_value()) {
                throw archview::ModelError("representative projection failed");
            }
            std::cout << "PASS: loaded " << model.solids.size()
                      << " solids; selected " << solid.id << "\n";
            return 0;
        }

        const archview::Model model = load_selected_model(options);
        (void)archview::find_solid(model, options.solid_id);
        const archview::Renderer renderer;
        std::string error;
        if (!renderer.run(
                model,
                options.solid_id,
                options.width,
                options.height,
                options.axis,
                options.speed_degrees,
                error,
                true,
                options.ico_motion
            )) {
            std::cerr << "error: " << error << "\n";
            return 1;
        }
        return 0;
    } catch (const archview::ModelError& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 2;
    }
}
