#pragma once

#include "../ir.hpp"
#include "../lir_adapter.hpp"

#include "../../codegen/lir/ir.hpp"

#include <optional>
#include <string>
#include <vector>

namespace c4c::backend {

std::optional<std::vector<std::string>> infer_extern_param_types(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirExternDecl& decl);

BackendFunction lower_extern_decl(const c4c::codegen::lir::LirModule& module,
                                  const c4c::codegen::lir::LirExternDecl& decl);

}  // namespace c4c::backend
