#include "legalize.hpp"

#include <type_traits>

namespace c4c::backend::prepare {

namespace {

bool should_promote_i1(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
    case Target::Aarch64:
    case Target::Riscv64:
      return true;
  }
  return false;
}

bir::TypeKind legalize_type(Target target, bir::TypeKind type) {
  if (should_promote_i1(target) && type == bir::TypeKind::I1) {
    return bir::TypeKind::I32;
  }
  return type;
}

void legalize_value(Target target, bir::Value& value) {
  const auto original_type = value.type;
  value.type = legalize_type(target, value.type);
  if (original_type == bir::TypeKind::I1 && value.type == bir::TypeKind::I32 &&
      value.kind == bir::Value::Kind::Immediate) {
    value.immediate = value.immediate != 0 ? 1 : 0;
    value.immediate_bits = value.immediate != 0 ? 1u : 0u;
  }
}

void legalize_module(Target target, bir::Module& module) {
  for (auto& global : module.globals) {
    global.type = legalize_type(target, global.type);
    if (global.initializer.has_value()) {
      legalize_value(target, *global.initializer);
    }
    for (auto& element : global.initializer_elements) {
      legalize_value(target, element);
    }
  }

  for (auto& function : module.functions) {
    function.return_type = legalize_type(target, function.return_type);
    for (auto& param : function.params) {
      param.type = legalize_type(target, param.type);
    }
    for (auto& slot : function.local_slots) {
      slot.type = legalize_type(target, slot.type);
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        std::visit(
            [&](auto& lowered) {
              using T = std::decay_t<decltype(lowered)>;
              if constexpr (std::is_same_v<T, bir::BinaryInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                lowered.operand_type = legalize_type(target, lowered.operand_type);
                legalize_value(target, lowered.lhs);
                legalize_value(target, lowered.rhs);
              } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                lowered.compare_type = legalize_type(target, lowered.compare_type);
                legalize_value(target, lowered.lhs);
                legalize_value(target, lowered.rhs);
                legalize_value(target, lowered.true_value);
                legalize_value(target, lowered.false_value);
              } else if constexpr (std::is_same_v<T, bir::CastInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                legalize_value(target, lowered.operand);
              } else if constexpr (std::is_same_v<T, bir::CallInst>) {
                lowered.return_type = legalize_type(target, lowered.return_type);
                if (lowered.result.has_value()) {
                  legalize_value(target, *lowered.result);
                }
                if (lowered.callee_value.has_value()) {
                  legalize_value(target, *lowered.callee_value);
                }
                for (auto& arg : lowered.args) {
                  legalize_value(target, arg);
                }
                for (auto& arg_type : lowered.arg_types) {
                  arg_type = legalize_type(target, arg_type);
                }
                if (lowered.result_abi.has_value()) {
                  lowered.result_abi->type = legalize_type(target, lowered.result_abi->type);
                }
              } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                                   std::is_same_v<T, bir::LoadGlobalInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                if (lowered.address.has_value()) {
                  legalize_value(target, lowered.address->base_value);
                }
              } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                                   std::is_same_v<T, bir::StoreGlobalInst>) {
                legalize_value(target, lowered.value);
                if (lowered.address.has_value()) {
                  legalize_value(target, lowered.address->base_value);
                }
              }
            },
            inst);
      }

      if (block.terminator.value.has_value()) {
        legalize_value(target, *block.terminator.value);
      }
      legalize_value(target, block.terminator.condition);
    }
  }
}

}  // namespace

void run_legalize(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("legalize");
  module.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "target-dependent legalization skeleton is wired; bool promotion and ABI/type legalization still need implementation",
  });
}

void run_legalize(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("legalize");
  legalize_module(module.target, module.module);

  module.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "bootstrap BIR legalize promoted i1 values to i32 for x86/i686/aarch64/riscv64",
  });
}

}  // namespace c4c::backend::prepare
