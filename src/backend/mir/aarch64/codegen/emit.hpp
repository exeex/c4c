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

// Terminal text emitters for --codegen asm-style output, analogous to the LIR
// printer route. Their returned assembly text may be validated by external
// clang/as smoke paths, but it is not normal input to backend encoder, object,
// or linker stages.
std::string emit_direct_bir_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::backend::bir::Module& module);
std::string emit_prepared_lir_module(const c4c::codegen::lir::LirModule& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);

// Legacy assemble-module staging around text emission. Future internal
// compile-through should lower machine instruction nodes to structured
// asm/encoding records and object records instead of parsing terminal .s text.
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::aarch64
