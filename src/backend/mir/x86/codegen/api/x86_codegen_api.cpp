#include "x86_codegen_api.hpp"

#include "../../assembler/mod.hpp"
#include "../module/module_emit.hpp"

namespace c4c::backend::x86::api {

std::string emit_module(const c4c::backend::bir::Module& module,
                        const c4c::TargetProfile& target_profile) {
  return c4c::backend::emit_target_bir_module(module, target_profile);
}

std::string emit_module(const c4c::codegen::lir::LirModule& module,
                        const c4c::TargetProfile& target_profile) {
  return c4c::backend::emit_target_lir_module(module, target_profile);
}

c4c::backend::BackendAssembleResult assemble_module(
    const c4c::codegen::lir::LirModule& module,
    const c4c::TargetProfile& target_profile,
    const std::string& output_path) {
  const auto emitted = emit_module(module, target_profile);
  const auto assembled = c4c::backend::x86::assembler::assemble(
      c4c::backend::x86::assembler::AssembleRequest{
          .asm_text = emitted,
          .output_path = output_path,
      });
  return c4c::backend::BackendAssembleResult{
      .staged_text = std::move(assembled.staged_text),
      .output_path = std::move(assembled.output_path),
      .object_emitted = assembled.object_emitted,
      .error = std::move(assembled.error),
  };
}

std::string emit_prepared_module(const c4c::backend::prepare::PreparedBirModule& module) {
  return c4c::backend::x86::module::emit_prepared_module_text(module);
}

}  // namespace c4c::backend::x86::api
