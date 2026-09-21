#include <nlohmann/json.hpp>

bool test_json_dependency()
{
    const nlohmann::json value = {{"schema_version", 1}};
    return value.at("schema_version").get<int>() == 1;
}
