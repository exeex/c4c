#include "backend.hpp"
#include "aarch64/assembler/mod.hpp"
#include "aarch64/codegen/emit.hpp"
#include "bir_printer.hpp"
#include "lowering/lir_to_bir.hpp"
#include "riscv/assembler/mod.hpp"
#include "riscv/codegen/emit.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "x86/assembler/mod.hpp"
#include "x86/codegen/peephole/peephole.hpp"
#include "x86/codegen/x86_codegen.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <array>
#include <optional>
#include <stdexcept>

namespace c4c::backend {

namespace {

struct NativePreparationConfig {
  RegAllocConfig regalloc;
  std::vector<PhysReg> callee_saved;
};

std::string finalize_x86_asm(std::string rendered, Target target) {
  if (target != Target::X86_64 && target != Target::I686) {
    return rendered;
  }
  return c4c::backend::x86::codegen::peephole::peephole_optimize(std::move(rendered));
}

std::string finalize_riscv_asm(std::string rendered, Target target) {
  if (target != Target::Riscv64) {
    return rendered;
  }
  return c4c::backend::riscv::codegen::peephole_optimize(std::move(rendered));
}

NativePreparationConfig build_native_preparation_config(Target target) {
  NativePreparationConfig config;
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      config.regalloc.available_regs = {{1}, {2}, {3}, {4}, {5}};
      config.regalloc.caller_saved_regs = {{10}, {11}, {12}, {13}, {14}, {15}};
      config.callee_saved = {{1}, {2}, {3}, {4}, {5}};
      return config;
    case Target::Aarch64: {
      constexpr std::array<PhysReg, 9> kAarch64CalleeSaved = {
          PhysReg{20}, PhysReg{21}, PhysReg{22}, PhysReg{23}, PhysReg{24},
          PhysReg{25}, PhysReg{26}, PhysReg{27}, PhysReg{28},
      };
      constexpr std::array<PhysReg, 2> kAarch64CallerSaved = {
          PhysReg{9},
          PhysReg{10},
      };
      config.regalloc.available_regs.assign(
          kAarch64CalleeSaved.begin(), kAarch64CalleeSaved.end());
      config.regalloc.caller_saved_regs.assign(
          kAarch64CallerSaved.begin(), kAarch64CallerSaved.end());
      config.callee_saved.assign(kAarch64CalleeSaved.begin(), kAarch64CalleeSaved.end());
      return config;
    }
    case Target::Riscv64:
      return config;
  }
  throw std::logic_error("unreachable backend target");
}

std::string emit_native_prepared_lir_module(const c4c::codegen::lir::LirModule& module,
                                            Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return finalize_x86_asm(c4c::backend::x86::emit_module(module), target);
    case Target::Aarch64:
      return c4c::backend::aarch64::emit_prepared_lir_module(module);
    case Target::Riscv64:
      return finalize_riscv_asm(
          c4c::backend::riscv::codegen::emit_prepared_lir_module(module), target);
  }
  throw std::logic_error("unreachable backend target");
}

std::optional<std::string> try_render_bir_module(const bir::Module& module, Target target) {
  if (target == Target::Riscv64) {
    return c4c::backend::bir::print(module);
  }
  std::optional<std::string> rendered;
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      rendered = finalize_x86_asm(c4c::backend::x86::emit_module(module), target);
      break;
    case Target::Aarch64:
      rendered = c4c::backend::aarch64::emit_direct_bir_module(module);
      break;
    case Target::Riscv64:
      throw std::logic_error("riscv64 BIR rendering is handled above");
  }
  return rendered;
}

std::string render_direct_bir_module(const bir::Module& module, Target target) {
  if (const auto rendered = try_render_bir_module(module, target); rendered.has_value()) {
    return *rendered;
  }
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return c4c::backend::x86::emit_module(module);
    case Target::Aarch64:
      return c4c::backend::aarch64::emit_module(module);
    case Target::Riscv64:
      return c4c::backend::bir::print(module);
  }
  throw std::logic_error("unreachable backend target");
}

Target resolve_public_lir_target(const c4c::codegen::lir::LirModule& module,
                                 Target public_target) {
  if (public_target != Target::X86_64) {
    return public_target;
  }

  auto target = target_from_triple(module.target_triple);
  if (target != Target::I686) {
    target = Target::X86_64;
  }
  return target;
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : module_(bir_module) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : module_(std::cref(lir_module)) {}

c4c::codegen::lir::LirModule prepare_lir_module_for_target(
    const c4c::codegen::lir::LirModule& module,
    Target target) {
  if (target == Target::Riscv64) {
    return module;
  }

  const auto config = build_native_preparation_config(target);
  return c4c::backend::stack_layout::rewrite_module_entry_allocas(
      module, config.regalloc, {}, config.callee_saved);
}

std::string emit_target_bir_module(const bir::Module& module, Target public_target) {
  if (const auto rendered = try_render_bir_module(module, public_target); rendered.has_value()) {
    return *rendered;
  }
  switch (public_target) {
    case Target::X86_64:
    case Target::I686:
      throw std::invalid_argument(
          "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
    case Target::Aarch64:
      throw std::invalid_argument(
          "aarch64 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
    case Target::Riscv64:
      return c4c::backend::bir::print(module);
  }
  throw std::logic_error("unreachable backend target");
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   Target public_target) {
  const auto target = resolve_public_lir_target(module, public_target);
  return emit_module(BackendModuleInput{module}, BackendOptions{.target = target});
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    Target public_target,
    const std::string& output_path) {
  const auto target = resolve_public_lir_target(module, public_target);
  const auto emitted = emit_module(BackendModuleInput{module}, BackendOptions{.target = target});

  switch (target) {
    case Target::X86_64:
    case Target::I686: {
      const auto assembled = c4c::backend::x86::assembler::assemble(
          c4c::backend::x86::assembler::AssembleRequest{
              .asm_text = emitted,
              .output_path = output_path,
          });
      return BackendAssembleResult{
          .staged_text = assembled.staged_text,
          .output_path = assembled.output_path,
          .object_emitted = assembled.object_emitted,
          .error = assembled.error,
      };
    }
    case Target::Aarch64: {
      const auto assembled = c4c::backend::aarch64::assembler::assemble(
          c4c::backend::aarch64::assembler::AssembleRequest{
              .asm_text = emitted,
              .output_path = output_path,
          });
      return BackendAssembleResult{
          .staged_text = assembled.staged_text,
          .output_path = assembled.output_path,
          .object_emitted = assembled.object_emitted,
      };
    }
    case Target::Riscv64: {
      const auto assembled = c4c::backend::riscv::assembler::assemble(
          c4c::backend::riscv::assembler::AssembleRequest{
              .asm_text = emitted,
              .output_path = output_path,
          });
      return BackendAssembleResult{
          .staged_text = assembled.staged_text,
          .output_path = assembled.output_path,
          .object_emitted = assembled.object_emitted,
          .error = assembled.error,
      };
    }
  }
  throw std::logic_error("unreachable backend target");
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    return render_direct_bir_module(input.bir_module(), options.target);
  }

  const auto& lir_module = input.lir_module();
  const auto prepared_lir_module =
      c4c::backend::prepare_lir_module_for_target(lir_module, options.target);
  if (options.target == Target::Riscv64) {
    return finalize_riscv_asm(
        c4c::backend::riscv::codegen::emit_prepared_lir_module(prepared_lir_module),
        options.target);
  }

  if (auto raw_bir_module = c4c::backend::try_lower_to_bir(lir_module);
      raw_bir_module.has_value()) {
    if (options.target == Target::Riscv64) {
      if (!lir_module.globals.empty() || !lir_module.string_pool.empty() ||
          !lir_module.extern_decls.empty()) {
        return c4c::codegen::lir::print_llvm(lir_module);
      }
    }
    if (const auto rendered = try_render_bir_module(*raw_bir_module, options.target);
        rendered.has_value()) {
      return *rendered;
    }
  }

  auto bir_module = c4c::backend::try_lower_to_bir(prepared_lir_module);
  if (!bir_module.has_value()) {
    return emit_native_prepared_lir_module(prepared_lir_module, options.target);
  }
  if (options.target == Target::Riscv64) {
    if (!prepared_lir_module.globals.empty() ||
        !prepared_lir_module.string_pool.empty() ||
        !prepared_lir_module.extern_decls.empty()) {
      return c4c::codegen::lir::print_llvm(prepared_lir_module);
    }
  }
  if (const auto rendered = try_render_bir_module(*bir_module, options.target);
      rendered.has_value()) {
    return *rendered;
  }
  return emit_native_prepared_lir_module(prepared_lir_module, options.target);
}

}  // namespace c4c::backend
