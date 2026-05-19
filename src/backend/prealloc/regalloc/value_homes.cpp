#include "value_homes.hpp"

#include "../control_flow.hpp"
#include "../names.hpp"
#include "../target_register_profile.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace c4c::backend::prepare::regalloc_detail {

PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedRegallocValue& value) {
  PreparedValueHome home{
      .value_id = value.value_id,
      .function_name = value.function_name,
      .value_name = value.value_name,
      .kind = PreparedValueHomeKind::None,
      .register_name = std::nullopt,
      .slot_id = std::nullopt,
      .offset_bytes = std::nullopt,
      .size_bytes = std::nullopt,
      .align_bytes = std::nullopt,
      .immediate_i32 = std::nullopt,
      .immediate_f128 = std::nullopt,
      .pointer_base_value_name = std::nullopt,
      .pointer_byte_delta = std::nullopt,
  };
  if (value.constant_f128_payload.has_value()) {
    home.kind = PreparedValueHomeKind::RematerializableImmediate;
    home.immediate_f128 = value.constant_f128_payload;
    return home;
  }
  if (function != nullptr && value.value_kind == PreparedValueKind::Parameter) {
    const std::string_view value_name = prepared_value_name(names, value.value_name);
    for (std::size_t param_index = 0; param_index < function->params.size(); ++param_index) {
      const auto& param = function->params[param_index];
      if (param.name != value_name || !param.abi.has_value() || param.is_varargs || param.is_sret ||
          param.is_byval) {
        continue;
      }
      if (function->is_variadic && value.assigned_stack_slot.has_value()) {
        home.kind = PreparedValueHomeKind::StackSlot;
        home.slot_id = value.assigned_stack_slot->slot_id;
        home.offset_bytes = value.assigned_stack_slot->offset_bytes;
        home.size_bytes = value.assigned_stack_slot->size_bytes;
        home.align_bytes = value.assigned_stack_slot->align_bytes;
        return home;
      }
      if (function->is_variadic && value.assigned_register.has_value()) {
        home.kind = PreparedValueHomeKind::Register;
        home.register_name = value.assigned_register->register_name;
        return home;
      }
      if (const auto register_name =
              call_arg_destination_register_name(target_profile, *param.abi, param_index);
          register_name.has_value()) {
        home.kind = PreparedValueHomeKind::Register;
        home.register_name = *register_name;
      }
      return home;
    }
  }
  if (function != nullptr && value.type == bir::TypeKind::I32) {
    std::unordered_map<std::string_view, const bir::BinaryInst*> named_binaries;
    std::unordered_map<std::string_view, const bir::LoadGlobalInst*> named_global_loads;
    for (const auto& block : function->blocks) {
      for (const auto& inst : block.insts) {
        if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
            binary != nullptr && binary->result.kind == bir::Value::Kind::Named &&
            !binary->result.name.empty()) {
          named_binaries.emplace(binary->result.name, binary);
        } else if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst);
                   load_global != nullptr &&
                   load_global->result.kind == bir::Value::Kind::Named &&
                   !load_global->result.name.empty()) {
          named_global_loads.emplace(load_global->result.name, load_global);
        }
      }
    }
    const bir::Value named_value =
        bir::Value::named(value.type, std::string(prepared_value_name(names, value.value_name)));
    if (const auto computed_value =
            classify_computed_value(names, named_value, *function, named_binaries, named_global_loads);
        computed_value.has_value() &&
        computed_value->base.kind == PreparedComputedBaseKind::ImmediateI32) {
      auto current_value = static_cast<std::int32_t>(computed_value->base.immediate);
      bool supported = true;
      for (const auto& operation : computed_value->operations) {
        const auto immediate = static_cast<std::int32_t>(operation.immediate);
        switch (operation.opcode) {
          case bir::BinaryOpcode::Add:
            current_value = static_cast<std::int32_t>(current_value + immediate);
            break;
          case bir::BinaryOpcode::Mul:
            current_value = static_cast<std::int32_t>(current_value * immediate);
            break;
          case bir::BinaryOpcode::And:
            current_value = static_cast<std::int32_t>(current_value & immediate);
            break;
          case bir::BinaryOpcode::Or:
            current_value = static_cast<std::int32_t>(current_value | immediate);
            break;
          case bir::BinaryOpcode::Xor:
            current_value = static_cast<std::int32_t>(current_value ^ immediate);
            break;
          case bir::BinaryOpcode::Sub:
            current_value = static_cast<std::int32_t>(current_value - immediate);
            break;
          case bir::BinaryOpcode::Shl:
            current_value =
                static_cast<std::int32_t>(static_cast<std::uint32_t>(current_value) << immediate);
            break;
          case bir::BinaryOpcode::LShr:
            current_value = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(current_value) >> immediate);
            break;
          case bir::BinaryOpcode::AShr:
            current_value = static_cast<std::int32_t>(current_value >> immediate);
            break;
          default:
            supported = false;
            break;
        }
        if (!supported) {
          break;
        }
      }
      if (supported) {
        home.kind = PreparedValueHomeKind::RematerializableImmediate;
        home.immediate_i32 = current_value;
        return home;
      }
    }
  }
  if (value.type == bir::TypeKind::Ptr) {
    if (const auto carrier_it = pointer_carriers.find(value.value_name);
        carrier_it != pointer_carriers.end() &&
        (carrier_it->second.base_value_name != value.value_name ||
         carrier_it->second.byte_delta != 0)) {
      home.kind = PreparedValueHomeKind::PointerBasePlusOffset;
      home.pointer_base_value_name = carrier_it->second.base_value_name;
      home.pointer_byte_delta = carrier_it->second.byte_delta;
      if (value.assigned_register.has_value()) {
        home.register_name = value.assigned_register->register_name;
      }
      if (value.assigned_stack_slot.has_value()) {
        home.slot_id = value.assigned_stack_slot->slot_id;
        home.offset_bytes = value.assigned_stack_slot->offset_bytes;
        home.size_bytes = value.assigned_stack_slot->size_bytes;
        home.align_bytes = value.assigned_stack_slot->align_bytes;
      }
      return home;
    }
  }
  if (value.assigned_register.has_value()) {
    home.kind = PreparedValueHomeKind::Register;
    home.register_name = value.assigned_register->register_name;
    return home;
  }
  if (value.assigned_stack_slot.has_value()) {
    home.kind = PreparedValueHomeKind::StackSlot;
    home.slot_id = value.assigned_stack_slot->slot_id;
    home.offset_bytes = value.assigned_stack_slot->offset_bytes;
    home.size_bytes = value.assigned_stack_slot->size_bytes;
    home.align_bytes = value.assigned_stack_slot->align_bytes;
    return home;
  }
  return home;
}

std::vector<PreparedValueHome> build_prepared_value_homes(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedAddressingFunction* function_addressing,
    const PreparedRegallocFunction& regalloc_function) {
  const auto pointer_carriers =
      function == nullptr ? PreparedPointerCarrierMap{}
                          : build_pointer_carrier_map(names, *function, function_addressing);
  std::vector<PreparedValueHome> value_homes;
  value_homes.reserve(regalloc_function.values.size());
  for (const auto& value : regalloc_function.values) {
    value_homes.push_back(
        classify_prepared_value_home(names, target_profile, function, pointer_carriers, value));
  }
  return value_homes;
}

}  // namespace c4c::backend::prepare::regalloc_detail
