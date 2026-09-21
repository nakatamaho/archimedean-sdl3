#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "archview/math3d.hpp"

namespace archview {

constexpr std::int64_t kSchemaVersion = 1;

struct SolidStatistics {
    std::size_t vertices{0};
    std::size_t edges{0};
    std::size_t faces{0};
    std::map<std::size_t, std::size_t> face_histogram;
};

struct Face {
    std::vector<std::size_t> indices;
};

struct Solid {
    std::string id;
    std::string display_name;
    std::vector<Vec3> vertices;
    std::vector<Face> faces;
    std::vector<std::pair<std::size_t, std::size_t>> edges;
    SolidStatistics statistics;
};

struct Model {
    std::int64_t schema_version{0};
    std::string sage_version;
    std::vector<Solid> solids;
};

class ModelError : public std::runtime_error {
public:
    explicit ModelError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

[[nodiscard]] bool core_schema_version_supported(std::int64_t version) noexcept;
[[nodiscard]] Model load_model_file(const std::string& path);
[[nodiscard]] Model load_model_text(
    const std::string& text,
    const std::string& source_name
);
[[nodiscard]] Model load_model_json(const nlohmann::json& document);
[[nodiscard]] const Solid& find_solid(const Model& model, const std::string& id);

}  // namespace archview
