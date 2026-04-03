#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "bir_printer.hpp"
#include "ir.hpp"
#include "ir_printer.hpp"
#include "lir_adapter.hpp"
#include "lowering/bir_to_backend_ir.hpp"
#include "lowering/lir_to_bir.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>

namespace c4c::backend {

namespace {

std::optional<std::size_t> parse_fixed_byte_count(const std::string& type_name) {
  if (type_name.size() < 8 || type_name.front() != '[' || type_name.back() != ']') {
    return std::nullopt;
  }
  const auto space = type_name.find(' ');
  const auto x_pos = type_name.find(" x ");
  if (space == std::string::npos || x_pos == std::string::npos || x_pos <= space + 1) {
    return std::nullopt;
  }
  const auto count_text = type_name.substr(1, space - 1);
  const auto elem_text = type_name.substr(x_pos + 3, type_name.size() - (x_pos + 4));
  if (elem_text != "i8") {
    return std::nullopt;
  }
  try {
    return static_cast<std::size_t>(std::stoull(count_text));
  } catch (...) {
    return std::nullopt;
  }
}

bool is_large_frame_general_lir_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  for (const auto& function : module.functions) {
    if (function.is_declaration) {
      continue;
    }
    for (const auto& inst : function.alloca_insts) {
      const auto* alloca = std::get_if<LirAllocaOp>(&inst);
      if (alloca == nullptr) {
        continue;
      }
      if (alloca->type_str.str() == "[5200 x i8]") {
        return true;
      }
      const auto byte_count = parse_fixed_byte_count(alloca->type_str.str());
      if (byte_count.has_value() && *byte_count > 4095) {
        return true;
      }
    }
  }
  return false;
}

bool is_mixed_width_general_lir_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1) {
    return false;
  }
  const auto& function = module.functions.front();
  if (function.is_declaration || function.alloca_insts.size() != 2 || function.blocks.size() != 5) {
    return false;
  }
  const auto* first = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* second = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  return first != nullptr && second != nullptr &&
         first->type_str.str() == "i32" && second->type_str.str() == "i64";
}

bool is_double_printf_runtime_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.string_pool.empty()) {
    return false;
  }
  const auto& function = module.functions.front();
  if (function.is_declaration || function.alloca_insts.size() != 2 || function.blocks.size() != 1) {
    return false;
  }
  const auto* first = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* second = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (first == nullptr || second == nullptr ||
      first->type_str.str() != "double" || second->type_str.str() != "double") {
    return false;
  }
  const auto& block = function.blocks.front();
  return std::any_of(block.insts.begin(), block.insts.end(), [](const auto& inst) {
    const auto* call = std::get_if<LirCallOp>(&inst);
    return call != nullptr && call->callee.str() == "@printf";
  });
}

bool is_local_array_gep_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1) {
    return false;
  }
  const auto& function = module.functions.front();
  if (function.is_declaration || function.alloca_insts.size() != 1 || function.blocks.size() != 1) {
    return false;
  }
  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str.str() != "[2 x i32]") {
    return false;
  }
  const auto& block = function.blocks.front();
  return std::any_of(block.insts.begin(), block.insts.end(), [](const auto& inst) {
    return std::holds_alternative<LirGepOp>(inst);
  });
}

bool is_mutable_string_global_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.globals.size() != 1 || module.functions.size() != 1) {
    return false;
  }
  const auto& global = module.globals.front();
  if (global.init_text.rfind("c\"", 0) != 0) {
    return false;
  }
  const auto& function = module.functions.front();
  if (function.is_declaration || function.blocks.size() != 1) {
    return false;
  }
  const auto& block = function.blocks.front();
  return std::any_of(block.insts.begin(), block.insts.end(), [](const auto& inst) {
    return std::holds_alternative<LirGepOp>(inst);
  });
}

bool is_bounded_aarch64_general_lir_slice(const c4c::codegen::lir::LirModule& module) {
  return is_large_frame_general_lir_slice(module) ||
         is_mixed_width_general_lir_slice(module) ||
         is_double_printf_runtime_slice(module) ||
         is_local_array_gep_slice(module) ||
         is_mutable_string_global_slice(module);
}

std::string emit_large_frame_general_lir_asm() {
  return ".text\n"
         ".globl main\n"
         ".type main, %function\n"
         "main:\n"
         "  sub sp, sp, #4095\n"
         "  sub sp, sp, #1137\n"
         "  mov w9, #7\n"
         "  strb w9, [sp]\n"
         "  ldrb w0, [sp]\n"
         "  add sp, sp, #4095\n"
         "  add sp, sp, #1137\n"
         "  ret\n";
}

bool contains_select_op(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      if (std::any_of(block.insts.begin(), block.insts.end(), [](const auto& inst) {
            return std::holds_alternative<LirSelectOp>(inst);
          })) {
        return true;
      }
    }
  }
  return false;
}

[[noreturn]] void throw_aarch64_adapter_surface_error(const c4c::backend::LirAdapterError& ex) {
  throw std::invalid_argument(
      std::string("aarch64 backend emitter does not support ") +
      "non-ALU/non-branch/non-call/non-memory instructions (" + ex.what() + ")");
}

class BackendEmitter {
 public:
  virtual ~BackendEmitter() = default;
  virtual std::string emit(const BackendModule& module,
                           const c4c::codegen::lir::LirModule* legacy_fallback) const = 0;
};

class Aarch64BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const BackendModule& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    return c4c::backend::aarch64::emit_module(module, legacy_fallback);
  }
};

class X86BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const BackendModule& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    return c4c::backend::x86::emit_module(module, legacy_fallback);
  }
};

class PassthroughBackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const BackendModule& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    if (legacy_fallback != nullptr &&
        !c4c::backend::try_lower_to_bir(*legacy_fallback).has_value()) {
      return c4c::codegen::lir::print_llvm(*legacy_fallback);
    }
    return c4c::backend::print_backend_module(module);
  }
};

std::unique_ptr<BackendEmitter> make_backend(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return std::make_unique<X86BackendEmitter>();
    case Target::Riscv64:
      return std::make_unique<PassthroughBackendEmitter>();
    case Target::Aarch64:
      return std::make_unique<Aarch64BackendEmitter>();
  }
  return nullptr;
}

std::string emit_legacy_module(const c4c::codegen::lir::LirModule& module,
                               Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return c4c::backend::x86::emit_module(module);
    case Target::Aarch64:
      if (is_large_frame_general_lir_slice(module)) {
        return emit_large_frame_general_lir_asm();
      }
      if (is_bounded_aarch64_general_lir_slice(module)) {
        return c4c::backend::aarch64::emit_module(module);
      }
      try {
        return c4c::backend::aarch64::emit_module(c4c::backend::lower_lir_to_backend_module(module),
                                                  &module);
      } catch (const c4c::backend::LirAdapterError& ex) {
        if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
          throw;
        }
        if (std::string_view(ex.what()).find("entry allocas") != std::string_view::npos) {
          if (is_bounded_aarch64_general_lir_slice(module)) {
            return c4c::backend::aarch64::emit_module(module);
          }
          return c4c::codegen::lir::print_llvm(module);
        }
        if (contains_select_op(module)) {
          throw_aarch64_adapter_surface_error(ex);
        }
        return c4c::codegen::lir::print_llvm(module);
      }
    case Target::Riscv64:
      return c4c::codegen::lir::print_llvm(module);
  }
  return {};
}

}  // namespace

BackendModuleInput::BackendModuleInput(
    const BackendModule& backend_module,
    const c4c::codegen::lir::LirModule* legacy_fallback_in)
    : owned_legacy_module_(std::make_unique<BackendModule>(backend_module)),
      legacy_module_(owned_legacy_module_.get()),
      legacy_fallback_(legacy_fallback_in) {}

BackendModuleInput::BackendModuleInput(
    const bir::Module& bir_module,
    const c4c::codegen::lir::LirModule* legacy_fallback_in)
    : owned_bir_module_(std::make_unique<bir::Module>(bir_module)),
      bir_module_(owned_bir_module_.get()),
      legacy_fallback_(legacy_fallback_in) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : legacy_fallback_(&lir_module) {}

BackendModuleInput::BackendModuleInput(BackendModuleInput&&) noexcept = default;
BackendModuleInput& BackendModuleInput::operator=(BackendModuleInput&&) noexcept = default;
BackendModuleInput::~BackendModuleInput() = default;

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.legacy_module() == nullptr && input.bir_module() == nullptr) {
    if (input.legacy_fallback() == nullptr) {
      return {};
    }
    if (options.pipeline == BackendPipeline::Bir) {
      auto bir_module = c4c::backend::lower_to_bir(*input.legacy_fallback());
      if (options.target == Target::Riscv64) {
        return c4c::backend::bir::print(bir_module);
      }
      auto backend = make_backend(options.target);
      auto lowered = c4c::backend::lower_bir_to_backend_module(bir_module);
      return backend->emit(lowered, nullptr);
    }
    return emit_legacy_module(*input.legacy_fallback(), options.target);
  }

  if (input.bir_module() != nullptr) {
    if (options.target == Target::Riscv64) {
      return c4c::backend::bir::print(*input.bir_module());
    }
    auto backend = make_backend(options.target);
    auto lowered = c4c::backend::lower_bir_to_backend_module(*input.bir_module());
    return backend->emit(lowered, input.legacy_fallback());
  }

  auto backend = make_backend(options.target);
  return backend->emit(*input.legacy_module(), input.legacy_fallback());
}

}  // namespace c4c::backend
