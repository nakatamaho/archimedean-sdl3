#pragma once

#include <cstdint>

namespace archview {

constexpr std::int64_t kSchemaVersion = 1;

[[nodiscard]] bool core_schema_version_supported(std::int64_t version) noexcept;

}  // namespace archview
