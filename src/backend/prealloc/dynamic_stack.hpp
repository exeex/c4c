#pragma once

#include "module.hpp"

#include <string_view>

namespace c4c::backend::prepare {

[[nodiscard]] bool is_dynamic_alloca_call(std::string_view callee);

void populate_dynamic_stack_plan(PreparedBirModule& prepared);

}  // namespace c4c::backend::prepare
