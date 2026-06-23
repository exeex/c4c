#include "prepared_call_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_scalar_emit.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/calls.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <vector>

namespace c4c::backend::riscv::codegen {
namespace {

std::optional<std::string_view> riscv_gpr_argument_register(std::size_t index) {
  switch (index) {
    case 0: return std::string_view{"a0"};
    case 1: return std::string_view{"a1"};
    case 2: return std::string_view{"a2"};
    case 3: return std::string_view{"a3"};
    case 4: return std::string_view{"a4"};
    case 5: return std::string_view{"a5"};
    case 6: return std::string_view{"a6"};
    case 7: return std::string_view{"a7"};
    default: return std::nullopt;
  }
}

struct ByvalPayloadField {
  std::size_t payload_offset = 0;
  std::size_t source_stack_offset = 0;
  std::size_t size_bytes = 0;
};

const c4c::backend::prepare::PreparedStackObject* stack_object_for_id(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    c4c::backend::prepare::PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

std::optional<std::size_t> aggregate_member_payload_offset(
    std::string_view aggregate_name,
    std::string_view object_name) {
  if (aggregate_name.empty() ||
      object_name.size() <= aggregate_name.size() + 1 ||
      object_name.substr(0, aggregate_name.size()) != aggregate_name ||
      object_name[aggregate_name.size()] != '.') {
    return std::nullopt;
  }
  const std::string_view suffix = object_name.substr(aggregate_name.size() + 1);
  std::size_t offset = 0;
  const auto* first = suffix.data();
  const auto* last = suffix.data() + suffix.size();
  const auto parsed = std::from_chars(first, last, offset);
  if (parsed.ec != std::errc{} || parsed.ptr != last) {
    return std::nullopt;
  }
  return offset;
}

std::optional<std::vector<ByvalPayloadField>> byval_payload_fields_for_argument(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::string_view aggregate_name,
    std::size_t payload_size) {
  if (payload_size == 0 || payload_size > 128 || aggregate_name.empty()) {
    return std::nullopt;
  }

  std::vector<ByvalPayloadField> fields;
  std::vector<bool> covered(payload_size, false);
  for (const auto& slot : prepared.stack_layout.frame_slots) {
    if (slot.function_name != function_name || slot.size_bytes == 0) {
      continue;
    }
    const auto* object = stack_object_for_id(prepared.stack_layout, slot.object_id);
    if (object == nullptr || object->function_name != function_name) {
      continue;
    }
    const std::string_view object_name =
        c4c::backend::prepare::prepared_stack_object_name(prepared.names, *object);
    const auto payload_offset =
        aggregate_member_payload_offset(aggregate_name, object_name);
    if (!payload_offset.has_value() ||
        *payload_offset > payload_size ||
        slot.size_bytes > payload_size - *payload_offset ||
        (slot.size_bytes != 1 && slot.size_bytes != 4 && slot.size_bytes != 8)) {
      continue;
    }
    for (std::size_t byte = 0; byte < slot.size_bytes; ++byte) {
      if (covered[*payload_offset + byte]) {
        return std::nullopt;
      }
      covered[*payload_offset + byte] = true;
    }
    fields.push_back(ByvalPayloadField{
        .payload_offset = *payload_offset,
        .source_stack_offset = slot.offset_bytes,
        .size_bytes = slot.size_bytes,
    });
  }

  if (fields.empty()) {
    return std::nullopt;
  }
  for (bool byte_covered : covered) {
    if (!byte_covered) {
      return std::nullopt;
    }
  }
  return fields;
}

bool emit_riscv_byval_aggregate_address_argument(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedCallArgumentPlan& plan,
    const c4c::backend::bir::Value& argument,
    const c4c::backend::bir::CallArgAbiInfo* argument_abi,
    const PreparedCurrentInstructionContext& context,
    std::size_t& active_stack_adjustment_bytes) {
  namespace prepare = c4c::backend::prepare;

  const auto destination_register = riscv_gpr_argument_register(plan.arg_index);
  if (!destination_register.has_value() ||
      plan.value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      plan.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      plan.source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
      !plan.source_register_name.has_value() ||
      plan.source_register_name->empty() ||
      plan.destination_register_bank.has_value() ||
      plan.destination_register_name.has_value() ||
      plan.destination_contiguous_width != 1 ||
      plan.destination_stack_offset_bytes.has_value() ||
      plan.destination_stack_size_bytes.has_value() ||
      plan.aggregate_transport.has_value()) {
    return false;
  }

  if (argument.kind == c4c::backend::bir::Value::Kind::Named &&
      !argument.name.empty() &&
      argument_abi != nullptr &&
      argument_abi->byval_copy &&
      argument_abi->size_bytes > 0 &&
      argument_abi->size_bytes <= 128 &&
      argument_abi->align_bytes <= 16) {
    const auto fields = byval_payload_fields_for_argument(
        prepared,
        function_name,
        argument.name,
        argument_abi->size_bytes);
    if (fields.has_value()) {
      const std::size_t payload_size =
          align_riscv_stack_slot(argument_abi->size_bytes, 16);
      if (payload_size == 0 ||
          payload_size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
          active_stack_adjustment_bytes >
              std::numeric_limits<std::size_t>::max() - payload_size ||
          !fits_signed_12_bit_immediate(-static_cast<std::int64_t>(payload_size))) {
        return false;
      }
      const std::size_t pending_stack_adjustment =
          active_stack_adjustment_bytes + payload_size;
      std::string payload_out;
      payload_out += "    addi sp, sp, -";
      payload_out += std::to_string(payload_size);
      payload_out += "\n";
      for (const auto& field : *fields) {
        if (field.source_stack_offset >
            std::numeric_limits<std::size_t>::max() - pending_stack_adjustment) {
          return false;
        }
        const std::size_t adjusted_source_offset =
            field.source_stack_offset + pending_stack_adjustment;
        if (adjusted_source_offset >
                static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
            field.payload_offset >
                static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
          return false;
        }
        if (!fits_signed_12_bit_immediate(
                static_cast<std::int64_t>(adjusted_source_offset)) ||
            !fits_signed_12_bit_immediate(
                static_cast<std::int64_t>(field.payload_offset))) {
          return false;
        }
        if (field.size_bytes == 8) {
          payload_out +=
              "    ld t3, " + std::to_string(adjusted_source_offset) + "(sp)\n";
          payload_out +=
              "    sd t3, " + std::to_string(field.payload_offset) + "(sp)\n";
        } else if (field.size_bytes == 4) {
          payload_out +=
              "    lw t3, " + std::to_string(adjusted_source_offset) + "(sp)\n";
          payload_out +=
              "    sw t3, " + std::to_string(field.payload_offset) + "(sp)\n";
        } else if (field.size_bytes == 1) {
          payload_out +=
              "    lb t3, " + std::to_string(adjusted_source_offset) + "(sp)\n";
          payload_out +=
              "    sb t3, " + std::to_string(field.payload_offset) + "(sp)\n";
        } else {
          return false;
        }
      }
      payload_out += "    mv ";
      payload_out += *destination_register;
      payload_out += ", sp\n";
      out += payload_out;
      active_stack_adjustment_bytes = pending_stack_adjustment;
      return true;
    }
  }

  if (context.lookups != nullptr && plan.source_value_id.has_value()) {
    const auto home_it =
        context.lookups->value_homes.homes_by_id.find(*plan.source_value_id);
    const auto* materializations =
        prepare::find_indexed_prepared_address_materializations(
            &context.lookups->address_materializations,
            context.block_label);
    if (home_it != context.lookups->value_homes.homes_by_id.end() &&
        home_it->second != nullptr &&
        home_it->second->value_name != c4c::kInvalidValueName &&
        materializations != nullptr) {
      const prepare::PreparedAddressMaterialization* selected = nullptr;
      for (const auto* materialization : *materializations) {
        if (materialization == nullptr ||
            materialization->inst_index != context.instruction_index ||
            materialization->kind !=
                prepare::PreparedAddressMaterializationKind::FrameSlot ||
            materialization->result_value_name != home_it->second->value_name ||
            materialization->byte_offset < 0 ||
            !fits_signed_12_bit_immediate(materialization->byte_offset)) {
          continue;
        }
        if (selected != nullptr) {
          return false;
        }
        selected = materialization;
      }
      if (selected != nullptr) {
        out += "    addi ";
        out += *destination_register;
        out += ", sp, ";
        out += std::to_string(selected->byte_offset);
        out += "\n";
        return true;
      }
    }
  }

  if (*plan.source_register_name != *destination_register) {
    out += "    mv ";
    out += *destination_register;
    out += ", ";
    out += *plan.source_register_name;
    out += "\n";
  }
  return true;
}

}  // namespace

std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    const PreparedCurrentInstructionContext& context) {
  namespace prepare = c4c::backend::prepare;

  if (call.args.size() > 8 || context.lookups == nullptr) {
    return std::nullopt;
  }

  const auto* call_plan = prepare::find_indexed_prepared_call_plan(
      &context.lookups->call_plans,
      nullptr,
      block_index,
      context.instruction_index);
  if (call_plan == nullptr ||
      call_plan->variadic_fpr_arg_register_count != 0 ||
      call_plan->memory_return.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value() ||
      call_plan->arguments.size() != call.args.size() ||
      call_plan->result.has_value() != call.result.has_value()) {
    return std::nullopt;
  }

  const bool direct_call =
      !call.is_indirect &&
      !call.callee.empty() &&
      !call_plan->is_indirect &&
      !call_plan->indirect_callee.has_value() &&
      call_plan->direct_callee_name == std::optional<std::string>{call.callee} &&
      (call_plan->wrapper_kind == prepare::PreparedCallWrapperKind::SameModule ||
       call_plan->wrapper_kind ==
           prepare::PreparedCallWrapperKind::DirectExternFixedArity);
  const bool indirect_call =
      call.is_indirect &&
      call_plan->is_indirect &&
      call_plan->wrapper_kind == prepare::PreparedCallWrapperKind::Indirect &&
      !call_plan->direct_callee_name.has_value() &&
      call_plan->indirect_callee.has_value() &&
      call_plan->indirect_callee->encoding ==
          prepare::PreparedStorageEncodingKind::Register &&
      call_plan->indirect_callee->bank == prepare::PreparedRegisterBank::Gpr &&
      call_plan->indirect_callee->register_name.has_value() &&
      !call_plan->indirect_callee->register_name->empty();
  if (!direct_call && !indirect_call) {
    return std::nullopt;
  }

  std::string out;
  std::size_t active_stack_adjustment_bytes = 0;
  for (std::size_t arg_index = 0; arg_index < call.args.size(); ++arg_index) {
    const auto* plan = prepare::find_indexed_prepared_immediate_call_argument(
        &context.lookups->call_plans,
        block_index,
        context.instruction_index,
        arg_index);
    if (plan == nullptr && arg_index < call_plan->arguments.size()) {
      plan = &call_plan->arguments[arg_index];
    }
    if (plan == nullptr || plan->arg_index != arg_index) {
      return std::nullopt;
    }
    if (emit_riscv_byval_aggregate_address_argument(out,
                                                    prepared,
                                                    function_name,
                                                    *plan,
                                                    call.args[arg_index],
                                                    arg_index < call.arg_abi.size()
                                                        ? &call.arg_abi[arg_index]
                                                        : nullptr,
                                                    context,
                                                    active_stack_adjustment_bytes)) {
      continue;
    }
    if (plan->value_bank != prepare::PreparedRegisterBank::Gpr ||
        plan->destination_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !plan->destination_register_name.has_value() ||
        plan->destination_register_name->empty() ||
        plan->destination_contiguous_width != 1 ||
        plan->destination_stack_offset_bytes.has_value() ||
        plan->destination_stack_size_bytes.has_value() ||
        plan->aggregate_transport.has_value()) {
      return std::nullopt;
    }
    const auto* source_selection =
        plan->source_selection.has_value() ? &*plan->source_selection : nullptr;
    if (source_selection != nullptr &&
        source_selection->kind ==
            prepare::PreparedCallArgumentSourceSelectionKind::
                LocalFrameAddressMaterialization) {
      if (!source_selection->source_stack_offset_bytes.has_value() ||
          *source_selection->source_stack_offset_bytes >
              static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) ||
          *source_selection->source_stack_offset_bytes >
              static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) -
                  active_stack_adjustment_bytes ||
          !fits_signed_12_bit_immediate(static_cast<std::int64_t>(
              *source_selection->source_stack_offset_bytes +
              active_stack_adjustment_bytes))) {
        return std::nullopt;
      }
      out += "    addi " + *plan->destination_register_name + ", sp, " +
             std::to_string(*source_selection->source_stack_offset_bytes +
                            active_stack_adjustment_bytes) + "\n";
    } else if (plan->source_encoding ==
            prepare::PreparedStorageEncodingKind::Register &&
        plan->source_register_bank ==
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} &&
        plan->source_register_name.has_value() &&
        !plan->source_register_name->empty()) {
      if (*plan->source_register_name != *plan->destination_register_name) {
        out += "    mv " + *plan->destination_register_name + ", " +
               *plan->source_register_name + "\n";
      }
    } else {
      if (!emit_move_to_register(out,
                                 *plan->destination_register_name,
                                 context.names,
                                 context.lookups,
                                 call.args[arg_index])) {
        return std::nullopt;
      }
    }
  }

  if (direct_call) {
    out += "    call " + call.callee + "\n";
  } else {
    out += "    jalr ra, 0(" + *call_plan->indirect_callee->register_name + ")\n";
  }

  if (active_stack_adjustment_bytes != 0) {
    if (!fits_signed_12_bit_immediate(
            static_cast<std::int64_t>(active_stack_adjustment_bytes))) {
      return std::nullopt;
    }
    out += "    addi sp, sp, ";
    out += std::to_string(active_stack_adjustment_bytes);
    out += "\n";
  }

  if (call.result.has_value()) {
    if (!call_plan->result.has_value() ||
        call_plan->result->value_bank != prepare::PreparedRegisterBank::Gpr ||
        call_plan->result->source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        call_plan->result->destination_storage_kind !=
            prepare::PreparedMoveStorageKind::Register ||
        call_plan->result->source_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        call_plan->result->destination_register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !call_plan->result->source_register_name.has_value() ||
        call_plan->result->source_register_name->empty() ||
        !call_plan->result->destination_register_name.has_value() ||
        call_plan->result->destination_register_name->empty() ||
        call_plan->result->source_contiguous_width != 1 ||
        call_plan->result->destination_contiguous_width != 1 ||
        call_plan->result->source_stack_offset_bytes.has_value() ||
        call_plan->result->destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }
    const std::string& destination_register = *call_plan->result->destination_register_name;
    const std::string& source_register = *call_plan->result->source_register_name;
    if (destination_register != source_register) {
      out += "    mv " + destination_register + ", " + source_register + "\n";
    }
  }

  return out;
}

}  // namespace c4c::backend::riscv::codegen
