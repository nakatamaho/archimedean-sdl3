#include "archview/polyhedron.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <numeric>
#include <set>
#include <sstream>

namespace archview {

namespace {

constexpr double kNormalizedEdgeTolerance = 1.0e-8;
constexpr double kPlanarityToleranceFactor = 1.0e-9;
constexpr double kCentroidToleranceFactor = 1.0e-12;
constexpr double kZeroTolerance = 1.0e-12;

struct ExpectedSolid {
    const char* id;
    const char* display_name;
    std::size_t vertices;
    std::size_t edges;
    std::size_t faces;
    std::map<std::size_t, std::size_t> face_histogram;
};

const std::array<ExpectedSolid, 13>& expected_solids()
{
    static const std::array<ExpectedSolid, 13> values{{
        {"truncated_tetrahedron", "Truncated tetrahedron", 12, 18, 8, {{3, 4}, {6, 4}}},
        {"cuboctahedron", "Cuboctahedron", 12, 24, 14, {{3, 8}, {4, 6}}},
        {"truncated_cube", "Truncated cube", 24, 36, 14, {{3, 8}, {8, 6}}},
        {"truncated_octahedron", "Truncated octahedron", 24, 36, 14, {{4, 6}, {6, 8}}},
        {"rhombicuboctahedron", "Rhombicuboctahedron", 24, 48, 26, {{3, 8}, {4, 18}}},
        {"truncated_cuboctahedron", "Truncated cuboctahedron", 48, 72, 26, {{4, 12}, {6, 8}, {8, 6}}},
        {"snub_cube", "Snub cube", 24, 60, 38, {{3, 32}, {4, 6}}},
        {"icosidodecahedron", "Icosidodecahedron", 30, 60, 32, {{3, 20}, {5, 12}}},
        {"truncated_dodecahedron", "Truncated dodecahedron", 60, 90, 32, {{3, 20}, {10, 12}}},
        {"truncated_icosahedron", "Truncated icosahedron", 60, 90, 32, {{5, 12}, {6, 20}}},
        {"rhombicosidodecahedron", "Rhombicosidodecahedron", 60, 120, 62, {{3, 20}, {4, 30}, {5, 12}}},
        {"truncated_icosidodecahedron", "Truncated icosidodecahedron", 120, 180, 62, {{4, 30}, {6, 20}, {10, 12}}},
        {"snub_dodecahedron", "Snub dodecahedron", 60, 150, 92, {{3, 80}, {5, 12}}},
    }};
    return values;
}

[[noreturn]] void fail(const std::string& context, const std::string& message)
{
    throw ModelError(context + ": " + message);
}

void require(bool condition, const std::string& context, const std::string& message)
{
    if (!condition) {
        fail(context, message);
    }
}

const nlohmann::json& require_member(
    const nlohmann::json& object,
    const char* key,
    const std::string& context
)
{
    require(object.is_object() && object.contains(key), context,
            std::string("missing member ") + key);
    return object.at(key);
}

std::pair<std::size_t, std::size_t> canonical_edge(
    const std::size_t left,
    const std::size_t right
)
{
    return std::minmax(left, right);
}

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

std::size_t parse_index(
    const nlohmann::json& value,
    const std::string& context,
    const std::size_t vertex_count
)
{
    require(value.is_number_integer(), context, "index must be an integer");
    const std::int64_t index = value.get<std::int64_t>();
    require(index >= 0, context, "index must not be negative");
    require(static_cast<std::uint64_t>(index) < vertex_count, context,
            "index is out of range");
    return static_cast<std::size_t>(index);
}

Vec3 parse_vertex(const nlohmann::json& value, const std::string& context)
{
    require(value.is_array() && value.size() == 3, context,
            "vertex must be an array of three numbers");
    require(std::all_of(
                value.begin(), value.end(),
                [](const nlohmann::json& component) {
                    return component.is_number();
                }
            ),
            context, "vertex coordinates must be numbers");
    Vec3 vertex{
        value.at(0).get<double>(),
        value.at(1).get<double>(),
        value.at(2).get<double>(),
    };
    require(vertex.finite(), context, "vertex contains a non-finite coordinate");
    return vertex;
}

std::size_t parse_count(const nlohmann::json& value, const std::string& context)
{
    require(value.is_number_unsigned() || value.is_number_integer(), context,
            "count must be an integer");
    if (value.is_number_integer()) {
        require(value.get<std::int64_t>() >= 0, context, "count must not be negative");
    }
    return value.get<std::size_t>();
}

SolidStatistics parse_statistics(
    const nlohmann::json& value,
    const std::string& context
)
{
    require(value.is_object(), context, "statistics must be an object");
    SolidStatistics statistics{
        parse_count(require_member(value, "vertices", context), context + ".vertices"),
        parse_count(require_member(value, "edges", context), context + ".edges"),
        parse_count(require_member(value, "faces", context), context + ".faces"),
        {},
    };
    const auto& histogram = require_member(value, "face_histogram", context);
    require(histogram.is_object(), context, "face_histogram must be an object");
    for (const auto& [key, count] : histogram.items()) {
        std::size_t side_count = 0;
        try {
            side_count = static_cast<std::size_t>(std::stoul(key));
        } catch (const std::exception&) {
            fail(context, "face_histogram key is not an integer");
        }
        statistics.face_histogram[side_count] =
            parse_count(count, context + ".face_histogram");
    }
    return statistics;
}

void validate_solid(const Solid& solid, const ExpectedSolid& expected)
{
    const std::string context = solid.id;
    require(solid.vertices.size() == expected.vertices, context,
            "unexpected vertex count");
    require(solid.faces.size() == expected.faces, context,
            "unexpected face count");
    require(solid.edges.size() == expected.edges, context,
            "unexpected edge count");

    double radius = 0.0;
    Vec3 centroid{};
    for (const Vec3& vertex : solid.vertices) {
        require(vertex.finite(), context, "non-finite vertex");
        centroid = centroid + vertex;
        radius = std::max(radius, vertex.length());
    }
    centroid = centroid * (1.0 / static_cast<double>(solid.vertices.size()));
    require(radius > kZeroTolerance, context, "zero characteristic radius");
    require(centroid.length() <= kCentroidToleranceFactor * radius, context,
            "vertex centroid is not at the origin");

    std::set<std::pair<std::size_t, std::size_t>> actual_edges;
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> incidence;
    std::map<std::size_t, std::size_t> histogram;
    for (std::size_t face_position = 0; face_position < solid.faces.size();
         ++face_position) {
        const Face& face = solid.faces[face_position];
        const std::string face_context =
            context + ".faces[" + std::to_string(face_position) + "]";
        require(face.indices.size() >= 3, face_context,
                "face has fewer than three vertices");
        std::set<std::size_t> distinct(face.indices.begin(), face.indices.end());
        require(distinct.size() == face.indices.size(), face_context,
                "face contains duplicate vertices");

        std::vector<std::pair<std::size_t, std::size_t>> face_edges;
        for (std::size_t index = 0; index < face.indices.size(); ++index) {
            const auto edge = canonical_edge(
                face.indices[index],
                face.indices[(index + 1) % face.indices.size()]
            );
            face_edges.push_back(edge);
            actual_edges.insert(edge);
            ++incidence[edge];
        }
        const std::set<std::pair<std::size_t, std::size_t>> unique_face_edges(
            face_edges.begin(), face_edges.end()
        );
        require(unique_face_edges.size() == face_edges.size(), face_context,
                "face contains a duplicate edge");

        const Vec3 raw_normal = face_normal(face, solid.vertices);
        require(raw_normal.length() > kZeroTolerance, face_context,
                "face normal is zero");
        const Vec3 normal = raw_normal.normalized();
        const Vec3 reference = solid.vertices[face.indices.front()];
        double planarity = 0.0;
        for (const std::size_t index : face.indices) {
            planarity = std::max(
                planarity,
                std::abs(dot(normal, solid.vertices[index] - reference))
            );
        }
        require(planarity <= kPlanarityToleranceFactor * radius, face_context,
                "face is not planar within tolerance");
        require(dot(normal, face_center(face, solid.vertices)) > kZeroTolerance,
                face_context, "face winding is not outward");
        ++histogram[face.indices.size()];
    }

    for (const auto& [edge, count] : incidence) {
        require(count == 2, context, "edge does not have incidence two");
        require(edge.first < solid.vertices.size()
                    && edge.second < solid.vertices.size(),
                context, "edge index is out of range");
    }
    require(actual_edges.size() == expected.edges, context,
            "topology edge count mismatch");
    require(solid.vertices.size() - actual_edges.size() + solid.faces.size() == 2,
            context, "Euler characteristic is not two");
    require(histogram == expected.face_histogram, context,
            "face histogram mismatch");

    const std::vector<std::pair<std::size_t, std::size_t>> sorted_actual(
        actual_edges.begin(), actual_edges.end()
    );
    require(solid.edges == sorted_actual, context,
            "explicit edges do not match polygon topology");

    std::vector<double> lengths;
    lengths.reserve(actual_edges.size());
    for (const auto& [left, right] : actual_edges) {
        lengths.push_back((solid.vertices[left] - solid.vertices[right]).length());
    }
    const double mean_edge =
        std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
    const auto [minimum, maximum] =
        std::minmax_element(lengths.begin(), lengths.end());
    const double spread = (*maximum - *minimum) / mean_edge;
    require(std::abs(mean_edge - 1.0) <= kNormalizedEdgeTolerance, context,
            "mean edge length is not one");
    require(spread <= kNormalizedEdgeTolerance, context,
            "normalized edge spread exceeds tolerance");

    require(solid.statistics.vertices == solid.vertices.size(), context,
            "statistics vertex count mismatch");
    require(solid.statistics.edges == actual_edges.size(), context,
            "statistics edge count mismatch");
    require(solid.statistics.faces == solid.faces.size(), context,
            "statistics face count mismatch");
    require(solid.statistics.face_histogram == histogram, context,
            "statistics face histogram mismatch");
}

Solid parse_solid(const nlohmann::json& value, const std::size_t position)
{
    const std::string context = "solids[" + std::to_string(position) + "]";
    require(value.is_object(), context, "solid must be an object");
    const auto& id_value = require_member(value, "id", context);
    require(id_value.is_string(), context, "id must be a string");
    const std::string id = id_value.get<std::string>();
    const auto expected_iterator = std::find_if(
        expected_solids().begin(),
        expected_solids().end(),
        [&](const ExpectedSolid& expected) { return id == expected.id; }
    );
    require(expected_iterator != expected_solids().end(), context,
            "unsupported solid id");
    const std::string solid_context = id;
    const auto& display_name = require_member(value, "display_name", solid_context);
    require(display_name.is_string()
                && display_name.get<std::string>() == expected_iterator->display_name,
            solid_context, "display_name mismatch");

    Solid solid{id, display_name.get<std::string>(), {}, {}, {}, {}};
    const auto& vertices = require_member(value, "vertices", solid_context);
    require(vertices.is_array(), solid_context, "vertices must be an array");
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        solid.vertices.push_back(parse_vertex(
            vertices.at(index),
            solid_context + ".vertices[" + std::to_string(index) + "]"
        ));
    }

    const auto& faces = require_member(value, "faces", solid_context);
    require(faces.is_array(), solid_context, "faces must be an array");
    for (std::size_t face_position = 0; face_position < faces.size();
         ++face_position) {
        const auto& value_face = faces.at(face_position);
        const std::string face_context =
            solid_context + ".faces[" + std::to_string(face_position) + "]";
        require(value_face.is_array() && value_face.size() >= 3, face_context,
                "face must be a polygon");
        Face face;
        for (std::size_t index = 0; index < value_face.size(); ++index) {
            face.indices.push_back(parse_index(
                value_face.at(index),
                face_context + "[" + std::to_string(index) + "]",
                solid.vertices.size()
            ));
        }
        solid.faces.push_back(std::move(face));
    }

    const auto& edges = require_member(value, "edges", solid_context);
    require(edges.is_array(), solid_context, "edges must be an array");
    for (std::size_t edge_position = 0; edge_position < edges.size();
         ++edge_position) {
        const auto& value_edge = edges.at(edge_position);
        const std::string edge_context =
            solid_context + ".edges[" + std::to_string(edge_position) + "]";
        require(value_edge.is_array() && value_edge.size() == 2, edge_context,
                "edge must contain two indices");
        const auto left = parse_index(
            value_edge.at(0), edge_context + "[0]", solid.vertices.size()
        );
        const auto right = parse_index(
            value_edge.at(1), edge_context + "[1]", solid.vertices.size()
        );
        require(left < right, edge_context, "edge is not canonical");
        solid.edges.emplace_back(left, right);
    }
    require(std::is_sorted(solid.edges.begin(), solid.edges.end()), solid_context,
            "edges are not deterministically ordered");
    require(std::adjacent_find(solid.edges.begin(), solid.edges.end())
                == solid.edges.end(),
            solid_context, "duplicate explicit edge");
    solid.statistics = parse_statistics(
        require_member(value, "statistics", solid_context),
        solid_context + ".statistics"
    );
    validate_solid(solid, *expected_iterator);
    return solid;
}

}  // namespace

bool core_schema_version_supported(const std::int64_t version) noexcept
{
    return version == kSchemaVersion;
}

Model load_model_file(const std::string& path)
{
    std::ifstream stream(path);
    if (!stream) {
        throw ModelError("cannot open JSON file: " + path);
    }
    try {
        nlohmann::json document;
        stream >> document;
        return load_model_json(document);
    } catch (const nlohmann::json::exception& error) {
        throw ModelError("malformed JSON in " + path + ": " + error.what());
    }
}

Model load_model_json(const nlohmann::json& document)
{
    try {
        require(document.is_object(), "document", "top level must be an object");
        require_member(document, "schema_version", "document");
        const auto& schema = document.at("schema_version");
        require(schema.is_number_integer(), "document.schema_version",
                "schema version must be an integer");
        const auto schema_version = schema.get<std::int64_t>();
        require(core_schema_version_supported(schema_version),
                "document.schema_version", "unsupported schema version");

        require_member(document, "generator", "document");
        const auto& generator = document.at("generator");
        require(generator.is_object(), "document.generator",
                "generator must be an object");
        require_member(generator, "program", "document.generator");
        require_member(generator, "sage_version", "document.generator");
        const auto& program = generator.at("program");
        const auto& sage_version = generator.at("sage_version");
        require(program.is_string()
                    && program.get<std::string>() == "tools/generate_archimedean.py",
                "document.generator.program", "unexpected generator program");
        require(sage_version.is_string()
                    && !sage_version.get<std::string>().empty(),
                "document.generator.sage_version", "missing Sage version");

        require_member(document, "solids", "document");
        const auto& solids = document.at("solids");
        require(solids.is_array(), "document.solids", "solids must be an array");
        require(solids.size() == expected_solids().size(), "document.solids",
                "expected exactly 13 solids");

        Model model{schema_version, sage_version.get<std::string>(), {}};
        for (std::size_t position = 0; position < solids.size(); ++position) {
            const auto& value = solids.at(position);
            require(value.is_object(), "document.solids", "solid must be an object");
            require_member(value, "id", "document.solids");
            const auto& id = value.at("id");
            require(id.is_string(), "document.solids", "solid id must be a string");
            require(id.get<std::string>() == expected_solids()[position].id,
                    "document.solids", "solid order or membership mismatch");
            model.solids.push_back(parse_solid(value, position));
        }
        return model;
    } catch (const nlohmann::json::exception& error) {
        throw ModelError(std::string("invalid model JSON: ") + error.what());
    }
}

const Solid& find_solid(const Model& model, const std::string& id)
{
    const auto iterator = std::find_if(
        model.solids.begin(),
        model.solids.end(),
        [&](const Solid& solid) { return solid.id == id; }
    );
    if (iterator == model.solids.end()) {
        throw ModelError("requested solid is missing: " + id);
    }
    return *iterator;
}

}  // namespace archview
