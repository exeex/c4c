#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_operand_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("operands.fn");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto param_name = prepared.names.value_names.intern("param.semantic");
  const auto slot_name = prepared.names.value_names.intern("slot.semantic");
  const auto symbol_value_name = prepared.names.value_names.intern("symbol.ptr.semantic");
  const auto base_name = prepared.names.value_names.intern("base.semantic");
  const auto symbol_name = prepared.names.link_names.intern("global.identity");

  const auto function_link_name = prepared.module.names.link_names.intern("operands.fn");
  const auto entry_bir_label = prepared.module.names.block_labels.intern("entry");

  bir::Block entry;
  entry.label = "entry.raw";
  entry.label_id = entry_bir_label;
  entry.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "operands.raw";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 7,
                  .function_name = function_name,
                  .value_name = param_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x8"},
              },
              prepare::PreparedValueHome{
                  .value_id = 8,
                  .function_name = function_name,
                  .value_name = slot_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{3},
                  .offset_bytes = std::size_t{24},
              },
              prepare::PreparedValueHome{
                  .value_id = 9,
                  .function_name = function_name,
                  .value_name = symbol_value_name,
                  .kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset,
                  .pointer_base_value_name = base_name,
                  .pointer_byte_delta = std::int64_t{16},
              },
          },
  });

  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedRegallocValue{
                  .value_id = 7,
                  .function_name = function_name,
                  .value_name = param_name,
                  .type = bir::TypeKind::I64,
                  .value_kind = prepare::PreparedValueKind::Parameter,
                  .register_class = prepare::PreparedRegisterClass::General,
                  .register_group_width = 1,
                  .allocation_status = prepare::PreparedAllocationStatus::AssignedRegister,
                  .assigned_register =
                      prepare::PreparedPhysicalRegisterAssignment{
                          .reg_class = prepare::PreparedRegisterClass::General,
                          .register_name = "x19",
                          .contiguous_width = 1,
                          .occupied_register_names = {"x19"},
                      },
              },
              prepare::PreparedRegallocValue{
                  .value_id = 8,
                  .function_name = function_name,
                  .value_name = slot_name,
                  .type = bir::TypeKind::I32,
                  .value_kind = prepare::PreparedValueKind::Temporary,
                  .register_class = prepare::PreparedRegisterClass::General,
                  .allocation_status = prepare::PreparedAllocationStatus::AssignedStackSlot,
                  .assigned_stack_slot =
                      prepare::PreparedStackSlotAssignment{
                          .slot_id = 3,
                          .offset_bytes = 24,
                      },
              },
          },
  });

  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = 7,
                  .value_name = param_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"x20"},
                  .occupied_register_names = {"x20"},
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = 8,
                  .value_name = slot_name,
                  .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                  .slot_id = 3,
                  .stack_offset_bytes = std::size_t{24},
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = 9,
                  .value_name = symbol_value_name,
                  .encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .symbol_name = symbol_name,
              },
          },
  });

  return prepared;
}

const aarch64_module::OperandRecord* find_operand(const aarch64_module::FunctionRecord& function,
                                                  prepare::PreparedValueId value_id) {
  for (const auto& operand : function.operands) {
    if (operand.value_id == value_id) {
      return &operand;
    }
  }
  return nullptr;
}

int records_preserve_operand_identity_and_separate_register_refs() {
  auto prepared = prepared_operand_module();

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared operand module to build");
  }
  const auto& function = result.module->functions.front();
  if (function.operands.size() != 3) {
    return fail("expected three semantic operand records");
  }
  if (function.target_registers.size() != 3) {
    return fail("expected target register references to be recorded separately");
  }

  const auto* param = find_operand(function, 7);
  if (param == nullptr) {
    return fail("expected operand for prepared value id 7");
  }
  if (param->label != "param.semantic" || param->type != bir::TypeKind::I64 ||
      param->value_kind != prepare::PreparedValueKind::Parameter) {
    return fail("expected operand to preserve prepared value name, type, and kind");
  }
  if (!param->value_home_register.has_value() || !param->assigned_register.has_value() ||
      !param->storage_register.has_value()) {
    return fail("expected operand to reference home, assigned, and storage register records");
  }
  if (function.target_registers[*param->value_home_register].physical_register != "x8" ||
      function.target_registers[*param->assigned_register].physical_register != "x19" ||
      function.target_registers[*param->storage_register].physical_register != "x20") {
    return fail("expected physical register names to live in target register records");
  }
  if (function.target_registers[*param->assigned_register].value_name != param->value_name ||
      function.target_registers[*param->assigned_register].physical_register == param->label) {
    return fail("expected target register reference not to replace semantic value identity");
  }
  if (param->source_value_home != &prepared.value_locations.functions.front().value_homes.front() ||
      param->source_regalloc != &prepared.regalloc.functions.front().values.front() ||
      param->source_storage != &prepared.storage_plans.functions.front().values.front()) {
    return fail("expected operand to preserve prepared carrier pointers for inspection");
  }

  const auto* slot = find_operand(function, 8);
  if (slot == nullptr || !slot->frame_slot_id.has_value() || *slot->frame_slot_id != 3 ||
      !slot->stack_offset_bytes.has_value() || *slot->stack_offset_bytes != 24) {
    return fail("expected stack operand to preserve frame-slot identity and offset");
  }
  if (slot->storage_encoding != prepare::PreparedStorageEncodingKind::FrameSlot) {
    return fail("expected stack operand to preserve storage-plan encoding");
  }

  const auto* symbol = find_operand(function, 9);
  if (symbol == nullptr || !symbol->symbol_name.has_value() ||
      symbol->symbol_label != "global.identity") {
    return fail("expected symbol operand to preserve structured link-name identity");
  }
  if (!symbol->pointer_base_value_name.has_value() ||
      symbol->pointer_base_label != "base.semantic" || !symbol->pointer_byte_delta.has_value() ||
      *symbol->pointer_byte_delta != 16) {
    return fail("expected pointer-base identity and byte delta to survive the handoff");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = records_preserve_operand_identity_and_separate_register_refs();
      status != 0) {
    return status;
  }
  return 0;
}
