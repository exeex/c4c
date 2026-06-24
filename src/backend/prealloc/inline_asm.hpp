#pragma once

#include "module.hpp"

namespace c4c::backend::prepare {

void populate_inline_asm_register_group_overrides(PreparedBirModule& prepared);
void populate_inline_asm_carriers(PreparedBirModule& prepared);

}  // namespace c4c::backend::prepare
