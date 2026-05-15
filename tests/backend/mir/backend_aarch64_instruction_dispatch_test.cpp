#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_with_unsupported_instructions() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.fn");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.entry");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.fn",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "dispatch.entry",
          .insts = {bir::BinaryInst{
                        .opcode = bir::BinaryOpcode::Add,
                        .result = bir::Value::named(bir::TypeKind::I32, "sum"),
                        .operand_type = bir::TypeKind::I32,
                        .lhs = bir::Value::immediate_i32(1),
                        .rhs = bir::Value::immediate_i32(2),
                    },
                    bir::CallInst{
                        .result = bir::Value::named(bir::TypeKind::I32, "call_result"),
                        .callee = "external",
                        .return_type = bir::TypeKind::I32,
                    }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_call_plan() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.call");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.call.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.call.entry");
  const auto actual_link = prepared.names.link_names.intern("actual_function");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.entry",
          .insts = {bir::CallInst{
              .callee = "actual_function",
              .callee_link_name_id = actual_link,
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"actual_function"},
          .preserved_values =
              {prepare::PreparedCallPreservedValue{
                  .value_id = prepare::PreparedValueId{42},
                  .value_name = c4c::ValueNameId{17},
                  .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
                  .callee_saved_save_index = std::size_t{0},
                  .contiguous_width = 1,
                  .register_name = std::string{"x19"},
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .occupied_register_names = {"x19"},
              },
              prepare::PreparedCallPreservedValue{
                  .value_id = prepare::PreparedValueId{43},
                  .value_name = c4c::ValueNameId{18},
                  .route = prepare::PreparedCallPreservationRoute::StackSlot,
                  .contiguous_width = 1,
                  .slot_id = prepare::PreparedFrameSlotId{11},
                  .stack_offset_bytes = std::size_t{96},
                  .spill_slot_placement =
                      prepare::PreparedSpillSlotPlacement{
                          .slot_id = prepare::PreparedFrameSlotId{11},
                          .offset_bytes = 96,
                      },
              }},
          .clobbered_registers = {prepare::PreparedClobberedRegister{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .register_name = "v13",
              .contiguous_width = 2,
              .occupied_register_names = {"v13", "v14"},
          }},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_memory_return_call_plan() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.memret");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.memret.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.memret.entry");
  const auto actual_link = prepared.names.link_names.intern("make_large");
  const auto storage_slot = prepared.names.slot_names.intern("lv.call.sret.storage");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.memret",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.memret.entry",
          .insts = {bir::CallInst{
              .callee = "make_large",
              .callee_link_name_id = actual_link,
              .args = {bir::Value::named(bir::TypeKind::Ptr, "lv.call.sret.storage")},
              .arg_types = {bir::TypeKind::Ptr},
              .arg_abi = {bir::CallArgAbiInfo{
                  .type = bir::TypeKind::Ptr,
                  .size_bytes = 24,
                  .align_bytes = 8,
                  .primary_class = bir::AbiValueClass::Memory,
                  .sret_pointer = true,
              }},
              .return_type = bir::TypeKind::Void,
              .result_abi = bir::CallResultAbiInfo{
                  .type = bir::TypeKind::Void,
                  .primary_class = bir::AbiValueClass::Memory,
                  .returned_in_memory = true,
              },
              .calling_convention = bir::CallingConv::C,
              .sret_storage_name = "lv.call.sret.storage",
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"make_large"},
          .memory_return =
              prepare::PreparedMemoryReturnPlan{
                  .sret_arg_index = std::size_t{0},
                  .storage_slot_name = storage_slot,
                  .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                  .slot_id = prepare::PreparedFrameSlotId{7},
                  .stack_offset_bytes = std::size_t{64},
                  .size_bytes = 24,
                  .align_bytes = 8,
              },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_variadic_entry_helper_call(
    bool include_entry_plan,
    bool complete_storage_facts = false,
    bool complete_scratch_facts = false) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.variadic.entry");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.variadic.entry.block");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.variadic.entry.block");
  const auto va_start_link = prepared.names.link_names.intern("llvm.va_start.p0");
  const auto ap_value = prepared.names.value_names.intern("%ap");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.variadic.entry",
      .return_type = bir::TypeKind::Void,
      .is_variadic = true,
      .params = {bir::Param{.type = bir::TypeKind::I32, .name = "fixed"}},
      .blocks = {bir::Block{
          .label = "dispatch.variadic.entry.block",
          .insts = {bir::CallInst{
              .callee = "llvm.va_start.p0",
              .callee_link_name_id = va_start_link,
              .args = {bir::Value::named(bir::TypeKind::Ptr, "%ap")},
              .arg_types = {bir::TypeKind::Ptr},
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"llvm.va_start.p0"},
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{12},
              .function_name = function_name,
              .value_name = ap_value,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x3"},
          }},
  });
  if (include_entry_plan) {
    const bool complete_operand_home_facts = complete_storage_facts && complete_scratch_facts;
    prepared.variadic_entry_plans.functions.push_back(
        prepare::PreparedVariadicEntryPlanFunction{
            .function_name = function_name,
            .named_parameter_count = 1,
            .named_register_counts =
                prepare::PreparedVariadicEntryNamedRegisterCounts{
                    .gp = std::size_t{1},
                    .fp = std::size_t{0},
                },
            .register_save_area =
                prepare::PreparedVariadicEntryRegisterSaveArea{
                    .required = true,
                    .size_bytes = std::size_t{192},
                    .align_bytes = std::size_t{16},
                    .slot_id = prepare::PreparedFrameSlotId{5},
                    .stack_offset_bytes = std::size_t{16},
                    .gp_offset_bytes = std::size_t{0},
                    .fp_offset_bytes = std::size_t{64},
                    .gp_slot_size_bytes = std::size_t{8},
                    .fp_slot_size_bytes = std::size_t{16},
                    .saved_gp_register_count = std::size_t{7},
                    .saved_fp_register_count = std::size_t{8},
                    .initial_gp_offset_bytes = std::ptrdiff_t{-56},
                    .initial_fp_offset_bytes = std::ptrdiff_t{-128},
                },
            .overflow_area =
                prepare::PreparedVariadicEntryOverflowArea{
                    .required = true,
                    .base_slot_id =
                        complete_storage_facts
                            ? std::optional<prepare::PreparedFrameSlotId>{
                                  prepare::PreparedFrameSlotId{6}}
                            : std::nullopt,
                    .base_stack_offset_bytes = std::size_t{208},
                    .align_bytes = std::size_t{8},
                },
            .va_list_layout =
                prepare::PreparedVariadicVaListLayout{
                    .required = true,
                    .size_bytes = std::size_t{32},
                    .align_bytes = std::size_t{8},
                    .fields =
                        {
                            prepare::PreparedVariadicVaListField{
                                .kind =
                                    prepare::PreparedVariadicVaListFieldKind::GpOffset,
                                .offset_bytes = 0,
                                .size_bytes = 4,
                            },
                        },
                },
            .helper_resources =
                prepare::PreparedVariadicEntryHelperResources{
                    .required_helpers =
                        {prepare::PreparedVariadicEntryHelperKind::VaStart},
                    .scratch_register_count = complete_scratch_facts
                                                  ? std::optional<std::size_t>{1}
                                                  : std::nullopt,
                    .scratch_stack_bytes = complete_scratch_facts
                                               ? std::optional<std::size_t>{0}
                                               : std::nullopt,
                },
            .helper_operand_homes =
                complete_operand_home_facts
                    ? std::vector<prepare::PreparedVariadicEntryHelperOperandHomes>{
                          prepare::PreparedVariadicEntryHelperOperandHomes{
                              .helper =
                                  prepare::PreparedVariadicEntryHelperKind::VaStart,
                              .block_index = 0,
                              .instruction_index = 0,
                              .destination_va_list =
                                  prepared.value_locations.functions.front()
                                      .value_homes.front(),
                          }}
                    : std::vector<prepare::PreparedVariadicEntryHelperOperandHomes>{},
            .missing_required_facts =
                complete_storage_facts ? std::vector<std::string>{}
                                       : std::vector<std::string>{"overflow_area.base_slot_id"},
        });
  }
  return prepared;
}

prepare::PreparedBirModule prepared_with_scalar_va_arg_helper_call() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.scalar.va_arg");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.scalar.va_arg.block");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.scalar.va_arg.block");
  const auto va_arg_link = prepared.names.link_names.intern("llvm.va_arg.i32");
  const auto ap_value = prepared.names.value_names.intern("%ap");
  const auto result_value = prepared.names.value_names.intern("%next");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.scalar.va_arg",
      .return_type = bir::TypeKind::Void,
      .is_variadic = true,
      .params = {bir::Param{.type = bir::TypeKind::I32, .name = "fixed"}},
      .blocks = {bir::Block{
          .label = "dispatch.scalar.va_arg.block",
          .insts = {bir::CallInst{
              .result = bir::Value::named(bir::TypeKind::I32, "%next"),
              .callee = "llvm.va_arg.i32",
              .callee_link_name_id = va_arg_link,
              .args = {bir::Value::named(bir::TypeKind::Ptr, "%ap")},
              .arg_types = {bir::TypeKind::Ptr},
              .return_type = bir::TypeKind::I32,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"llvm.va_arg.i32"},
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{22},
                  .function_name = function_name,
                  .value_name = ap_value,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x3"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{23},
                  .function_name = function_name,
                  .value_name = result_value,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w0"},
              },
          },
  });
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{1},
                  .fp = std::size_t{0},
              },
          .register_save_area =
              prepare::PreparedVariadicEntryRegisterSaveArea{
                  .required = true,
                  .size_bytes = std::size_t{192},
                  .align_bytes = std::size_t{16},
                  .slot_id = prepare::PreparedFrameSlotId{5},
                  .stack_offset_bytes = std::size_t{16},
                  .gp_offset_bytes = std::size_t{0},
                  .fp_offset_bytes = std::size_t{64},
                  .gp_slot_size_bytes = std::size_t{8},
                  .fp_slot_size_bytes = std::size_t{16},
                  .saved_gp_register_count = std::size_t{7},
                  .saved_fp_register_count = std::size_t{8},
                  .initial_gp_offset_bytes = std::ptrdiff_t{-56},
                  .initial_fp_offset_bytes = std::ptrdiff_t{-128},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = prepare::PreparedFrameSlotId{6},
                  .base_stack_offset_bytes = std::size_t{208},
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{32},
                  .align_bytes = std::size_t{8},
                  .fields =
                      {
                          prepare::PreparedVariadicVaListField{
                              .kind =
                                  prepare::PreparedVariadicVaListFieldKind::GpOffset,
                              .offset_bytes = 0,
                              .size_bytes = 4,
                          },
                      },
              },
          .helper_resources =
              prepare::PreparedVariadicEntryHelperResources{
                  .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaArg},
                  .scratch_register_count = std::size_t{2},
                  .scratch_stack_bytes = std::size_t{0},
              },
          .helper_operand_homes =
              {prepare::PreparedVariadicEntryHelperOperandHomes{
                  .helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_va_list =
                      prepared.value_locations.functions.front().value_homes.front(),
                  .scalar_result =
                      prepared.value_locations.functions.front().value_homes.back(),
              }},
      });
  return prepared;
}

prepare::PreparedBirModule prepared_with_aggregate_va_arg_helper_call() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("dispatch.aggregate.va_arg");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.aggregate.va_arg.block");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.aggregate.va_arg.block");
  const auto va_arg_link =
      prepared.names.link_names.intern("llvm.va_arg.aggregate");
  const auto dst_value = prepared.names.value_names.intern("%dst");
  const auto ap_value = prepared.names.value_names.intern("%ap");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.aggregate.va_arg",
      .return_type = bir::TypeKind::Void,
      .is_variadic = true,
      .params = {bir::Param{.type = bir::TypeKind::I32, .name = "fixed"}},
      .blocks = {bir::Block{
          .label = "dispatch.aggregate.va_arg.block",
          .insts = {bir::CallInst{
              .callee = "llvm.va_arg.aggregate",
              .callee_link_name_id = va_arg_link,
              .args = {bir::Value::named(bir::TypeKind::Ptr, "%dst"),
                       bir::Value::named(bir::TypeKind::Ptr, "%ap")},
              .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"llvm.va_arg.aggregate"},
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{31},
                  .function_name = function_name,
                  .value_name = dst_value,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{9},
                  .offset_bytes = std::size_t{32},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{32},
                  .function_name = function_name,
                  .value_name = ap_value,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x3"},
              },
          },
  });
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{1},
                  .fp = std::size_t{0},
              },
          .register_save_area =
              prepare::PreparedVariadicEntryRegisterSaveArea{
                  .required = true,
                  .size_bytes = std::size_t{192},
                  .align_bytes = std::size_t{16},
                  .slot_id = prepare::PreparedFrameSlotId{5},
                  .stack_offset_bytes = std::size_t{16},
                  .gp_offset_bytes = std::size_t{0},
                  .fp_offset_bytes = std::size_t{64},
                  .gp_slot_size_bytes = std::size_t{8},
                  .fp_slot_size_bytes = std::size_t{16},
                  .saved_gp_register_count = std::size_t{7},
                  .saved_fp_register_count = std::size_t{8},
                  .initial_gp_offset_bytes = std::ptrdiff_t{-56},
                  .initial_fp_offset_bytes = std::ptrdiff_t{-128},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = prepare::PreparedFrameSlotId{6},
                  .base_stack_offset_bytes = std::size_t{208},
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{32},
                  .align_bytes = std::size_t{8},
                  .fields =
                      {
                          prepare::PreparedVariadicVaListField{
                              .kind =
                                  prepare::PreparedVariadicVaListFieldKind::GpOffset,
                              .offset_bytes = 0,
                              .size_bytes = 4,
                          },
                      },
              },
          .helper_resources =
              prepare::PreparedVariadicEntryHelperResources{
                  .required_helpers =
                      {prepare::PreparedVariadicEntryHelperKind::VaArgAggregate},
                  .scratch_register_count = std::size_t{2},
                  .scratch_stack_bytes = std::size_t{0},
              },
          .helper_operand_homes =
              {prepare::PreparedVariadicEntryHelperOperandHomes{
                  .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_va_list =
                      prepared.value_locations.functions.front().value_homes.back(),
                  .aggregate_destination_payload =
                      prepared.value_locations.functions.front().value_homes.front(),
              }},
      });
  return prepared;
}

prepare::PreparedBirModule prepared_with_va_copy_helper_call() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.va_copy");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.va_copy.block");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.va_copy.block");
  const auto va_copy_link =
      prepared.names.link_names.intern("llvm.va_copy.p0.p0");
  const auto dst_value = prepared.names.value_names.intern("%dst_ap");
  const auto src_value = prepared.names.value_names.intern("%src_ap");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.va_copy",
      .return_type = bir::TypeKind::Void,
      .is_variadic = true,
      .params = {bir::Param{.type = bir::TypeKind::I32, .name = "fixed"}},
      .blocks = {bir::Block{
          .label = "dispatch.va_copy.block",
          .insts = {bir::CallInst{
              .callee = "llvm.va_copy.p0.p0",
              .callee_link_name_id = va_copy_link,
              .args = {bir::Value::named(bir::TypeKind::Ptr, "%dst_ap"),
                       bir::Value::named(bir::TypeKind::Ptr, "%src_ap")},
              .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"llvm.va_copy.p0.p0"},
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{33},
                  .function_name = function_name,
                  .value_name = dst_value,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{10},
                  .offset_bytes = std::size_t{64},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{34},
                  .function_name = function_name,
                  .value_name = src_value,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x4"},
              },
          },
  });
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{1},
                  .fp = std::size_t{0},
              },
          .register_save_area =
              prepare::PreparedVariadicEntryRegisterSaveArea{
                  .required = true,
                  .size_bytes = std::size_t{192},
                  .align_bytes = std::size_t{16},
                  .slot_id = prepare::PreparedFrameSlotId{5},
                  .stack_offset_bytes = std::size_t{16},
                  .gp_offset_bytes = std::size_t{0},
                  .fp_offset_bytes = std::size_t{64},
                  .gp_slot_size_bytes = std::size_t{8},
                  .fp_slot_size_bytes = std::size_t{16},
                  .saved_gp_register_count = std::size_t{7},
                  .saved_fp_register_count = std::size_t{8},
                  .initial_gp_offset_bytes = std::ptrdiff_t{-56},
                  .initial_fp_offset_bytes = std::ptrdiff_t{-128},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = prepare::PreparedFrameSlotId{6},
                  .base_stack_offset_bytes = std::size_t{208},
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{32},
                  .align_bytes = std::size_t{8},
                  .fields =
                      {
                          prepare::PreparedVariadicVaListField{
                              .kind =
                                  prepare::PreparedVariadicVaListFieldKind::GpOffset,
                              .offset_bytes = 0,
                              .size_bytes = 4,
                          },
                          prepare::PreparedVariadicVaListField{
                              .kind =
                                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                              .offset_bytes = 8,
                              .size_bytes = 8,
                          },
                      },
              },
          .helper_resources =
              prepare::PreparedVariadicEntryHelperResources{
                  .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaCopy},
                  .scratch_register_count = std::size_t{1},
                  .scratch_stack_bytes = std::size_t{0},
              },
          .helper_operand_homes =
              {prepare::PreparedVariadicEntryHelperOperandHomes{
                  .helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
                  .block_index = 0,
                  .instruction_index = 0,
                  .destination_va_list =
                      prepared.value_locations.functions.front().value_homes.front(),
                  .source_va_list =
                      prepared.value_locations.functions.front().value_homes.back(),
              }},
      });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_call_argument_register_move() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.call.arg");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.call.arg.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.call.arg.entry");
  const auto actual_link = prepared.names.link_names.intern("actual_function");
  const auto arg_name = prepared.names.value_names.intern("%arg");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call.arg",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.arg.entry",
          .insts = {bir::CallInst{
              .callee = "actual_function",
              .callee_link_name_id = actual_link,
              .args = {bir::Value::named(bir::TypeKind::I64, "%arg")},
              .arg_types = {bir::TypeKind::I64},
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

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
                  .value_id = prepare::PreparedValueId{41},
                  .function_name = function_name,
                  .value_name = arg_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x2"},
              },
          },
      .move_bundles =
          {
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BeforeCall,
                  .block_index = 0,
                  .instruction_index = 0,
                  .moves =
                      {
                          prepare::PreparedMoveResolution{
                              .from_value_id = prepare::PreparedValueId{41},
                              .to_value_id = prepare::PreparedValueId{41},
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{0},
                              .destination_register_name = std::string{"x0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                              .block_index = 0,
                              .instruction_index = 0,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .reason = "call_arg_register_to_register",
                          },
                      },
                  .abi_bindings =
                      {
                          prepare::PreparedAbiBinding{
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{0},
                              .destination_register_name = std::string{"x0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                          },
                      },
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"actual_function"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{41},
                      .source_register_name = std::string{"x2"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"x0"},
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                  },
              },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_call_result_register_move() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.call.result");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.call.result.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.call.result.entry");
  const auto actual_link = prepared.names.link_names.intern("actual_function");
  const auto result_name = prepared.names.value_names.intern("%call_result");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call.result",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.result.entry",
          .insts = {bir::CallInst{
              .result = bir::Value::named(bir::TypeKind::I64, "%call_result"),
              .callee = "actual_function",
              .callee_link_name_id = actual_link,
              .return_type = bir::TypeKind::I64,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

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
                  .value_id = prepare::PreparedValueId{51},
                  .function_name = function_name,
                  .value_name = result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x3"},
              },
          },
      .move_bundles =
          {
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::AfterCall,
                  .block_index = 0,
                  .instruction_index = 0,
                  .moves =
                      {
                          prepare::PreparedMoveResolution{
                              .from_value_id = prepare::PreparedValueId{51},
                              .to_value_id = prepare::PreparedValueId{51},
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallResultAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_register_name = std::string{"x0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                              .block_index = 0,
                              .instruction_index = 0,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .reason = "call_result_register_to_register",
                          },
                      },
                  .abi_bindings =
                      {
                          prepare::PreparedAbiBinding{
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallResultAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_register_name = std::string{"x0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                          },
                      },
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"actual_function"},
          .result = prepare::PreparedCallResultPlan{
              .instruction_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_value_id = prepare::PreparedValueId{51},
              .source_register_name = std::string{"x0"},
              .source_contiguous_width = 1,
              .source_occupied_register_names = {"x0"},
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_name = std::string{"x3"},
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {"x3"},
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_indirect_call_plan(bool register_callee) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.indirect");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.indirect.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.indirect.entry");
  const auto callee_name = prepared.names.value_names.intern("%callee");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.indirect",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.indirect.entry",
          .insts = {bir::CallInst{
              .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%callee"),
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
              .is_indirect = true,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
          .is_indirect = true,
          .indirect_callee =
              prepare::PreparedIndirectCalleePlan{
                  .value_name = callee_name,
                  .value_id = prepare::PreparedValueId{31},
                  .encoding = register_callee
                                  ? prepare::PreparedStorageEncodingKind::Register
                                  : prepare::PreparedStorageEncodingKind::FrameSlot,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .register_name = register_callee
                                       ? std::optional<std::string>{"x9"}
                                       : std::nullopt,
                  .slot_id = register_callee
                                 ? std::nullopt
                                 : std::optional<prepare::PreparedFrameSlotId>{4},
                  .stack_offset_bytes = register_callee
                                            ? std::nullopt
                                            : std::optional<std::size_t>{24},
              },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_reordered_retained_bir() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto first_function = prepared.names.function_names.intern("dispatch.first");
  const auto second_function = prepared.names.function_names.intern("dispatch.second");
  const auto first_entry = prepared.names.block_labels.intern("first.entry");
  const auto second_entry = prepared.names.block_labels.intern("second.entry");
  const auto second_late = prepared.names.block_labels.intern("second.late");

  const auto bir_first_entry = prepared.module.names.block_labels.intern("first.entry");
  const auto bir_second_entry = prepared.module.names.block_labels.intern("second.entry");
  const auto bir_second_late = prepared.module.names.block_labels.intern("second.late");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.second",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
                     .label = "second.late",
                     .insts = {bir::CallInst{
                         .result = bir::Value::named(bir::TypeKind::I32, "late_call"),
                         .callee = "external",
                         .return_type = bir::TypeKind::I32,
                     }},
                     .terminator = bir::Terminator{bir::ReturnTerminator{}},
                     .label_id = bir_second_late,
                 },
                 bir::Block{
                     .label = "second.entry",
                     .insts = {bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Mul,
                         .result = bir::Value::named(bir::TypeKind::I32, "entry_product"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::immediate_i32(3),
                         .rhs = bir::Value::immediate_i32(4),
                     }},
                     .terminator = bir::Terminator{bir::ReturnTerminator{}},
                     .label_id = bir_second_entry,
                 }},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.first",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "first.entry",
          .insts = {bir::CallInst{
              .result = bir::Value::named(bir::TypeKind::I32, "first_call"),
              .callee = "external",
              .return_type = bir::TypeKind::I32,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_first_entry,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = first_function,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = first_entry,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = second_function,
      .blocks = {prepare::PreparedControlFlowBlock{
                     .block_label = second_entry,
                     .terminator_kind = bir::TerminatorKind::Return,
                 },
                 prepare::PreparedControlFlowBlock{
                     .block_label = second_late,
                     .terminator_kind = bir::TerminatorKind::Return,
                 }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_control_flow_only() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.cf_only");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.cf_entry");
  const auto exit_label = prepared.names.block_labels.intern("dispatch.cf_exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Branch,
          .branch_target_label = exit_label,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_simple_fixed_frame() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.frame");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.frame.entry");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
  });
  return prepared;
}

int block_dispatch_visits_prepared_terminator_without_bir_block_mapping() {
  auto prepared = prepared_with_control_flow_only();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty()) {
    return fail("expected control-flow-only dispatch to visit terminator only");
  }
  if (block.block_label != block_cf.block_label || block.index != 0) {
    return fail("expected control-flow-only dispatch to preserve prepared block identity");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected unsupported terminator diagnostic without BIR mapping");
  }
  return 0;
}

int block_dispatch_visits_prepared_instructions_in_order_and_fails_closed() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block{
      .block_label = c4c::kInvalidBlockLabel,
      .index = 99,
      .instructions = {},
  };
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 2 || !result.visited_terminator ||
      result.emitted_instructions != 1) {
    return fail("expected dispatch to visit prepared instructions and emit return");
  }
  if (block.block_label != block_cf.block_label || block.index != 0 ||
      block.instructions.size() != 1) {
    return fail("expected dispatch to preserve identity and emit one return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected dispatch to emit canonical return instruction target");
  }
  if (diagnostics.entries.size() != 2) {
    return fail("expected unsupported instruction diagnostics only");
  }
  if (diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      diagnostics.entries[0].instruction_index != 0 ||
      diagnostics.entries[0].instruction_family !=
          aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected first diagnostic to describe scalar instruction zero");
  }
  if (diagnostics.entries[1].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan ||
      diagnostics.entries[1].instruction_index != 1 ||
      diagnostics.entries[1].instruction_family !=
          aarch64_module::InstructionLoweringFamily::Call) {
    return fail("expected second diagnostic to describe missing prepared call plan");
  }
  return 0;
}

int block_dispatch_lowers_prepared_direct_call_without_reclassifying_abi() {
  auto prepared = prepared_with_direct_call_plan();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  if (function_context.call_plans == nullptr ||
      function_context.frame_plan != nullptr ||
      function_context.dynamic_stack_plan != nullptr) {
    return fail("expected function context to thread prepared call plans without fake frame plans");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      !diagnostics.empty()) {
    return fail("expected prepared direct call dispatch to emit call plus return");
  }

  const auto& call_instruction = block.instructions.front();
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &call_instruction.target.payload);
  if (call == nullptr ||
      call_instruction.target.family != aarch64_module::codegen::InstructionFamily::Call ||
      call_instruction.target.opcode != aarch64_module::codegen::MachineOpcode::DirectCall ||
      call_instruction.target.function_name != function_cf.function_name ||
      call_instruction.target.block_label != block_cf.block_label ||
      call_instruction.target.instruction_index != 0 ||
      !call->direct_callee.has_value() ||
      call->direct_callee_label != "actual_function" ||
      call->wrapper_kind != prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      call->source_call != &function_context.call_plans->calls.front() ||
      call->preserved_values.size() != 2 ||
      call->clobbered_registers.size() != 1 ||
      !call->arguments.empty() || call->result.has_value()) {
    return fail("expected direct call node to preserve prepared call provenance");
  }
  if (call->preserved_values[1].route != prepare::PreparedCallPreservationRoute::StackSlot ||
      call->preserved_values[1].slot_id != std::optional<prepare::PreparedFrameSlotId>{11} ||
      call->preserved_values[1].stack_offset_bytes != std::optional<std::size_t>{96}) {
    return fail("expected direct call node to retain stack-slot preserved-value provenance");
  }
  if (call_instruction.target.preserves.size() != 1 ||
      call_instruction.target.preserves.front().kind !=
          aarch64_module::codegen::MachineEffectResourceKind::Register ||
      call_instruction.target.preserves.front().reg != aarch64_module::abi::x_register(19) ||
      call_instruction.target.preserves.front().value_id != prepare::PreparedValueId{42} ||
      call_instruction.target.preserves.front().value_name != c4c::ValueNameId{17} ||
      !call_instruction.target.preserves.front().operand.has_value()) {
    return fail("expected direct call node to expose prepared preserved-value register effect");
  }
  const auto* preserved = std::get_if<aarch64_module::codegen::RegisterOperand>(
      &call_instruction.target.preserves.front().operand->payload);
  if (preserved == nullptr ||
      preserved->role != aarch64_module::codegen::RegisterOperandRole::CallAbi ||
      preserved->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      preserved->value_id != prepare::PreparedValueId{42} ||
      preserved->value_name != c4c::ValueNameId{17} ||
      preserved->occupied_register_references.size() != 1 ||
      preserved->occupied_register_references.front() !=
          aarch64_module::abi::x_register(19)) {
    return fail("expected preserved-value effect to retain prepared value and register facts");
  }
  if (call_instruction.target.clobbers.size() != 1 ||
      call_instruction.target.clobbers.front().kind !=
          aarch64_module::codegen::MachineEffectResourceKind::Register ||
      call_instruction.target.clobbers.front().reg != aarch64_module::abi::v_register(13) ||
      !call_instruction.target.clobbers.front().operand.has_value()) {
    return fail("expected direct call node to expose prepared grouped vector clobber effect");
  }
  const auto* clobber = std::get_if<aarch64_module::codegen::RegisterOperand>(
      &call_instruction.target.clobbers.front().operand->payload);
  if (clobber == nullptr ||
      clobber->role != aarch64_module::codegen::RegisterOperandRole::CallAbi ||
      clobber->prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      clobber->contiguous_width != 2 ||
      clobber->occupied_register_references.size() != 2 ||
      clobber->occupied_register_references[0] != aarch64_module::abi::v_register(13) ||
      clobber->occupied_register_references[1] != aarch64_module::abi::v_register(14)) {
    return fail("expected grouped vector clobber to preserve prepared occupied registers");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.back().target.payload)) {
    return fail("expected return terminator to remain lowered after direct call");
  }
  return 0;
}

int block_dispatch_exposes_prepared_memory_return_storage_on_call_node() {
  auto prepared = prepared_with_direct_memory_return_call_plan();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      !diagnostics.empty()) {
    return fail("expected prepared memory-return direct call dispatch to emit call plus return");
  }

  const auto& call_instruction = block.instructions.front();
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &call_instruction.target.payload);
  if (call == nullptr ||
      call_instruction.target.family != aarch64_module::codegen::InstructionFamily::Call ||
      call_instruction.target.opcode != aarch64_module::codegen::MachineOpcode::DirectCall ||
      call_instruction.target.function_name != function_cf.function_name ||
      call_instruction.target.block_label != block_cf.block_label ||
      !call->memory_return.has_value() ||
      !call->memory_return_storage.has_value() ||
      call->memory_return->slot_id != std::optional<prepare::PreparedFrameSlotId>{7} ||
      call->memory_return->stack_offset_bytes != std::optional<std::size_t>{64} ||
      call->memory_return_storage->base_kind !=
          aarch64_module::codegen::MemoryBaseKind::FrameSlot ||
      call->memory_return_storage->frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{7} ||
      call->memory_return_storage->byte_offset != 64 ||
      call->memory_return_storage->size_bytes != 24 ||
      call->memory_return_storage->align_bytes != 8 ||
      call->source_call != &function_context.call_plans->calls.front()) {
    return fail("expected call node to preserve prepared memory-return storage facts");
  }
  if (call_instruction.target.defs.size() != 1 ||
      call_instruction.target.defs.front().kind !=
          aarch64_module::codegen::MachineEffectResourceKind::Memory ||
      call_instruction.target.defs.front().frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{7} ||
      call_instruction.target.side_effects.size() != 2 ||
      call_instruction.target.side_effects.front() !=
          aarch64_module::codegen::MachineSideEffectKind::Call ||
      call_instruction.target.side_effects.back() !=
          aarch64_module::codegen::MachineSideEffectKind::MemoryWrite ||
      !call_instruction.target.defs.front().operand.has_value()) {
    return fail("expected memory-return call to expose storage as a memory def effect");
  }
  const auto* storage = std::get_if<aarch64_module::codegen::MemoryOperand>(
      &call_instruction.target.defs.front().operand->payload);
  if (storage == nullptr ||
      storage->base_kind != aarch64_module::codegen::MemoryBaseKind::FrameSlot ||
      storage->frame_slot_id != std::optional<prepare::PreparedFrameSlotId>{7} ||
      storage->byte_offset != 64) {
    return fail("expected memory-return effect operand to carry prepared slot and offset");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.back().target.payload)) {
    return fail("expected return terminator to remain lowered after memory-return direct call");
  }
  return 0;
}

int variadic_entry_helper_dispatch_requires_complete_prepared_entry_plan() {
  auto missing_entry_prepared = prepared_with_variadic_entry_helper_call(false);
  const auto& missing_function_cf = missing_entry_prepared.control_flow.functions.front();
  const auto& missing_block_cf = missing_function_cf.blocks.front();
  const auto missing_function_context = aarch64_codegen::make_function_lowering_context(
      missing_entry_prepared, missing_entry_prepared.target_profile, missing_function_cf);
  const auto missing_block_context =
      aarch64_codegen::make_block_lowering_context(
          missing_function_context, missing_block_cf, 0);
  aarch64_module::MachineBlock missing_block;
  aarch64_module::ModuleLoweringDiagnostics missing_diagnostics;
  const auto missing_result = aarch64_codegen::dispatch_prepared_block(
      missing_block_context, missing_block, missing_diagnostics);
  if (missing_result.visited_operations != 1 || !missing_result.visited_terminator ||
      missing_result.emitted_instructions != 1 || missing_block.instructions.size() != 1 ||
      missing_diagnostics.entries.size() != 1 ||
      missing_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan ||
      missing_diagnostics.entries.front().message.find(
          "PreparedVariadicEntryPlanFunction") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          missing_block.instructions.front().target.payload)) {
    return fail("expected va_start dispatch to require prepared variadic entry carrier");
  }

  auto incomplete_entry_prepared = prepared_with_variadic_entry_helper_call(true);
  const auto& incomplete_function_cf =
      incomplete_entry_prepared.control_flow.functions.front();
  const auto& incomplete_block_cf = incomplete_function_cf.blocks.front();
  const auto incomplete_function_context =
      aarch64_codegen::make_function_lowering_context(
          incomplete_entry_prepared,
          incomplete_entry_prepared.target_profile,
          incomplete_function_cf);
  const auto incomplete_block_context =
      aarch64_codegen::make_block_lowering_context(
          incomplete_function_context, incomplete_block_cf, 0);
  aarch64_module::MachineBlock incomplete_block;
  aarch64_module::ModuleLoweringDiagnostics incomplete_diagnostics;
  const auto incomplete_result = aarch64_codegen::dispatch_prepared_block(
      incomplete_block_context, incomplete_block, incomplete_diagnostics);
  if (incomplete_result.visited_operations != 1 ||
      !incomplete_result.visited_terminator ||
      incomplete_result.emitted_instructions != 1 ||
      incomplete_block.instructions.size() != 1 ||
      incomplete_diagnostics.entries.size() != 1 ||
      incomplete_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      incomplete_diagnostics.entries.front().message.find(
          "complete prepared variadic entry facts") == std::string::npos ||
      incomplete_diagnostics.entries.front().message.find(
          "overflow_area.base_slot_id") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          incomplete_block.instructions.front().target.payload)) {
    return fail("expected incomplete va_start prepared overflow storage facts to fail closed");
  }

  auto missing_scratch_prepared = prepared_with_variadic_entry_helper_call(true, true);
  const auto& missing_scratch_function_cf =
      missing_scratch_prepared.control_flow.functions.front();
  const auto& missing_scratch_block_cf = missing_scratch_function_cf.blocks.front();
  const auto missing_scratch_function_context =
      aarch64_codegen::make_function_lowering_context(
          missing_scratch_prepared,
          missing_scratch_prepared.target_profile,
          missing_scratch_function_cf);
  const auto missing_scratch_block_context =
      aarch64_codegen::make_block_lowering_context(
          missing_scratch_function_context, missing_scratch_block_cf, 0);
  aarch64_module::MachineBlock missing_scratch_block;
  aarch64_module::ModuleLoweringDiagnostics missing_scratch_diagnostics;
  const auto missing_scratch_result = aarch64_codegen::dispatch_prepared_block(
      missing_scratch_block_context, missing_scratch_block, missing_scratch_diagnostics);
  if (missing_scratch_result.visited_operations != 1 ||
      !missing_scratch_result.visited_terminator ||
      missing_scratch_result.emitted_instructions != 1 ||
      missing_scratch_block.instructions.size() != 1 ||
      missing_scratch_diagnostics.entries.size() != 1 ||
      missing_scratch_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      missing_scratch_diagnostics.entries.front().message.find(
          "helper_resources.scratch_register_count") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          missing_scratch_block.instructions.front().target.payload)) {
    return fail("expected incomplete va_start prepared scratch facts to fail closed");
  }

  auto missing_scratch_stack_prepared =
      prepared_with_variadic_entry_helper_call(true, true, true);
  missing_scratch_stack_prepared.variadic_entry_plans.functions.front()
      .helper_resources.scratch_stack_bytes = std::nullopt;
  const auto& missing_scratch_stack_function_cf =
      missing_scratch_stack_prepared.control_flow.functions.front();
  const auto& missing_scratch_stack_block_cf =
      missing_scratch_stack_function_cf.blocks.front();
  const auto missing_scratch_stack_function_context =
      aarch64_codegen::make_function_lowering_context(
          missing_scratch_stack_prepared,
          missing_scratch_stack_prepared.target_profile,
          missing_scratch_stack_function_cf);
  const auto missing_scratch_stack_block_context =
      aarch64_codegen::make_block_lowering_context(
          missing_scratch_stack_function_context, missing_scratch_stack_block_cf, 0);
  aarch64_module::MachineBlock missing_scratch_stack_block;
  aarch64_module::ModuleLoweringDiagnostics missing_scratch_stack_diagnostics;
  const auto missing_scratch_stack_result = aarch64_codegen::dispatch_prepared_block(
      missing_scratch_stack_block_context,
      missing_scratch_stack_block,
      missing_scratch_stack_diagnostics);
  if (missing_scratch_stack_result.visited_operations != 1 ||
      !missing_scratch_stack_result.visited_terminator ||
      missing_scratch_stack_result.emitted_instructions != 1 ||
      missing_scratch_stack_block.instructions.size() != 1 ||
      missing_scratch_stack_diagnostics.entries.size() != 1 ||
      missing_scratch_stack_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      missing_scratch_stack_diagnostics.entries.front().message.find(
          "helper_resources.scratch_stack_bytes") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          missing_scratch_stack_block.instructions.front().target.payload)) {
    return fail("expected incomplete va_start prepared scratch-stack facts to fail closed");
  }

  auto missing_operand_home_prepared =
      prepared_with_variadic_entry_helper_call(true, true, true);
  missing_operand_home_prepared.variadic_entry_plans.functions.front()
      .helper_operand_homes.clear();
  const auto& missing_operand_home_function_cf =
      missing_operand_home_prepared.control_flow.functions.front();
  const auto& missing_operand_home_block_cf =
      missing_operand_home_function_cf.blocks.front();
  const auto missing_operand_home_function_context =
      aarch64_codegen::make_function_lowering_context(
          missing_operand_home_prepared,
          missing_operand_home_prepared.target_profile,
          missing_operand_home_function_cf);
  const auto missing_operand_home_block_context =
      aarch64_codegen::make_block_lowering_context(
          missing_operand_home_function_context, missing_operand_home_block_cf, 0);
  aarch64_module::MachineBlock missing_operand_home_block;
  aarch64_module::ModuleLoweringDiagnostics missing_operand_home_diagnostics;
  const auto missing_operand_home_result = aarch64_codegen::dispatch_prepared_block(
      missing_operand_home_block_context,
      missing_operand_home_block,
      missing_operand_home_diagnostics);
  if (missing_operand_home_result.visited_operations != 1 ||
      !missing_operand_home_result.visited_terminator ||
      missing_operand_home_result.emitted_instructions != 1 ||
      missing_operand_home_block.instructions.size() != 1 ||
      missing_operand_home_diagnostics.entries.size() != 1 ||
      missing_operand_home_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      missing_operand_home_diagnostics.entries.front().message.find(
          "helper_operand_homes") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          missing_operand_home_block.instructions.front().target.payload)) {
    return fail("expected incomplete va_start prepared operand-home facts to fail closed");
  }

  auto complete_entry_prepared = prepared_with_variadic_entry_helper_call(true, true, true);
  const auto& complete_function_cf = complete_entry_prepared.control_flow.functions.front();
  const auto& complete_block_cf = complete_function_cf.blocks.front();
  const auto complete_function_context =
      aarch64_codegen::make_function_lowering_context(
          complete_entry_prepared,
          complete_entry_prepared.target_profile,
          complete_function_cf);
  const auto complete_block_context =
      aarch64_codegen::make_block_lowering_context(
          complete_function_context, complete_block_cf, 0);
  aarch64_module::MachineBlock complete_block;
  aarch64_module::ModuleLoweringDiagnostics complete_diagnostics;
  const auto complete_result = aarch64_codegen::dispatch_prepared_block(
      complete_block_context, complete_block, complete_diagnostics);
  if (complete_result.visited_operations != 1 ||
      !complete_result.visited_terminator ||
      complete_result.emitted_instructions != 2 ||
      complete_block.instructions.size() != 2 ||
      !complete_diagnostics.entries.empty()) {
    return fail("expected complete va_start prepared storage facts to select helper record");
  }
  const auto* complete_call =
      std::get_if<aarch64_module::codegen::CallInstructionRecord>(
          &complete_block.instructions.front().target.payload);
  if (complete_call == nullptr ||
      complete_block.instructions.front().target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      complete_block.instructions.front().target.opcode !=
          aarch64_module::codegen::MachineOpcode::VariadicVaStart ||
      complete_call->source_variadic_entry == nullptr ||
      complete_call->source_variadic_helper_operand_homes == nullptr ||
      !complete_call->variadic_va_start.has_value() ||
      complete_call->source_variadic_entry->register_save_area.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{5} ||
      complete_call->source_variadic_entry->register_save_area.stack_offset_bytes !=
          std::optional<std::size_t>{16} ||
      complete_call->source_variadic_entry->overflow_area.base_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{6} ||
      complete_call->source_variadic_entry->overflow_area.base_stack_offset_bytes !=
          std::optional<std::size_t>{208} ||
      complete_call->source_variadic_entry->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{1} ||
      complete_call->source_variadic_entry->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      !complete_call->source_variadic_helper_operand_homes->destination_va_list.has_value() ||
      complete_call->source_variadic_helper_operand_homes->destination_va_list->register_name !=
          std::optional<std::string>{"x3"} ||
      complete_call->variadic_va_start->register_save_area_slot_id !=
          prepare::PreparedFrameSlotId{5} ||
      complete_call->variadic_va_start->overflow_area_base_slot_id !=
          prepare::PreparedFrameSlotId{6} ||
      complete_call->variadic_va_start->destination_va_list.register_name !=
          std::optional<std::string>{"x3"}) {
    return fail("expected selected va_start helper record to expose prepared storage, scratch, and operand-home authority");
  }

  return 0;
}

int scalar_va_arg_dispatch_reports_missing_prepared_access_plan() {
  auto prepared = prepared_with_scalar_va_arg_helper_call();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context =
      aarch64_codegen::make_function_lowering_context(
          prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);
  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      diagnostics.entries.front().message.find(
          "helper_operand_homes.va_arg.scalar_access_plan") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected scalar va_arg dispatch to report missing prepared access plan");
  }

  auto selected_prepared = prepared_with_scalar_va_arg_helper_call();
  auto& selected_entry = selected_prepared.variadic_entry_plans.functions.front();
  selected_entry.helper_operand_homes.front().scalar_access_plan =
      prepare::PreparedVariadicScalarVaArgAccessPlan{
          .source_class =
              prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea,
          .value_type = bir::TypeKind::I32,
          .value_size_bytes = 4,
          .value_align_bytes = 4,
          .result_home = selected_entry.helper_operand_homes.front().scalar_result,
          .source_field =
              prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea,
          .source_field_offset_bytes = std::size_t{16},
          .source_slot_size_bytes = std::size_t{8},
          .progression_field = prepare::PreparedVariadicVaListFieldKind::GpOffset,
          .progression_field_offset_bytes = std::size_t{0},
          .progression_stride_bytes = std::size_t{8},
          .overflow_source_field =
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          .overflow_source_field_offset_bytes = std::size_t{8},
          .overflow_stride_bytes = std::size_t{8},
      };
  const auto& selected_function_cf =
      selected_prepared.control_flow.functions.front();
  const auto& selected_block_cf = selected_function_cf.blocks.front();
  const auto selected_function_context =
      aarch64_codegen::make_function_lowering_context(
          selected_prepared, selected_prepared.target_profile, selected_function_cf);
  const auto selected_block_context =
      aarch64_codegen::make_block_lowering_context(
          selected_function_context, selected_block_cf, 0);
  aarch64_module::MachineBlock selected_block;
  aarch64_module::ModuleLoweringDiagnostics selected_diagnostics;
  const auto selected_result =
      aarch64_codegen::dispatch_prepared_block(
          selected_block_context, selected_block, selected_diagnostics);
  const auto* selected_call =
      selected_block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_module::codegen::CallInstructionRecord>(
                &selected_block.instructions.front().target.payload);
  if (selected_result.visited_operations != 1 ||
      !selected_result.visited_terminator ||
      selected_result.emitted_instructions != 2 ||
      selected_block.instructions.size() != 2 ||
      !selected_diagnostics.entries.empty() ||
      selected_call == nullptr ||
      selected_block.instructions.front().target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      selected_block.instructions.front().target.opcode !=
          aarch64_module::codegen::MachineOpcode::VariadicVaArgScalar ||
      !selected_call->variadic_scalar_va_arg.has_value() ||
      selected_call->variadic_scalar_va_arg->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea ||
      selected_call->variadic_scalar_va_arg->source_va_list.register_name !=
          std::optional<std::string>{"x3"} ||
      selected_call->variadic_scalar_va_arg->result_home.register_name !=
          std::optional<std::string>{"w0"}) {
    return fail("expected scalar va_arg dispatch to select prepared access plan");
  }

  auto fp_access_plan = *selected_entry.helper_operand_homes.front().scalar_access_plan;
  fp_access_plan.source_class =
      prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea;
  fp_access_plan.value_type = bir::TypeKind::F64;
  fp_access_plan.value_size_bytes = 8;
  fp_access_plan.value_align_bytes = 8;
  fp_access_plan.result_home =
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{24},
          .function_name = selected_entry.function_name,
          .value_name =
              selected_prepared.names.value_names.intern("%next_fp"),
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"d0"},
      };
  fp_access_plan.source_field =
      prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea;
  fp_access_plan.source_field_offset_bytes = std::size_t{24};
  fp_access_plan.source_slot_size_bytes = std::size_t{16};
  fp_access_plan.progression_field =
      prepare::PreparedVariadicVaListFieldKind::FpOffset;
  fp_access_plan.progression_field_offset_bytes = std::size_t{4};
  fp_access_plan.progression_stride_bytes = std::size_t{16};
  selected_entry.helper_operand_homes.front().scalar_result =
      *fp_access_plan.result_home;
  selected_entry.helper_operand_homes.front().scalar_access_plan = fp_access_plan;
  aarch64_module::MachineBlock selected_fp_block;
  aarch64_module::ModuleLoweringDiagnostics selected_fp_diagnostics;
  const auto selected_fp_result =
      aarch64_codegen::dispatch_prepared_block(
          selected_block_context, selected_fp_block, selected_fp_diagnostics);
  const auto* selected_fp_call =
      selected_fp_block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_module::codegen::CallInstructionRecord>(
                &selected_fp_block.instructions.front().target.payload);
  if (selected_fp_result.visited_operations != 1 ||
      !selected_fp_result.visited_terminator ||
      selected_fp_result.emitted_instructions != 2 ||
      selected_fp_block.instructions.size() != 2 ||
      !selected_fp_diagnostics.entries.empty() ||
      selected_fp_call == nullptr ||
      selected_fp_block.instructions.front().target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      selected_fp_block.instructions.front().target.opcode !=
          aarch64_module::codegen::MachineOpcode::VariadicVaArgScalar ||
      !selected_fp_call->variadic_scalar_va_arg.has_value() ||
      selected_fp_call->variadic_scalar_va_arg->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea ||
      selected_fp_call->variadic_scalar_va_arg->source_field !=
          prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea ||
      selected_fp_call->variadic_scalar_va_arg->progression_field !=
          prepare::PreparedVariadicVaListFieldKind::FpOffset ||
      selected_fp_call->variadic_scalar_va_arg->result_home.register_name !=
          std::optional<std::string>{"d0"}) {
    return fail("expected scalar fp va_arg dispatch to select prepared access plan");
  }

  return 0;
}

int aggregate_va_arg_dispatch_reports_missing_prepared_access_plan() {
  auto prepared = prepared_with_aggregate_va_arg_helper_call();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context =
      aarch64_codegen::make_function_lowering_context(
          prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);
  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      diagnostics.entries.front().message.find(
          "helper_operand_homes.va_arg_aggregate.aggregate_access_plan") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail(
        "expected aggregate va_arg dispatch to report missing prepared access plan");
  }

  auto& homes = prepared.variadic_entry_plans.functions.front().helper_operand_homes.front();
  homes.aggregate_access_plan =
      prepare::PreparedVariadicAggregateVaArgAccessPlan{
          .source_class =
              prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
          .payload_size_bytes = 24,
          .payload_align_bytes = 8,
          .destination_payload_home = homes.aggregate_destination_payload,
          .source_field =
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          .source_field_offset_bytes = std::size_t{8},
          .source_payload_offset_bytes = std::size_t{0},
          .source_slot_size_bytes = std::size_t{24},
          .copy_size_bytes = std::size_t{24},
          .copy_align_bytes = std::size_t{8},
          .progression_field =
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          .progression_field_offset_bytes = std::size_t{8},
          .progression_stride_bytes = std::size_t{24},
      };
  aarch64_module::MachineBlock selected_block;
  aarch64_module::ModuleLoweringDiagnostics selected_diagnostics;
  const auto selected_result =
      aarch64_codegen::dispatch_prepared_block(
          block_context, selected_block, selected_diagnostics);
  const auto* selected_call =
      selected_block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::CallInstructionRecord>(
                &selected_block.instructions.front().target.payload);
  if (selected_result.visited_operations != 1 ||
      !selected_result.visited_terminator ||
      selected_result.emitted_instructions != 2 ||
      selected_block.instructions.size() != 2 ||
      !selected_diagnostics.entries.empty() ||
      selected_call == nullptr ||
      selected_block.instructions.front().target.opcode !=
          aarch64_codegen::MachineOpcode::VariadicVaArgAggregate ||
      selected_block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !selected_call->variadic_aggregate_va_arg.has_value() ||
      selected_call->variadic_aggregate_va_arg->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      selected_call->variadic_aggregate_va_arg->copy_size_bytes != 24 ||
      selected_call->variadic_aggregate_va_arg->destination_payload_home.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{9}}) {
    return fail("expected aggregate va_arg dispatch to select prepared access plan");
  }

  return 0;
}

int va_copy_dispatch_selects_prepared_layout_field_copies() {
  auto prepared = prepared_with_va_copy_helper_call();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context =
      aarch64_codegen::make_function_lowering_context(
          prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock selected_block;
  aarch64_module::ModuleLoweringDiagnostics selected_diagnostics;
  const auto selected_result =
      aarch64_codegen::dispatch_prepared_block(
          block_context, selected_block, selected_diagnostics);
  const auto* selected_call =
      selected_block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::CallInstructionRecord>(
                &selected_block.instructions.front().target.payload);
  if (selected_result.visited_operations != 1 ||
      !selected_result.visited_terminator ||
      selected_result.emitted_instructions != 2 ||
      selected_block.instructions.size() != 2 ||
      !selected_diagnostics.entries.empty() ||
      selected_call == nullptr ||
      selected_block.instructions.front().target.opcode !=
          aarch64_codegen::MachineOpcode::VariadicVaCopy ||
      selected_block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !selected_call->variadic_va_copy.has_value() ||
      selected_call->variadic_va_copy->destination_va_list.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{10}} ||
      selected_call->variadic_va_copy->source_va_list.register_name !=
          std::optional<std::string>{"x4"} ||
      selected_call->variadic_va_copy->field_copies.size() != 2 ||
      selected_call->variadic_va_copy->field_copies[1].kind !=
          prepare::PreparedVariadicVaListFieldKind::OverflowArgArea ||
      selected_call->variadic_va_copy->field_copies[1].size_bytes != 8) {
    return fail("expected va_copy dispatch to select prepared layout field copies");
  }

  prepared.variadic_entry_plans.functions.front()
      .helper_operand_homes.front()
      .source_va_list.reset();
  aarch64_module::MachineBlock missing_block;
  aarch64_module::ModuleLoweringDiagnostics missing_diagnostics;
  const auto missing_result =
      aarch64_codegen::dispatch_prepared_block(
          block_context, missing_block, missing_diagnostics);
  if (missing_result.visited_operations != 1 ||
      !missing_result.visited_terminator ||
      missing_result.emitted_instructions != 1 ||
      missing_diagnostics.entries.size() != 1 ||
      missing_diagnostics.entries.front().message.find(
          "prepared source and destination va_list homes") == std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          missing_block.instructions.front().target.payload)) {
    return fail("expected va_copy dispatch to report missing prepared source home");
  }

  return 0;
}

int block_dispatch_lowers_prepared_register_argument_move_before_direct_call() {
  auto prepared = prepared_with_direct_call_argument_register_move();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 3 || block.instructions.size() != 3 ||
      !diagnostics.empty()) {
    return fail("expected prepared direct call dispatch to emit arg move, call, and return");
  }

  const auto* move =
      std::get_if<aarch64_module::codegen::CallBoundaryMoveInstructionRecord>(
          &block.instructions[0].target.payload);
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &block.instructions[1].target.payload);
  if (move == nullptr ||
      block.instructions[0].target.family !=
          aarch64_module::codegen::InstructionFamily::CallBoundary ||
      block.instructions[0].target.opcode !=
          aarch64_module::codegen::MachineOpcode::CallBoundaryMove ||
      block.instructions[0].target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      !move->source_register.has_value() || !move->destination_register.has_value() ||
      move->source_register->reg != aarch64_module::abi::x_register(2) ||
      move->destination_register->reg != aarch64_module::abi::x_register(0) ||
      move->source_bundle != &function_context.value_locations->move_bundles.front() ||
      move->source_move !=
          &function_context.value_locations->move_bundles.front().moves.front()) {
    return fail("expected prepared before-call register move to select x2 -> x0");
  }
  if (call == nullptr ||
      block.instructions[1].target.opcode !=
          aarch64_module::codegen::MachineOpcode::DirectCall ||
      !call->direct_callee.has_value() ||
      call->direct_callee_label != "actual_function") {
    return fail("expected direct call to remain after selected argument move");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions[2].target.payload)) {
    return fail("expected return terminator after selected argument move and call");
  }
  return 0;
}

int block_dispatch_lowers_prepared_register_result_move_after_direct_call() {
  auto prepared = prepared_with_direct_call_result_register_move();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 3 || block.instructions.size() != 3 ||
      !diagnostics.empty()) {
    return fail("expected prepared direct call dispatch to emit call, result move, and return");
  }

  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &block.instructions[0].target.payload);
  const auto* move =
      std::get_if<aarch64_module::codegen::CallBoundaryMoveInstructionRecord>(
          &block.instructions[1].target.payload);
  if (call == nullptr ||
      block.instructions[0].target.opcode !=
          aarch64_module::codegen::MachineOpcode::DirectCall ||
      !call->direct_callee.has_value() ||
      call->direct_callee_label != "actual_function") {
    return fail("expected direct call to remain before selected result move");
  }
  if (move == nullptr ||
      block.instructions[1].target.family !=
          aarch64_module::codegen::InstructionFamily::CallBoundary ||
      block.instructions[1].target.opcode !=
          aarch64_module::codegen::MachineOpcode::CallBoundaryMove ||
      block.instructions[1].target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      move->phase != prepare::PreparedMovePhase::AfterCall ||
      !move->source_register.has_value() || !move->destination_register.has_value() ||
      move->source_register->reg != aarch64_module::abi::x_register(0) ||
      move->destination_register->reg != aarch64_module::abi::x_register(3) ||
      move->source_bundle != &function_context.value_locations->move_bundles.front() ||
      move->source_move !=
          &function_context.value_locations->move_bundles.front().moves.front()) {
    return fail("expected prepared after-call register result move to select x0 -> x3");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions[2].target.payload)) {
    return fail("expected return terminator after selected result move");
  }
  return 0;
}

int block_dispatch_lowers_prepared_indirect_call_only_with_register_authority() {
  auto prepared = prepared_with_indirect_call_plan(true);
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      !diagnostics.empty()) {
    return fail("expected prepared register indirect call dispatch to emit call plus return");
  }

  const auto& call_instruction = block.instructions.front();
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &call_instruction.target.payload);
  if (call == nullptr ||
      call_instruction.target.family != aarch64_module::codegen::InstructionFamily::Call ||
      call_instruction.target.opcode != aarch64_module::codegen::MachineOpcode::IndirectCall ||
      call_instruction.target.function_name != function_cf.function_name ||
      call_instruction.target.block_label != block_cf.block_label ||
      call_instruction.target.instruction_index != 0 ||
      !call->is_indirect ||
      !call->indirect_callee.has_value() ||
      !call->prepared_indirect_callee.has_value() ||
      call->prepared_indirect_callee->register_name != std::optional<std::string>{"x9"} ||
      call->source_call != &function_context.call_plans->calls.front()) {
    return fail("expected indirect call node to preserve prepared register callee provenance");
  }
  const auto* callee =
      std::get_if<aarch64_module::codegen::RegisterOperand>(&call->indirect_callee->payload);
  if (call->indirect_callee->kind != aarch64_module::codegen::OperandKind::Register ||
      callee == nullptr || callee->reg != aarch64_module::abi::x_register(9) ||
      callee->role != aarch64_module::codegen::RegisterOperandRole::CallAbi ||
      callee->value_id != prepare::PreparedValueId{31}) {
    return fail("expected indirect call callee to use prepared x9 register authority");
  }

  auto stack_callee_prepared = prepared_with_indirect_call_plan(false);
  const auto& stack_function_cf = stack_callee_prepared.control_flow.functions.front();
  const auto& stack_block_cf = stack_function_cf.blocks.front();
  const auto stack_function_context = aarch64_codegen::make_function_lowering_context(
      stack_callee_prepared, stack_callee_prepared.target_profile, stack_function_cf);
  const auto stack_block_context =
      aarch64_codegen::make_block_lowering_context(stack_function_context, stack_block_cf, 0);
  aarch64_module::MachineBlock stack_block;
  aarch64_module::ModuleLoweringDiagnostics stack_diagnostics;
  const auto stack_result = aarch64_codegen::dispatch_prepared_block(
      stack_block_context, stack_block, stack_diagnostics);
  if (stack_result.visited_operations != 1 || !stack_result.visited_terminator ||
      stack_result.emitted_instructions != 1 || stack_block.instructions.size() != 1 ||
      stack_diagnostics.entries.size() != 1 ||
      stack_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      stack_diagnostics.entries.front().message.find("explicit prepared GPR callee register") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          stack_block.instructions.front().target.payload)) {
    return fail("expected stack indirect callee to fail closed without scratch selection");
  }

  return 0;
}

int block_dispatch_maps_retained_bir_by_prepared_identity_not_index() {
  auto prepared = prepared_with_reordered_retained_bir();
  const auto& function_cf = prepared.control_flow.functions[1];
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);
  if (block_context.bir_block == nullptr ||
      block_context.bir_block->label != "second.entry") {
    return fail("expected block context to map retained BIR by prepared identity");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1) {
    return fail("expected retained BIR dispatch to visit the identity-mapped block instruction");
  }
  if (!result.visited_terminator) {
    return fail("expected retained BIR dispatch to visit the prepared terminator");
  }
  if (result.emitted_instructions != 1 || block.instructions.size() != 1) {
    return fail("expected retained BIR dispatch to emit one return instruction");
  }
  if (diagnostics.entries.size() != 1) {
    return fail("expected identity-mapped retained BIR dispatch to record operation diagnostic only");
  }
  if (diagnostics.entries[0].kind !=
      aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily) {
    return fail("expected identity-mapped retained BIR dispatch instruction diagnostic");
  }
  if (diagnostics.entries[0].instruction_family !=
      aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected identity-mapped retained BIR dispatch to classify the instruction");
  }
  if (diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected identity-mapped retained BIR dispatch diagnostic block identity");
  }
  return 0;
}

int missing_bir_block_mapping_is_diagnostic_only() {
  auto prepared = prepared_with_unsupported_instructions();
  prepared.module.functions.front().blocks.front().label_id = c4c::kInvalidBlockLabel;
  prepared.module.functions.front().blocks.front().label = "dispatch.unmatched";

  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1) {
    return fail("expected missing mapping to skip operations but still lower return terminator");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping ||
      diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected typed missing block mapping diagnostic");
  }
  return 0;
}

int module_build_dispatch_scaffold_lowers_return_and_keeps_machine_nodes_empty() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared module with dispatch scaffold to build");
  }

  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 1 || function.blocks.front().instructions.size() != 1) {
    return fail("expected dispatch scaffold to emit one return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          function.blocks.front().instructions.front().target.payload)) {
    return fail("expected module build to preserve canonical return target");
  }
  for (const auto& record : result.module->functions) {
    if (!record.machine_nodes.empty()) {
      return fail("expected dispatch scaffold not to fake flat compatibility machine nodes");
    }
  }
  return 0;
}

int module_build_lowers_simple_fixed_frame_around_function_stream() {
  auto prepared = prepared_with_simple_fixed_frame();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared module with simple fixed frame to build");
  }

  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 1 || function.blocks.front().instructions.size() != 3) {
    return fail("expected simple fixed frame to emit setup, teardown, and return");
  }
  const auto& block = function.blocks.front();
  const auto& setup_instruction = block.instructions[0];
  const auto& teardown_instruction = block.instructions[1];
  const auto& return_instruction = block.instructions[2];
  const auto* setup =
      std::get_if<aarch64_module::codegen::FrameInstructionRecord>(
          &setup_instruction.target.payload);
  const auto* teardown =
      std::get_if<aarch64_module::codegen::FrameInstructionRecord>(
          &teardown_instruction.target.payload);
  if (setup == nullptr || teardown == nullptr ||
      setup_instruction.target.family !=
          aarch64_module::codegen::InstructionFamily::Frame ||
      teardown_instruction.target.family !=
          aarch64_module::codegen::InstructionFamily::Frame ||
      setup_instruction.target.opcode !=
          aarch64_module::codegen::MachineOpcode::FrameSetup ||
      teardown_instruction.target.opcode !=
          aarch64_module::codegen::MachineOpcode::FrameTeardown ||
      setup->source_frame != &prepared.frame_plan.functions.front() ||
      teardown->source_frame != &prepared.frame_plan.functions.front() ||
      setup->frame_size_bytes != 32 || teardown->frame_size_bytes != 32 ||
      setup->frame_alignment_bytes != 16 || teardown->frame_alignment_bytes != 16 ||
      setup_instruction.target.function_name !=
          prepared.control_flow.functions.front().function_name ||
      setup_instruction.target.block_label != block.block_label ||
      teardown_instruction.target.block_label != block.block_label ||
      !setup_instruction.origin.has_value() || !teardown_instruction.origin.has_value() ||
      setup_instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::Synthetic ||
      teardown_instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::Synthetic) {
    return fail("expected fixed-frame setup/teardown to preserve prepared frame provenance");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          return_instruction.target.payload)) {
    return fail("expected fixed-frame teardown to be inserted before return");
  }

  auto dynamic_frame = prepared_with_simple_fixed_frame();
  dynamic_frame.frame_plan.functions.front().has_dynamic_stack = true;
  const auto dynamic_result = aarch64_api::build_prepared_module(dynamic_frame);
  if (dynamic_result.error.has_value() || !dynamic_result.module.has_value() ||
      dynamic_result.module->mir.functions.front().blocks.front().instructions.size() != 1 ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          dynamic_result.module->mir.functions.front()
              .blocks.front()
              .instructions.front()
              .target.payload)) {
    return fail("expected dynamic-stack frame to remain explicitly deferred");
  }

  auto callee_save_frame = prepared_with_simple_fixed_frame();
  callee_save_frame.frame_plan.functions.front().saved_callee_registers.push_back(
      prepare::PreparedSavedRegister{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_name = "x19",
          .contiguous_width = 1,
          .occupied_register_names = {"x19"},
          .save_index = 0,
      });
  const auto callee_save_result = aarch64_api::build_prepared_module(callee_save_frame);
  if (callee_save_result.error.has_value() || !callee_save_result.module.has_value() ||
      callee_save_result.module->mir.functions.front().blocks.front().instructions.size() != 1 ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          callee_save_result.module->mir.functions.front()
              .blocks.front()
              .instructions.front()
              .target.payload)) {
    return fail("expected callee-save frame to remain explicitly deferred");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status =
          block_dispatch_visits_prepared_terminator_without_bir_block_mapping();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_visits_prepared_instructions_in_order_and_fails_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_direct_call_without_reclassifying_abi();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_exposes_prepared_memory_return_storage_on_call_node();
      status != 0) {
    return status;
  }
  if (const int status =
          variadic_entry_helper_dispatch_requires_complete_prepared_entry_plan();
      status != 0) {
    return status;
  }
  if (const int status =
          scalar_va_arg_dispatch_reports_missing_prepared_access_plan();
      status != 0) {
    return status;
  }
  if (const int status =
          aggregate_va_arg_dispatch_reports_missing_prepared_access_plan();
      status != 0) {
    return status;
  }
  if (const int status = va_copy_dispatch_selects_prepared_layout_field_copies();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_register_argument_move_before_direct_call();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_register_result_move_after_direct_call();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_indirect_call_only_with_register_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_maps_retained_bir_by_prepared_identity_not_index();
      status != 0) {
    return status;
  }
  if (const int status = missing_bir_block_mapping_is_diagnostic_only();
      status != 0) {
    return status;
  }
  if (const int status = module_build_dispatch_scaffold_lowers_return_and_keeps_machine_nodes_empty();
      status != 0) {
    return status;
  }
  if (const int status = module_build_lowers_simple_fixed_frame_around_function_stream();
      status != 0) {
    return status;
  }
  return 0;
}
