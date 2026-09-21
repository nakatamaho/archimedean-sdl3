#include "archview/polyhedron.hpp"

#include <iostream>

bool test_json_dependency();
bool test_math_skeleton();

int main()
{
    if (!test_json_dependency()) {
        std::cerr << "JSON dependency test failed\n";
        return 1;
    }
    if (!test_math_skeleton()) {
        std::cerr << "math skeleton test failed\n";
        return 1;
    }
    if (!archview::core_schema_version_supported(archview::kSchemaVersion)) {
        std::cerr << "schema version test failed\n";
        return 1;
    }
    std::cout << "PASS: archview_tests\n";
    return 0;
}
