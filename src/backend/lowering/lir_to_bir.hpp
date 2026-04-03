#pragma once

#include "../bir.hpp"
#include "../../codegen/lir/ir.hpp"

#include <optional>

namespace c4c::backend {

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module);
bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend
