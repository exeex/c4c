#pragma once

#include <cstdint>

#include "text_id_table.hpp"

namespace c4c {

using StructNameId = uint32_t;

constexpr StructNameId kInvalidStructName = 0;

using StructNameTable = SemanticNameTable<StructNameId, kInvalidStructName>;

}  // namespace c4c
