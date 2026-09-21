#include "archview/polyhedron.hpp"

#include <iostream>

bool test_json_and_model();
bool test_math_and_projection();
bool test_renderer_colors();
bool test_viewer_state();

int main()
{
    if (!test_json_and_model()) {
        std::cerr << "JSON/model validation test failed\n";
        return 1;
    }
    if (!test_math_and_projection()) {
        std::cerr << "math/projection test failed\n";
        return 1;
    }
    if (!test_renderer_colors()) {
        std::cerr << "renderer color test failed\n";
        return 1;
    }
    if (!test_viewer_state()) {
        std::cerr << "viewer state test failed\n";
        return 1;
    }
    if (!archview::core_schema_version_supported(archview::kSchemaVersion)) {
        std::cerr << "schema version test failed\n";
        return 1;
    }
    std::cout << "PASS: archview_tests\n";
    return 0;
}
