#include "archview/renderer.hpp"

#include <iostream>
#include <string>

namespace {

void print_help(const char* program)
{
    std::cout
        << "Usage: " << program << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --data PATH       Load an Archimedean JSON file\n"
        << "  --solid ID        Select a solid\n"
        << "  --speed DEG/S     Set angular speed\n"
        << "  --axis X,Y,Z      Set rotation axis\n"
        << "  --width PIXELS    Set window width\n"
        << "  --height PIXELS   Set window height\n"
        << "  --selftest        Run the headless self-test\n"
        << "  --help            Show this help\n";
}

}  // namespace

int main(int argc, char** argv)
{
    bool selftest = false;
    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (argument == "--help") {
            print_help(argv[0]);
            return 0;
        }
        if (argument == "--selftest") {
            selftest = true;
            continue;
        }
        if (argument == "--data" || argument == "--solid"
            || argument == "--speed" || argument == "--axis"
            || argument == "--width" || argument == "--height") {
            if (index + 1 >= argc) {
                std::cerr << "error: missing value for " << argument << "\n";
                return 2;
            }
            ++index;
            continue;
        }
        std::cerr << "error: unknown option " << argument << "\n";
        return 2;
    }

    if (selftest) {
        std::cout << "PASS: M1 headless executable skeleton\n";
        return 0;
    }

    const archview::Renderer renderer;
    std::cout << "archimedean_viewer skeleton; renderer available="
              << (renderer.skeleton_available() ? "yes" : "no") << "\n";
    return 0;
}
