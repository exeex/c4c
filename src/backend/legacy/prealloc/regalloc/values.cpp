#include "values.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace c4c::backend::prepare::regalloc_detail {

bool is_f128_immediate_constant(const bir::Value& value) {
  return value.kind == bir::Value::Kind::Immediate &&
         value.type == bir::TypeKind::F128 &&
         value.f128_payload.has_value();
}

namespace {

[[nodiscard]] std::string f128_constant_prepared_name(const bir::Value::F128Payload& payload) {
  std::ostringstream out;
  out << "__f128.const." << std::hex << std::nouppercase << std::setfill('0')
      << std::setw(16) << payload.high_bits
      << std::setw(16) << payload.low_bits;
  return out.str();
}

void append_f128_constant_value(std::vector<PreparedRegallocValue>& values,
                                PreparedNameTables& names,
                                FunctionNameId function_name,
                                PreparedValueId& next_value_id,
                                std::unordered_map<std::string, PreparedValueId>& seen_constants,
                                const bir::Value& value) {
  if (!is_f128_immediate_constant(value)) {
    return;
  }
  const std::string value_name = f128_constant_prepared_name(*value.f128_payload);
  if (seen_constants.find(value_name) != seen_constants.end()) {
    return;
  }
  const ValueNameId value_name_id = names.value_names.intern(value_name);
  if (value_name_id == kInvalidValueName) {
    return;
  }
  const PreparedValueId value_id = next_value_id++;
  seen_constants.emplace(value_name, value_id);
  values.push_back(PreparedRegallocValue{
      .value_id = value_id,
      .stack_object_id = std::nullopt,
      .function_name = function_name,
      .value_name = value_name_id,
      .type = bir::TypeKind::F128,
      .constant_f128_payload = value.f128_payload,
      .value_kind = PreparedValueKind::Temporary,
      .register_class = PreparedRegisterClass::None,
      .register_group_width = 1,
      .allocation_status = PreparedAllocationStatus::Unallocated,
      .spillable = false,
      .requires_home_slot = false,
      .crosses_call = false,
      .priority = 0,
      .spill_weight = 0.0,
      .live_interval = std::nullopt,
      .assigned_register = std::nullopt,
      .assigned_stack_slot = std::nullopt,
      .spill_register_authority = std::nullopt,
  });
}

}  // namespace

void append_f128_constant_values_for_function(std::vector<PreparedRegallocValue>& values,
                                              PreparedNameTables& names,
                                              const bir::Function* function,
                                              FunctionNameId function_name,
                                              PreparedValueId& next_value_id) {
  if (function == nullptr) {
    return;
  }
  std::unordered_map<std::string, PreparedValueId> seen_constants;
  for (const auto& block : function->blocks) {
    for (const auto& inst : block.insts) {
      std::visit(
          [&](const auto& typed_inst) {
            using T = std::decay_t<decltype(typed_inst)>;
            auto append_value = [&](const bir::Value& value) {
              append_f128_constant_value(
                  values, names, function_name, next_value_id, seen_constants, value);
            };
            if constexpr (std::is_same_v<T, bir::BinaryInst>) {
              append_value(typed_inst.lhs);
              append_value(typed_inst.rhs);
            } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
              append_value(typed_inst.lhs);
              append_value(typed_inst.rhs);
              append_value(typed_inst.true_value);
              append_value(typed_inst.false_value);
            } else if constexpr (std::is_same_v<T, bir::CastInst>) {
              append_value(typed_inst.operand);
            } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
              for (const auto& incoming : typed_inst.incomings) {
                append_value(incoming.value);
              }
            } else if constexpr (std::is_same_v<T, bir::CallInst>) {
              if (typed_inst.callee_value.has_value()) {
                append_value(*typed_inst.callee_value);
              }
              for (const auto& arg : typed_inst.args) {
                append_value(arg);
              }
            } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst> ||
                                 std::is_same_v<T, bir::StoreLocalInst>) {
              append_value(typed_inst.value);
            }
          },
          inst);
    }
    if (block.terminator.kind == bir::TerminatorKind::Return &&
        block.terminator.value.has_value()) {
      append_f128_constant_value(
          values, names, function_name, next_value_id, seen_constants, *block.terminator.value);
    } else if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
      append_f128_constant_value(
          values, names, function_name, next_value_id, seen_constants, block.terminator.condition);
    }
  }
}

const PreparedRegallocValue* find_f128_constant_regalloc_value(
    const PreparedRegallocFunction& function,
    const bir::Value& value) {
  if (!is_f128_immediate_constant(value)) {
    return nullptr;
  }
  for (const auto& regalloc_value : function.values) {
    if (regalloc_value.type == bir::TypeKind::F128 &&
        regalloc_value.constant_f128_payload == value.f128_payload) {
      return &regalloc_value;
    }
  }
  return nullptr;
}

}  // namespace c4c::backend::prepare::regalloc_detail
