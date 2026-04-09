#pragma once

#include "../assembler/mod.hpp"

#include <optional>
#include <string>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {
namespace bir {
struct Module;
}
}

namespace c4c::backend::aarch64 {

std::optional<std::string> try_emit_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::backend::bir::Module& module);
std::optional<std::string> try_emit_prepared_lir_module(
    const c4c::codegen::lir::LirModule& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::aarch64
