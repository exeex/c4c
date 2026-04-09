#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "bir_printer.hpp"
#include "lowering/lir_to_bir.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <array>
#include <string_view>
#include <stdexcept>

namespace c4c::backend {

namespace {

struct NativePreparationConfig {
  RegAllocConfig regalloc;
  std::vector<PhysReg> callee_saved;
};

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

std::string emit_native_bir_module(const bir::Module& module, Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return c4c::backend::x86::emit_module(module);
    case Target::Aarch64:
      return c4c::backend::aarch64::emit_module(module);
    case Target::Riscv64:
      throw std::logic_error("riscv64 native BIR emission is handled by the caller");
  }
  throw std::logic_error("unreachable backend target");
}

bool is_direct_bir_subset_error(const std::invalid_argument& ex) {
  return std::string_view(ex.what()).find("does not support this direct BIR module") !=
         std::string_view::npos;
}

std::string emit_native_prepared_lir_module(const c4c::codegen::lir::LirModule& module,
                                            Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return c4c::backend::x86::emit_prepared_lir_module(module);
    case Target::Aarch64:
      return c4c::backend::aarch64::emit_prepared_lir_module(module);
    case Target::Riscv64:
      return c4c::codegen::lir::print_llvm(module);
  }
  throw std::logic_error("unreachable backend target");
}

std::string render_bir_module(const bir::Module& module, Target target) {
  if (target == Target::Riscv64) {
    return c4c::backend::bir::print(module);
  }
  return emit_native_bir_module(module, target);
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

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    return render_bir_module(input.bir_module(), options.target);
  }

  const auto& lir_module = input.lir_module();
  const auto prepared_lir_module =
      c4c::backend::prepare_lir_module_for_target(lir_module, options.target);
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
  try {
    return render_bir_module(*bir_module, options.target);
  } catch (const std::invalid_argument& ex) {
    if (!is_direct_bir_subset_error(ex)) {
      throw;
    }
  }
  return emit_native_prepared_lir_module(prepared_lir_module, options.target);
}

}  // namespace c4c::backend
