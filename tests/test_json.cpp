#include "archview/embedded_model.hpp"
#include "archview/polyhedron.hpp"

#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>

namespace {

const std::string data_path()
{
    return std::string(ARCHVIEW_SOURCE_DIR) + "/data/archimedean.json";
}

bool throws_model_error(const std::function<void()>& action)
{
    try {
        action();
    } catch (const archview::ModelError&) {
        return true;
    }
    return false;
}

}  // namespace

bool test_json_and_model()
{
    const archview::Model model = archview::load_model_file(data_path());
    if (model.schema_version != 1 || model.solids.size() != 18) {
        return false;
    }
    if (archview::find_solid(model, "truncated_icosahedron").faces.size() != 32) {
        return false;
    }
    if (!throws_model_error([&model] {
            (void)archview::find_solid(model, "not_a_solid");
        })) {
        return false;
    }

    std::ifstream stream(data_path());
    nlohmann::json document;
    stream >> document;
    document["schema_version"] = 99;
    if (!throws_model_error([&document] {
            (void)archview::load_model_json(document);
        })) {
        return false;
    }

    const archview::Model embedded = archview::load_model_text(
        archview::kEmbeddedModelJson,
        "embedded canonical model"
    );
    if (embedded.schema_version != 1 || embedded.solids.size() != 18) {
        return false;
    }
    if (archview::find_solid(embedded, "truncated_icosahedron").faces.size() != 32) {
        return false;
    }

    stream.clear();
    stream.seekg(0);
    stream >> document;
    document["solids"][0]["faces"][0][0] = 999999;
    if (!throws_model_error([&document] {
            (void)archview::load_model_json(document);
        })) {
        return false;
    }
    return true;
}
