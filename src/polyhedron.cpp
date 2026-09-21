#include "archview/polyhedron.hpp"

namespace archview {

bool core_schema_version_supported(const std::int64_t version) noexcept
{
    return version == kSchemaVersion;
}

}  // namespace archview
