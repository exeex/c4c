#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/calls.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/machine_printer.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/mir/printer.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

mir::MachinePrintResult print_route_block(
    c4c::FunctionNameId function_name,
    const aarch64_module::MachineBlock& block) {
  aarch64_module::MachineFunction function;
  function.function_name = function_name;
  function.blocks.push_back(block);
  return mir::print_machine_function(
      function, aarch64_codegen::MachineInstructionPrinter{});
}

prepare::PreparedF128Carrier dispatch_f128_register_carrier(
    c4c::FunctionNameId function_name,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const char* register_name);

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
                  .stack_size_bytes = std::size_t{16},
                  .stack_align_bytes = std::size_t{8},
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

prepare::PreparedBirModule prepared_with_dynamic_stack_helper_calls() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.dynamic_stack");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.dynamic_stack.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.dynamic_stack.entry");
  const auto saved_ptr_name = prepared.names.value_names.intern("%saved.sp");
  const auto count_name = prepared.names.value_names.intern("%count");
  const auto allocated_ptr_name = prepared.names.value_names.intern("%vla.ptr");
  constexpr prepare::PreparedValueId kSavedPtrValue{41};
  constexpr prepare::PreparedValueId kCountValue{42};
  constexpr prepare::PreparedValueId kAllocatedPtrValue{43};

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.dynamic_stack",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.dynamic_stack.entry",
          .insts = {bir::CallInst{
                        .result = bir::Value::named(bir::TypeKind::Ptr, "%saved.sp"),
                        .callee = "llvm.stacksave",
                        .return_type = bir::TypeKind::Ptr,
                    },
                    bir::CallInst{
                        .result = bir::Value::named(bir::TypeKind::Ptr, "%vla.ptr"),
                        .callee = "llvm.dynamic_alloca.i8",
                        .args = {bir::Value::named(bir::TypeKind::I64, "%count")},
                        .arg_types = {bir::TypeKind::I64},
                        .return_type = bir::TypeKind::Ptr,
                    },
                    bir::CallInst{
                        .callee = "llvm.stackrestore",
                        .args = {bir::Value::named(bir::TypeKind::Ptr, "%saved.sp")},
                        .arg_types = {bir::TypeKind::Ptr},
                        .return_type = bir::TypeKind::Void,
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
  prepared.dynamic_stack_plan.functions.push_back(prepare::PreparedDynamicStackPlanFunction{
      .function_name = function_name,
      .requires_stack_save_restore = true,
      .operations =
          {prepare::PreparedDynamicStackOp{
               .function_name = function_name,
               .block_label = entry_label,
               .instruction_index = 0,
               .kind = prepare::PreparedDynamicStackOpKind::StackSave,
               .result_value_name = saved_ptr_name,
           },
           prepare::PreparedDynamicStackOp{
               .function_name = function_name,
               .block_label = entry_label,
               .instruction_index = 1,
               .kind = prepare::PreparedDynamicStackOpKind::DynamicAlloca,
               .result_value_name = allocated_ptr_name,
               .operand_value_name = count_name,
               .allocation_type_text = "i8",
               .element_size_bytes = 1,
               .element_align_bytes = 1,
           },
           prepare::PreparedDynamicStackOp{
               .function_name = function_name,
               .block_label = entry_label,
               .instruction_index = 2,
               .kind = prepare::PreparedDynamicStackOpKind::StackRestore,
               .operand_value_name = saved_ptr_name,
           }},
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 16,
      .has_dynamic_stack = true,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = kSavedPtrValue,
                  .function_name = function_name,
                  .value_name = saved_ptr_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x20"},
                  .target_register_identity =
                      prepare::PreparedTargetRegisterIdentity{
                          .target_arch = c4c::TargetArch::Aarch64,
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .register_class = prepare::PreparedRegisterClass::General,
                          .physical_index = 20,
                      },
              },
              prepare::PreparedValueHome{
                  .value_id = kCountValue,
                  .function_name = function_name,
                  .value_name = count_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x0"},
                  .target_register_identity =
                      prepare::PreparedTargetRegisterIdentity{
                          .target_arch = c4c::TargetArch::Aarch64,
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .register_class = prepare::PreparedRegisterClass::General,
                          .physical_index = 0,
                      },
              },
              prepare::PreparedValueHome{
                  .value_id = kAllocatedPtrValue,
                  .function_name = function_name,
                  .value_name = allocated_ptr_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x13"},
                  .target_register_identity =
                      prepare::PreparedTargetRegisterIdentity{
                          .target_arch = c4c::TargetArch::Aarch64,
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .register_class = prepare::PreparedRegisterClass::General,
                          .physical_index = 13,
                      },
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
                    .block_index = 0,
                    .instruction_index = 0,
                    .wrapper_kind =
                        prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                    .direct_callee_name = std::string{"llvm.stacksave"},
                },
                prepare::PreparedCallPlan{
                    .block_index = 0,
                    .instruction_index = 1,
                    .wrapper_kind =
                        prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                    .direct_callee_name = std::string{"llvm.dynamic_alloca.i8"},
                    .arguments = {prepare::PreparedCallArgumentPlan{
                        .instruction_index = 1,
                        .arg_index = 0,
                        .value_bank = prepare::PreparedRegisterBank::Gpr,
                        .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                        .source_value_id = kCountValue,
                        .source_register_name = std::string{"x0"},
                        .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                        .destination_register_name = std::string{"x0"},
                        .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                    }},
                },
                prepare::PreparedCallPlan{
                    .block_index = 0,
                    .instruction_index = 2,
                    .wrapper_kind =
                        prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                    .direct_callee_name = std::string{"llvm.stackrestore"},
                    .arguments = {prepare::PreparedCallArgumentPlan{
                        .instruction_index = 2,
                        .arg_index = 0,
                        .value_bank = prepare::PreparedRegisterBank::Gpr,
                        .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                        .source_value_id = kSavedPtrValue,
                        .source_register_name = std::string{"x20"},
                        .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                        .destination_register_name = std::string{"x0"},
                        .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                    }},
                }},
  });
  return prepared;
}

enum class InlineAsmCarrierFixtureKind {
  Complete,
  SupportedTemplateModifier,
  NamedReferences,
  Incomplete,
  MissingResultHome,
  MissingTiedInputHome,
  MissingTiedOutputIndex,
  AllocatorDependentTiedInputHome,
  TargetInvalidTiedInputHome,
  ClassInvalidTiedInputHome,
  MismatchedTiedInputHome,
  MissingTiedHomeAuthority,
  AuthorityOutputMismatchedTiedInputHome,
  AuthorityMismatchedTiedInputHome,
  AliasAwareTiedInputHome,
  UnsupportedOperand,
  SupportedMemoryInputSelection,
  SupportedAddressInputSelection,
  UnsupportedMemoryInputSelection,
  UnsupportedAddressInputSelection,
  UnsupportedClobber,
  UnsupportedTemplateModifier,
  SupportedClobbers,
  UnknownClobber,
  MalformedClobber,
  TargetInvalidClobber,
};

prepare::PreparedValueHome inline_asm_register_home(
    prepare::PreparedValueId value_id,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name,
    std::string register_name) {
  std::optional<prepare::PreparedTargetRegisterIdentity> identity;
  if (register_name.size() >= 2 &&
      (register_name.front() == 'w' || register_name.front() == 'x')) {
    identity = prepare::PreparedTargetRegisterIdentity{
        .target_arch = c4c::TargetArch::Aarch64,
        .bank = prepare::PreparedRegisterBank::Gpr,
        .register_class = prepare::PreparedRegisterClass::General,
        .physical_index =
            static_cast<std::size_t>(std::stoul(register_name.substr(1))),
    };
  }
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::move(register_name),
      .target_register_identity = identity,
  };
}

prepare::PreparedInlineAsmTiedHomeAuthority inline_asm_tied_home_authority(
    std::size_t tied_output_index,
    std::size_t physical_index) {
  return prepare::PreparedInlineAsmTiedHomeAuthority{
      .tied_output_index = tied_output_index,
      .shared_register =
          prepare::PreparedTargetRegisterIdentity{
              .target_arch = c4c::TargetArch::Aarch64,
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_class = prepare::PreparedRegisterClass::General,
              .physical_index = physical_index,
          },
  };
}

prepare::PreparedValueHome inline_asm_immediate_home(
    prepare::PreparedValueId value_id,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name,
    std::int64_t value) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
      .immediate_i32 = value,
  };
}

prepare::PreparedBirModule prepared_with_inline_asm_carrier(
    InlineAsmCarrierFixtureKind kind) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.inline_asm");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.inline_asm.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.inline_asm.entry");
  const auto out_name = prepared.names.value_names.intern("%out");
  const auto seed_name = prepared.names.value_names.intern("%seed");
  const auto input_name = prepared.names.value_names.intern("%input");
  const auto imm_name = prepared.names.value_names.intern("7");

  const auto out_value = bir::Value::named(bir::TypeKind::I32, "%out");
  const auto seed_value = bir::Value::named(bir::TypeKind::I32, "%seed");
  const auto input_value =
      kind == InlineAsmCarrierFixtureKind::SupportedMemoryInputSelection ||
              kind == InlineAsmCarrierFixtureKind::SupportedAddressInputSelection
          ? bir::Value::named(bir::TypeKind::Ptr, "%input")
          : bir::Value::named(bir::TypeKind::I32, "%input");
  const auto imm_value = bir::Value::immediate_i32(7);

  auto selected_input_kind = bir::InlineAsmOperandKind::RegisterInput;
  std::string selected_input_constraint = "r";
  if (kind == InlineAsmCarrierFixtureKind::SupportedMemoryInputSelection) {
    selected_input_kind = bir::InlineAsmOperandKind::MemoryInput;
    selected_input_constraint = "m";
  } else if (kind == InlineAsmCarrierFixtureKind::SupportedAddressInputSelection) {
    selected_input_kind = bir::InlineAsmOperandKind::AddressInput;
    selected_input_constraint = "p";
  } else if (kind == InlineAsmCarrierFixtureKind::UnsupportedOperand) {
    selected_input_kind = bir::InlineAsmOperandKind::Unsupported;
  } else if (kind == InlineAsmCarrierFixtureKind::UnsupportedMemoryInputSelection) {
    selected_input_kind = bir::InlineAsmOperandKind::MemoryInput;
    selected_input_constraint = "m";
  } else if (kind == InlineAsmCarrierFixtureKind::UnsupportedAddressInputSelection) {
    selected_input_kind = bir::InlineAsmOperandKind::AddressInput;
    selected_input_constraint = "p";
  } else if (kind == InlineAsmCarrierFixtureKind::UnsupportedClobber) {
    selected_input_kind = bir::InlineAsmOperandKind::Clobber;
    selected_input_constraint = "~{x1}";
  }

  bir::InlineAsmMetadata inline_asm{
      .asm_text = kind == InlineAsmCarrierFixtureKind::NamedReferences
                      ? "add %w0, %w1, %[rhs]\nmov %x0, #%[imm]"
                      : kind == InlineAsmCarrierFixtureKind::SupportedMemoryInputSelection
                      ? "ldr %w0, %2"
                  : kind == InlineAsmCarrierFixtureKind::SupportedAddressInputSelection
                      ? "adr %x0, %2"
                  : kind == InlineAsmCarrierFixtureKind::AliasAwareTiedInputHome
                      ? "add %w0, %x1, %w2"
                  : kind == InlineAsmCarrierFixtureKind::SupportedTemplateModifier
                      ? "add %w0, %w1, %w2\nmov %x0, %x1"
                      : kind == InlineAsmCarrierFixtureKind::UnsupportedTemplateModifier
                            ? "add %q0, %w1, %w2"
                            : "add %0, %1, %2",
      .constraints = "=r,0,r,i",
      .side_effects = true,
      .operands =
          {bir::InlineAsmOperandMetadata{
               .kind = bir::InlineAsmOperandKind::RegisterOutput,
               .constraint_index = 0,
               .constraint = "=r",
               .output_index = std::size_t{0},
               .name = std::string{"dst"},
           },
           bir::InlineAsmOperandMetadata{
               .kind = bir::InlineAsmOperandKind::TiedInput,
               .constraint_index = 1,
               .constraint = "0",
               .arg_index = std::size_t{0},
               .tied_output_index = std::size_t{0},
               .name = std::string{"seed"},
           },
           bir::InlineAsmOperandMetadata{
               .kind = selected_input_kind,
               .constraint_index = 2,
               .constraint = selected_input_constraint,
               .arg_index = std::size_t{1},
               .name = std::string{"rhs"},
           },
           bir::InlineAsmOperandMetadata{
               .kind = bir::InlineAsmOperandKind::IntegerImmediateInput,
               .constraint_index = 3,
               .constraint = "i",
               .arg_index = std::size_t{2},
               .name = std::string{"imm"},
           }},
      .unsupported_facts =
          kind == InlineAsmCarrierFixtureKind::UnsupportedTemplateModifier
              ? std::vector<std::string>{"unsupported_template_modifiers"}
              : std::vector<std::string>{},
      .has_named_operand_references =
          kind == InlineAsmCarrierFixtureKind::NamedReferences,
      .has_template_modifiers =
          kind == InlineAsmCarrierFixtureKind::NamedReferences ||
          kind == InlineAsmCarrierFixtureKind::AliasAwareTiedInputHome ||
          kind == InlineAsmCarrierFixtureKind::SupportedTemplateModifier ||
          kind == InlineAsmCarrierFixtureKind::UnsupportedTemplateModifier,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.inline_asm",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "dispatch.inline_asm.entry",
          .insts = {bir::CallInst{
              .result = out_value,
              .callee = "llvm.inline_asm",
              .args = {seed_value, input_value, imm_value},
              .arg_types = {bir::TypeKind::I32, input_value.type, bir::TypeKind::I32},
              .return_type = bir::TypeKind::I32,
              .inline_asm = inline_asm,
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

  auto out_home = inline_asm_register_home(
      prepare::PreparedValueId{50}, function_name, out_name, "w3");
  auto seed_home = inline_asm_register_home(
      prepare::PreparedValueId{51}, function_name, seed_name, "w3");
  if (kind == InlineAsmCarrierFixtureKind::AllocatorDependentTiedInputHome) {
    seed_home.register_name = std::nullopt;
  } else if (kind == InlineAsmCarrierFixtureKind::TargetInvalidTiedInputHome) {
    seed_home.register_name = std::string{"sp"};
    seed_home.target_register_identity = std::nullopt;
  } else if (kind == InlineAsmCarrierFixtureKind::ClassInvalidTiedInputHome) {
    seed_home.register_name = std::string{"s3"};
    seed_home.target_register_identity =
        prepare::PreparedTargetRegisterIdentity{
            .target_arch = c4c::TargetArch::Aarch64,
            .bank = prepare::PreparedRegisterBank::Fpr,
            .register_class = prepare::PreparedRegisterClass::Float,
            .physical_index = 3,
        };
  } else if (kind == InlineAsmCarrierFixtureKind::MismatchedTiedInputHome) {
    seed_home.register_name = std::string{"w4"};
    seed_home.target_register_identity =
        prepare::PreparedTargetRegisterIdentity{
            .target_arch = c4c::TargetArch::Aarch64,
            .bank = prepare::PreparedRegisterBank::Gpr,
            .register_class = prepare::PreparedRegisterClass::General,
            .physical_index = 4,
        };
  } else if (kind == InlineAsmCarrierFixtureKind::AliasAwareTiedInputHome) {
    seed_home.register_name = std::string{"x3"};
  }
  std::optional<prepare::PreparedValueHome> seed_operand_home{seed_home};
  if (kind == InlineAsmCarrierFixtureKind::MissingTiedInputHome) {
    seed_operand_home = std::nullopt;
  }
  std::optional<std::size_t> tied_output_index{std::size_t{0}};
  if (kind == InlineAsmCarrierFixtureKind::MissingTiedOutputIndex) {
    tied_output_index = std::nullopt;
  }
  const auto input_home =
      inline_asm_register_home(prepare::PreparedValueId{52},
                               function_name,
                               input_name,
                               kind == InlineAsmCarrierFixtureKind::SupportedMemoryInputSelection ||
                                       kind == InlineAsmCarrierFixtureKind::SupportedAddressInputSelection
                                   ? "x5"
                                   : "w5");
  const auto imm_home = inline_asm_immediate_home(
      prepare::PreparedValueId{53}, function_name, imm_name, 7);
  auto carrier_kind = prepare::PreparedInlineAsmCarrierKind::Complete;
  std::vector<std::string> missing_facts;
  if (kind == InlineAsmCarrierFixtureKind::Incomplete) {
    carrier_kind = prepare::PreparedInlineAsmCarrierKind::Missing;
    missing_facts = {"missing_result_home"};
  } else if (kind == InlineAsmCarrierFixtureKind::UnsupportedTemplateModifier) {
    carrier_kind = prepare::PreparedInlineAsmCarrierKind::Missing;
    missing_facts = {"unsupported_template_modifiers"};
  }

  prepare::PreparedInlineAsmCarrier carrier{
      .function_name = function_name,
      .carrier_kind = carrier_kind,
      .block_index = 0,
      .inst_index = 0,
      .asm_text = inline_asm.asm_text,
      .constraints = inline_asm.constraints,
      .side_effects = inline_asm.side_effects,
      .has_named_operand_references = inline_asm.has_named_operand_references,
      .has_template_modifiers = inline_asm.has_template_modifiers,
      .operands =
          {prepare::PreparedInlineAsmOperand{
               .kind = bir::InlineAsmOperandKind::RegisterOutput,
               .constraint_index = 0,
               .constraint = "=r",
               .output_index = std::size_t{0},
           },
           prepare::PreparedInlineAsmOperand{
               .kind = bir::InlineAsmOperandKind::TiedInput,
               .constraint_index = 1,
               .constraint = "0",
               .arg_index = std::size_t{0},
               .tied_output_index = tied_output_index,
               .value = seed_value,
               .value_name = seed_name,
               .home = seed_operand_home,
               .tied_home_authority =
                   kind == InlineAsmCarrierFixtureKind::MismatchedTiedInputHome ||
                           kind == InlineAsmCarrierFixtureKind::AllocatorDependentTiedInputHome ||
                           kind == InlineAsmCarrierFixtureKind::TargetInvalidTiedInputHome ||
                           kind == InlineAsmCarrierFixtureKind::ClassInvalidTiedInputHome ||
                           kind == InlineAsmCarrierFixtureKind::MissingTiedInputHome ||
                           kind == InlineAsmCarrierFixtureKind::MissingTiedOutputIndex ||
                           kind == InlineAsmCarrierFixtureKind::MissingTiedHomeAuthority
                       ? std::nullopt
                       : std::optional<prepare::PreparedInlineAsmTiedHomeAuthority>{
                             inline_asm_tied_home_authority(
                                 kind ==
                                         InlineAsmCarrierFixtureKind::
                                             AuthorityOutputMismatchedTiedInputHome
                                     ? 1
                                     : 0,
                                 kind == InlineAsmCarrierFixtureKind::AuthorityMismatchedTiedInputHome
                                     ? 4
                                     : 3)},
           },
           prepare::PreparedInlineAsmOperand{
               .kind = selected_input_kind,
               .constraint_index = 2,
               .constraint = selected_input_constraint,
               .arg_index = std::size_t{1},
               .value = input_value,
               .value_name = input_name,
               .home = input_home,
               .memory_address =
                   kind == InlineAsmCarrierFixtureKind::SupportedMemoryInputSelection
                       ? std::optional<bir::MemoryAddress>{bir::MemoryAddress{
                             .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                             .base_value = input_value,
                             .byte_offset = 8,
                             .size_bytes = 4,
                             .align_bytes = 4,
                         }}
                       : std::nullopt,
               .address =
                   kind == InlineAsmCarrierFixtureKind::SupportedAddressInputSelection
                       ? std::optional<bir::MemoryAddress>{bir::MemoryAddress{
                             .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                             .base_value = input_value,
                             .byte_offset = 16,
                             .size_bytes = 8,
                             .align_bytes = 8,
                         }}
                       : std::nullopt,
           },
           prepare::PreparedInlineAsmOperand{
               .kind = bir::InlineAsmOperandKind::IntegerImmediateInput,
               .constraint_index = 3,
               .constraint = "i",
               .arg_index = std::size_t{2},
               .value = imm_value,
               .value_name = imm_name,
               .home = imm_home,
               .immediate_value = std::int64_t{7},
           }},
      .clobbers =
          kind == InlineAsmCarrierFixtureKind::SupportedClobbers
              ? std::vector<std::string>{"x7", "memory", "cc"}
          : kind == InlineAsmCarrierFixtureKind::UnknownClobber
              ? std::vector<std::string>{"v0"}
          : kind == InlineAsmCarrierFixtureKind::MalformedClobber
              ? std::vector<std::string>{"~{x7}"}
          : kind == InlineAsmCarrierFixtureKind::TargetInvalidClobber
              ? std::vector<std::string>{"sp"}
              : std::vector<std::string>{},
      .result = out_value,
      .result_value_name = out_name,
      .result_home = kind == InlineAsmCarrierFixtureKind::MissingResultHome
                         ? std::nullopt
                         : std::optional<prepare::PreparedValueHome>{out_home},
      .missing_required_facts = missing_facts,
  };

  prepared.inline_asm_carriers.functions.push_back(
      prepare::PreparedInlineAsmCarrierFunction{
          .function_name = function_name,
          .carriers = {std::move(carrier)},
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

prepare::PreparedBirModule prepared_with_load_global_call_argument(
    bir::GlobalAddressMaterializationPolicy policy) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const bool got_required =
      policy == bir::GlobalAddressMaterializationPolicy::GotRequired;
  const std::string function_spelling =
      got_required ? "dispatch.call.got.global.arg"
                   : "dispatch.call.direct.global.arg";
  const std::string entry_spelling = function_spelling + ".entry";
  const std::string global_spelling =
      got_required ? "external_data_symbol" : "internal_data_symbol";

  const auto function_name = prepared.names.function_names.intern(function_spelling);
  const auto entry_label = prepared.names.block_labels.intern(entry_spelling);
  const auto bir_entry_label = prepared.module.names.block_labels.intern(entry_spelling);
  const auto prepared_global_link = prepared.names.link_names.intern(global_spelling);
  const auto bir_global_link = prepared.module.names.link_names.intern(global_spelling);
  const auto callee_link = prepared.names.link_names.intern("consume_ptr");
  const auto loaded_value_name = prepared.names.value_names.intern("%loaded.global");
  constexpr prepare::PreparedValueId kLoadedValue{41};

  prepared.module.globals.push_back(bir::Global{
      .name = global_spelling,
      .link_name_id = bir_global_link,
      .type = bir::TypeKind::Ptr,
      .is_extern = got_required,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_materialization_policy = policy,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = function_spelling,
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = entry_spelling,
          .insts = {bir::LoadGlobalInst{
                        .result = bir::Value::named(bir::TypeKind::Ptr, "%loaded.global"),
                        .global_name = global_spelling,
                        .global_name_id = bir_global_link,
                        .align_bytes = 8,
                    },
                    bir::CallInst{
                        .callee = "consume_ptr",
                        .callee_link_name_id = callee_link,
                        .args = {bir::Value::named(bir::TypeKind::Ptr, "%loaded.global")},
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
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = kLoadedValue,
                  .function_name = function_name,
                  .value_name = loaded_value_name,
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
                  .instruction_index = 1,
                  .moves =
                      {
                          prepare::PreparedMoveResolution{
                              .from_value_id = kLoadedValue,
                              .to_value_id = kLoadedValue,
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{0},
                              .destination_register_name = std::string{"x0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                              .block_index = 0,
                              .instruction_index = 1,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .reason = "call_arg_loaded_global_to_register",
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
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = kLoadedValue,
                  .value_name = loaded_value_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"x2"},
                  .occupied_register_names = {"x2"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 1,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"consume_ptr"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 1,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = kLoadedValue,
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
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = entry_label,
                  .inst_index = 0,
                  .result_value_name = loaded_value_name,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                          .symbol_name = prepared_global_link,
                          .size_bytes = 8,
                          .align_bytes = 8,
                          .can_use_base_plus_offset = true,
                      },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_variadic_call_symbol_address_argument() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.call.symbol.arg");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.call.symbol.arg.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.call.symbol.arg.entry");
  const auto printf_link = prepared.names.link_names.intern("printf");
  const auto str_value_name = prepared.names.value_names.intern("@.str0");
  const auto str_arg = bir::Value::named(bir::TypeKind::Ptr, "@.str0");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call.symbol.arg",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.symbol.arg.entry",
          .insts = {bir::CallInst{
              .callee = "printf",
              .callee_link_name_id = printf_link,
              .args = {str_arg},
              .arg_types = {bir::TypeKind::Ptr},
              .return_type = bir::TypeKind::I32,
              .calling_convention = bir::CallingConv::C,
              .is_variadic = true,
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
                  .value_id = prepare::PreparedValueId{1},
                  .function_name = function_name,
                  .value_name = str_value_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x21"},
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
                              .from_value_id = prepare::PreparedValueId{1},
                              .to_value_id = prepare::PreparedValueId{1},
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
                              .reason = "call_arg_symbol_address_to_register",
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
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternVariadic,
          .direct_callee_name = std::string{"printf"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_symbol_name = std::string{"@.str0"},
                      .source_register_name = std::string{"x21"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"x0"},
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_register_placement = prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                          .slot_index = 1,
                          .contiguous_width = 1,
                      },
                  },
              },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_variadic_call_stack_symbol_address_argument() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("dispatch.call.stack.symbol.arg");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.call.stack.symbol.arg.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.call.stack.symbol.arg.entry");
  const auto printf_link = prepared.names.link_names.intern("printf");
  const auto fmt_value_name = prepared.names.value_names.intern("@.fmt");
  const auto payload_value_name = prepared.names.value_names.intern("@.payload");
  const auto fmt_text = prepared.names.texts.intern(".fmt");
  const auto payload_text = prepared.names.texts.intern(".payload");
  const auto fmt_arg = bir::Value::named(bir::TypeKind::Ptr, "@.fmt");
  const auto payload_arg = bir::Value::named(bir::TypeKind::Ptr, "@.payload");

  prepared.module.names.texts.intern(".fmt");
  prepared.module.names.texts.intern(".payload");
  prepared.module.string_constants.push_back(
      bir::StringConstant{.name = ".fmt", .name_id = fmt_text, .bytes = "%s\n"});
  prepared.module.string_constants.push_back(
      bir::StringConstant{.name = ".payload", .name_id = payload_text, .bytes = "ok"});
  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call.stack.symbol.arg",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.stack.symbol.arg.entry",
          .insts = {bir::CallInst{
              .callee = "printf",
              .callee_link_name_id = printf_link,
              .args = {fmt_arg, payload_arg},
              .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
              .return_type = bir::TypeKind::I32,
              .calling_convention = bir::CallingConv::C,
              .is_variadic = true,
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
                  .value_id = prepare::PreparedValueId{1},
                  .function_name = function_name,
                  .value_name = fmt_value_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x13"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{2},
                  .function_name = function_name,
                  .value_name = payload_value_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{0},
                  .offset_bytes = 0,
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
                              .from_value_id = prepare::PreparedValueId{1},
                              .to_value_id = prepare::PreparedValueId{1},
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
                              .reason = "call_arg_symbol_address_to_register",
                          },
                          prepare::PreparedMoveResolution{
                              .from_value_id = prepare::PreparedValueId{2},
                              .to_value_id = prepare::PreparedValueId{2},
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{1},
                              .destination_register_name = std::string{"x1"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x1"},
                              .block_index = 0,
                              .instruction_index = 0,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .reason = "call_arg_stack_to_register",
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
                          prepare::PreparedAbiBinding{
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{1},
                              .destination_register_name = std::string{"x1"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x1"},
                          },
                      },
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{1},
                  .value_name = fmt_value_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"x13"},
                  .register_placement = prepare::PreparedRegisterPlacement{
                      .bank = prepare::PreparedRegisterBank::Gpr,
                      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
                      .slot_index = 0,
                      .contiguous_width = 1,
                  },
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{2},
                  .value_name = payload_value_name,
                  .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .slot_id = prepare::PreparedFrameSlotId{0},
                  .stack_offset_bytes = 0,
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternVariadic,
          .direct_callee_name = std::string{"printf"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_symbol_name = std::string{"@.fmt"},
                      .source_register_name = std::string{"x13"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"x0"},
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
                      .source_value_id = prepare::PreparedValueId{2},
                      .source_symbol_name = std::string{"@.payload"},
                      .destination_register_name = std::string{"x1"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"x1"},
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_placement = prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CallArgument,
                          .slot_index = 1,
                          .contiguous_width = 1,
                      },
                  },
              },
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .address_materializations =
          {
              prepare::PreparedAddressMaterialization{
                  .function_name = function_name,
                  .block_label = entry_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::StringConstant,
                  .result_value_name = fmt_value_name,
                  .text_name = fmt_text,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = function_name,
                  .block_label = entry_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::StringConstant,
                  .result_value_name = payload_value_name,
                  .text_name = payload_text,
              },
          },
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

prepare::PreparedBirModule prepared_with_direct_call_f128_argument_register_move() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.call.f128.arg");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.call.f128.arg.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.call.f128.arg.entry");
  const auto actual_link = prepared.names.link_names.intern("actual_f128_function");
  const auto arg_name = prepared.names.value_names.intern("%f128.arg");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call.f128.arg",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.f128.arg.entry",
          .insts = {bir::CallInst{
              .callee = "actual_f128_function",
              .callee_link_name_id = actual_link,
              .args = {bir::Value::named(bir::TypeKind::F128, "%f128.arg")},
              .arg_types = {bir::TypeKind::F128},
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
                  .value_id = prepare::PreparedValueId{141},
                  .function_name = function_name,
                  .value_name = arg_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"q2"},
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
                              .from_value_id = prepare::PreparedValueId{141},
                              .to_value_id = prepare::PreparedValueId{141},
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{0},
                              .destination_register_name = std::string{"q0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"q0"},
                              .block_index = 0,
                              .instruction_index = 0,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .reason = "f128_call_arg_q_register_to_q_register",
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
                              .destination_register_name = std::string{"q0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"q0"},
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
          .direct_callee_name = std::string{"actual_f128_function"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Vreg,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{141},
                      .source_register_name = std::string{"q2"},
                      .source_register_bank = prepare::PreparedRegisterBank::Vreg,
                      .destination_register_name = std::string{"q0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"q0"},
                      .destination_register_bank = prepare::PreparedRegisterBank::Vreg,
                  },
              },
      }},
  });
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers =
          {
              prepare::PreparedF128Carrier{
                  .function_name = function_name,
                  .value_id = prepare::PreparedValueId{141},
                  .value_name = arg_name,
                  .source_type = bir::TypeKind::F128,
                  .kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
                  .total_size_bytes = 16,
                  .total_align_bytes = 16,
                  .register_bank = prepare::PreparedRegisterBank::Vreg,
                  .register_class = prepare::PreparedRegisterClass::Vector,
                  .contiguous_width = 1,
                  .register_name = std::string{"q2"},
                  .occupied_register_names = {"q2"},
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_call_f128_constant_argument(
    bool include_payload = true,
    bool include_source_value = true,
    bir::TypeKind literal_type = bir::TypeKind::F128) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("dispatch.call.f128.const.arg");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.call.f128.const.arg.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.call.f128.const.arg.entry");
  const auto actual_link = prepared.names.link_names.intern("actual_f128_function");
  const auto constant_name = prepared.names.value_names.intern("__f128.const.dispatch");
  const auto literal = literal_type == bir::TypeKind::F128
                           ? bir::Value::immediate_f128_bits(
                                 0x0123456789abcdefULL, 0x3fff800000000000ULL)
                           : bir::Value::immediate_f64_bits(0x3ff0000000000000ULL);

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call.f128.const.arg",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.f128.const.arg.entry",
          .insts = {bir::CallInst{
              .callee = "actual_f128_function",
              .callee_link_name_id = actual_link,
              .args = {literal},
              .arg_types = {bir::TypeKind::F128},
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
                              .from_value_id = prepare::PreparedValueId{241},
                              .to_value_id = prepare::PreparedValueId{241},
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{0},
                              .destination_register_name = std::string{"q0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"q0"},
                              .block_index = 0,
                              .instruction_index = 0,
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .reason = "f128_call_arg_constant_to_q_register",
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
                              .destination_register_name = std::string{"q0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"q0"},
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
          .direct_callee_name = std::string{"actual_f128_function"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Vreg,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
                      .source_value_id = include_source_value
                                             ? std::optional<prepare::PreparedValueId>{
                                                   prepare::PreparedValueId{241}}
                                             : std::nullopt,
                      .source_literal = literal,
                      .destination_register_name = std::string{"q0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"q0"},
                      .destination_register_bank = prepare::PreparedRegisterBank::Vreg,
                  },
              },
      }},
  });
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers =
          {
              prepare::PreparedF128Carrier{
                  .function_name = function_name,
                  .value_id = prepare::PreparedValueId{241},
                  .value_name = constant_name,
                  .source_type = bir::TypeKind::F128,
                  .kind = prepare::PreparedF128CarrierKind::Missing,
                  .total_size_bytes = 16,
                  .total_align_bytes = 16,
                  .constant_payload =
                      include_payload
                          ? std::optional<bir::Value::F128Payload>{
                                bir::Value::F128Payload{
                                    .low_bits = 0x0123456789abcdefULL,
                                    .high_bits = 0x3fff800000000000ULL,
                                }}
                          : std::nullopt,
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule prepared_semantic_f128_constant_call_argument() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "consume_tf";
  callee.return_type = bir::TypeKind::Void;
  callee.is_declaration = true;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "arg",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  module.functions.push_back(std::move(callee));

  bir::Function function;
  function.name = "dispatch.semantic.f128.const.arg";
  function.return_type = bir::TypeKind::Void;
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_tf",
      .args = {bir::Value::immediate_f128_bits(0x0123456789abcdefULL,
                                               0x3fff800000000000ULL)},
      .arg_types = {bir::TypeKind::F128},
      .return_type = bir::TypeKind::Void,
      .calling_convention = bir::CallingConv::C,
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::target_profile_from_triple("aarch64-unknown-linux-gnu"),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepared_semantic_aarch64_stack_call_argument() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "consume_byval";
  callee.return_type = bir::TypeKind::Void;
  callee.is_declaration = true;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "arg",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  module.functions.push_back(std::move(callee));

  bir::Function function;
  function.name = "dispatch.semantic.aarch64.stack.arg";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "payload",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_byval",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "payload")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type = bir::TypeKind::Void,
      .calling_convention = bir::CallingConv::C,
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::target_profile_from_triple("aarch64-unknown-linux-gnu"),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepared_semantic_f128_constant_helper_operand() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "dispatch.semantic.f128.const.helper";
  function.return_type = bir::TypeKind::F128;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "lhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F128, "sum"),
      .operand_type = bir::TypeKind::F128,
      .lhs = bir::Value::named(bir::TypeKind::F128, "lhs"),
      .rhs = bir::Value::immediate_f128_bits(0x0123456789abcdefULL,
                                             0x3fff800000000000ULL),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F128, "sum"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::target_profile_from_triple("aarch64-unknown-linux-gnu"),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
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

prepare::PreparedStoragePlanValue register_storage(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   const char* register_name);

prepare::PreparedBirModule prepared_with_atomic_memory_carriers(bool selected) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.atomic");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.atomic.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.atomic.entry");
  const auto ptr_name = prepared.names.value_names.intern("%ptr");
  const auto loaded_name = prepared.names.value_names.intern("%loaded");
  const auto stored_name = prepared.names.value_names.intern("%stored");
  const auto success_name = prepared.names.value_names.intern("%success");
  const auto old_name = prepared.names.value_names.intern("%old");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.atomic",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.atomic.entry",
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
                  .value_id = prepare::PreparedValueId{10},
                  .function_name = function_name,
                  .value_name = ptr_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x0"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{11},
                  .function_name = function_name,
                  .value_name = loaded_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w1"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{12},
                  .function_name = function_name,
                  .value_name = stored_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w2"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{13},
                  .function_name = function_name,
                  .value_name = success_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w3"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{14},
                  .function_name = function_name,
                  .value_name = old_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w4"},
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              register_storage(prepare::PreparedValueId{10}, ptr_name, "x0"),
              register_storage(prepare::PreparedValueId{11}, loaded_name, "w1"),
              register_storage(prepare::PreparedValueId{12}, stored_name, "w2"),
              register_storage(prepare::PreparedValueId{13}, success_name, "w3"),
              register_storage(prepare::PreparedValueId{14}, old_name, "w4"),
          },
  });

  if (selected) {
    prepared.atomic_operations.functions.push_back(
        prepare::PreparedAtomicOperationFunction{
            .function_name = function_name,
            .operations =
                {
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Load,
                        .block_label = entry_label,
                        .inst_index = 0,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = loaded_name,
                        .pointer_value_name = ptr_name,
                        .ordering = bir::AtomicOrdering::Acquire,
                        .result_mode = bir::AtomicResultMode::LoadedValue,
                        .address_space = bir::AddressSpace::Tls,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Store,
                        .block_label = entry_label,
                        .inst_index = 1,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .pointer_value_name = ptr_name,
                        .value_name = stored_name,
                        .ordering = bir::AtomicOrdering::Release,
                        .address_space = bir::AddressSpace::Gs,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Fence,
                        .block_label = entry_label,
                        .inst_index = 2,
                        .ordering = bir::AtomicOrdering::SeqCst,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Rmw,
                        .block_label = entry_label,
                        .inst_index = 3,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = old_name,
                        .pointer_value_name = ptr_name,
                        .value_name = stored_name,
                        .ordering = bir::AtomicOrdering::AcqRel,
                        .rmw_opcode = bir::AtomicRmwOpcode::Add,
                        .result_mode = bir::AtomicResultMode::OldValue,
                        .address_space = bir::AddressSpace::Default,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::CompareExchange,
                        .block_label = entry_label,
                        .inst_index = 4,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = success_name,
                        .pointer_value_name = ptr_name,
                        .expected_value_name = loaded_name,
                        .desired_value_name = stored_name,
                        .ordering = bir::AtomicOrdering::SeqCst,
                        .failure_ordering = bir::AtomicOrdering::Acquire,
                        .result_mode = bir::AtomicResultMode::BooleanSuccess,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::CompareExchange,
                        .block_label = entry_label,
                        .inst_index = 5,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = old_name,
                        .pointer_value_name = ptr_name,
                        .expected_value_name = loaded_name,
                        .desired_value_name = stored_name,
                        .ordering = bir::AtomicOrdering::Acquire,
                        .failure_ordering = bir::AtomicOrdering::Relaxed,
                        .result_mode = bir::AtomicResultMode::OldValue,
                    },
                },
        });
  } else {
    prepared.atomic_operations.functions.push_back(
        prepare::PreparedAtomicOperationFunction{
            .function_name = function_name,
            .operations =
                {
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Missing,
                        .operation_kind = bir::AtomicOperationKind::Load,
                        .block_label = entry_label,
                        .inst_index = 0,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = loaded_name,
                        .pointer_value_name = ptr_name,
                        .ordering = bir::AtomicOrdering::Acquire,
                        .result_mode = bir::AtomicResultMode::LoadedValue,
                        .missing_required_facts = {"missing_atomic_ordering"},
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Store,
                        .block_label = entry_label,
                        .inst_index = 1,
                        .value_type = bir::TypeKind::I128,
                        .width_bytes = 16,
                        .pointer_value_name = ptr_name,
                        .value_name = stored_name,
                        .ordering = bir::AtomicOrdering::Release,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Load,
                        .block_label = entry_label,
                        .inst_index = 2,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = loaded_name,
                        .pointer_value_name = ptr_name,
                        .ordering = bir::AtomicOrdering::Release,
                        .result_mode = bir::AtomicResultMode::LoadedValue,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Fence,
                        .block_label = entry_label,
                        .inst_index = 3,
                        .ordering = bir::AtomicOrdering::Relaxed,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::Rmw,
                        .block_label = entry_label,
                        .inst_index = 4,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = loaded_name,
                        .pointer_value_name = ptr_name,
                        .value_name = stored_name,
                        .ordering = bir::AtomicOrdering::AcqRel,
                        .rmw_opcode = bir::AtomicRmwOpcode::None,
                        .result_mode = bir::AtomicResultMode::OldValue,
                    },
                    prepare::PreparedAtomicOperationCarrier{
                        .function_name = function_name,
                        .carrier_kind =
                            prepare::PreparedAtomicOperationCarrierKind::Complete,
                        .operation_kind = bir::AtomicOperationKind::CompareExchange,
                        .block_label = entry_label,
                        .inst_index = 5,
                        .value_type = bir::TypeKind::I32,
                        .width_bytes = 4,
                        .result_value_name = success_name,
                        .pointer_value_name = ptr_name,
                        .value_name = stored_name,
                        .expected_value_name = loaded_name,
                        .desired_value_name = stored_name,
                        .ordering = bir::AtomicOrdering::SeqCst,
                        .failure_ordering = bir::AtomicOrdering::Release,
                        .result_mode = bir::AtomicResultMode::BooleanSuccess,
                    },
                },
        });
  }
  return prepared;
}

prepare::PreparedStoragePlanValue register_storage(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedStoragePlanValue fpr_storage(prepare::PreparedValueId value_id,
                                              c4c::ValueNameId value_name,
                                              const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Fpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedI128Carrier dispatch_i128_register_pair_carrier(
    c4c::FunctionNameId function_name,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    int low_register) {
  const std::string low = "x" + std::to_string(low_register);
  const std::string high = "x" + std::to_string(low_register + 1);
  return prepare::PreparedI128Carrier{
      .function_name = function_name,
      .value_id = value_id,
      .value_name = value_name,
      .source_type = bir::TypeKind::I128,
      .kind = prepare::PreparedI128CarrierKind::RegisterPair,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .contiguous_width = 2,
      .occupied_register_names = {low, high},
      .low_lane =
          prepare::PreparedI128LaneCarrier{
              .role = prepare::PreparedI128LaneRole::Low,
              .lane_index = 0,
              .width_bytes = 8,
              .register_name = low,
          },
      .high_lane =
          prepare::PreparedI128LaneCarrier{
              .role = prepare::PreparedI128LaneRole::High,
              .lane_index = 1,
              .width_bytes = 8,
              .register_name = high,
          },
  };
}

prepare::PreparedI128RuntimeHelper::LaneBinding dispatch_i128_helper_lane(
    const prepare::PreparedI128Carrier& carrier,
    const prepare::PreparedI128LaneCarrier& lane) {
  return prepare::PreparedI128RuntimeHelper::LaneBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .role = lane.role,
      .lane_index = lane.lane_index,
      .width_bytes = lane.width_bytes,
      .register_name = lane.register_name,
      .slot_id = lane.slot_id,
      .stack_offset_bytes = lane.stack_offset_bytes,
  };
}

prepare::PreparedI128RuntimeHelper::AbiRegisterBinding dispatch_i128_helper_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    prepare::PreparedI128LaneRole role,
    std::size_t lane_index,
    std::optional<std::size_t> argument_index,
    std::size_t abi_index,
    std::string register_name,
    prepare::PreparedRegisterSlotPool pool) {
  const std::vector<std::string> occupied_register_names{register_name};
  return prepare::PreparedI128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .role = role,
      .lane_index = lane_index,
      .width_bytes = 8,
      .helper_argument_index = argument_index,
      .abi_register_index = abi_index,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .register_name = std::move(register_name),
      .contiguous_width = 1,
      .occupied_register_names = occupied_register_names,
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = pool,
              .slot_index = abi_index,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedI128RuntimeHelper::MarshalingMove dispatch_i128_helper_marshaling_move(
    const prepare::PreparedI128RuntimeHelper::LaneBinding& lane,
    const prepare::PreparedI128RuntimeHelper::AbiRegisterBinding& binding,
    prepare::PreparedI128RuntimeHelperMarshalDirection direction) {
  return prepare::PreparedI128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .phase =
          direction ==
                  prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument
              ? prepare::PreparedMovePhase::BeforeCall
              : prepare::PreparedMovePhase::AfterCall,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .carrier_lane = lane,
      .abi_register = binding,
  };
}

prepare::PreparedI128RuntimeHelper dispatch_i128_runtime_helper(
    c4c::FunctionNameId function_name,
    std::size_t instruction_index,
    bir::BinaryOpcode opcode,
    prepare::PreparedI128RuntimeHelperKind helper_kind,
    std::string callee_name,
    const prepare::PreparedI128Carrier& result,
    const prepare::PreparedI128Carrier& lhs,
    const prepare::PreparedI128Carrier& rhs) {
  auto helper = prepare::PreparedI128RuntimeHelper{
      .function_name = function_name,
      .block_index = 0,
      .instruction_index = instruction_index,
      .source_binary_opcode = opcode,
      .source_type = bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .result_value_id = result.value_id,
      .result_value_name = result.value_name,
      .lhs_value_id = lhs.value_id,
      .lhs_value_name = lhs.value_name,
      .rhs_value_id = rhs.value_id,
      .rhs_value_name = rhs.value_name,
      .helper_family = prepare::PreparedI128RuntimeHelperFamily::DivRem,
      .helper_kind = helper_kind,
      .callee_name = std::move(callee_name),
      .result_ownership =
          prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes,
      .lhs_low_lane = dispatch_i128_helper_lane(lhs, lhs.low_lane),
      .lhs_high_lane = dispatch_i128_helper_lane(lhs, lhs.high_lane),
      .rhs_low_lane = dispatch_i128_helper_lane(rhs, rhs.low_lane),
      .rhs_high_lane = dispatch_i128_helper_lane(rhs, rhs.high_lane),
      .result_low_lane = dispatch_i128_helper_lane(result, result.low_lane),
      .result_high_lane = dispatch_i128_helper_lane(result, result.high_lane),
      .resource_policy =
          prepare::PreparedI128RuntimeHelper::ResourcePolicy{
              .call_boundary = true,
              .runtime_helper_callee = true,
              .caller_saved_clobbers = true,
              .preserves_source_operation_identity = true,
          },
      .abi_policy =
          prepare::PreparedI128RuntimeHelper::AbiPolicy{
              .transition =
                  prepare::PreparedI128RuntimeHelperAbiTransition::
                      DirectRegisterPairArgumentsAndResult,
              .argument_bank = prepare::PreparedRegisterBank::Gpr,
              .result_bank = prepare::PreparedRegisterBank::Gpr,
              .argument_count = 2,
              .lanes_per_argument = 2,
              .result_lane_count = 2,
              .lane_width_bytes = 8,
          },
      .clobbered_registers =
          {prepare::PreparedClobberedRegister{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = "x13",
              .contiguous_width = 1,
              .occupied_register_names = {"x13"},
          }},
  };
  helper.lhs_low_abi_argument = dispatch_i128_helper_abi_binding(
      helper.lhs_value_id, helper.lhs_value_name, prepare::PreparedI128LaneRole::Low, 0,
      std::size_t{0}, 0, "x0", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.lhs_high_abi_argument = dispatch_i128_helper_abi_binding(
      helper.lhs_value_id, helper.lhs_value_name, prepare::PreparedI128LaneRole::High, 1,
      std::size_t{0}, 1, "x1", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_low_abi_argument = dispatch_i128_helper_abi_binding(
      helper.rhs_value_id, helper.rhs_value_name, prepare::PreparedI128LaneRole::Low, 0,
      std::size_t{1}, 2, "x2", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_high_abi_argument = dispatch_i128_helper_abi_binding(
      helper.rhs_value_id, helper.rhs_value_name, prepare::PreparedI128LaneRole::High, 1,
      std::size_t{1}, 3, "x3", prepare::PreparedRegisterSlotPool::CallArgument);
  helper.result_low_abi_result = dispatch_i128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, prepare::PreparedI128LaneRole::Low, 0,
      std::nullopt, 0, "x0", prepare::PreparedRegisterSlotPool::CallResult);
  helper.result_high_abi_result = dispatch_i128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, prepare::PreparedI128LaneRole::High, 1,
      std::nullopt, 1, "x1", prepare::PreparedRegisterSlotPool::CallResult);
  helper.lhs_low_argument_move = dispatch_i128_helper_marshaling_move(
      *helper.lhs_low_lane, *helper.lhs_low_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.lhs_high_argument_move = dispatch_i128_helper_marshaling_move(
      *helper.lhs_high_lane, *helper.lhs_high_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.rhs_low_argument_move = dispatch_i128_helper_marshaling_move(
      *helper.rhs_low_lane, *helper.rhs_low_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.rhs_high_argument_move = dispatch_i128_helper_marshaling_move(
      *helper.rhs_high_lane, *helper.rhs_high_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.result_low_unmarshal_move = dispatch_i128_helper_marshaling_move(
      *helper.result_low_lane, *helper.result_low_abi_result,
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane);
  helper.result_high_unmarshal_move = dispatch_i128_helper_marshaling_move(
      *helper.result_high_lane, *helper.result_high_abi_result,
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane);
  helper.live_preservation_policy =
      prepare::PreparedI128RuntimeHelper::LivePreservationPolicy{
          .evaluated = true,
          .caller_saved_clobbers_modeled = true,
          .no_additional_live_preservation_required = true,
      };
  helper.selected_call_ownership =
      prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{
          .owns_terminal_call = true,
          .has_callee_identity = true,
          .has_resource_policy = true,
          .has_clobber_policy = true,
          .has_abi_bindings = true,
          .has_marshaling = true,
          .has_live_preservation = true,
  };
  return helper;
}

prepare::PreparedF128RuntimeHelper::CarrierBinding dispatch_f128_helper_carrier_binding(
    const prepare::PreparedF128Carrier& carrier) {
  return prepare::PreparedF128RuntimeHelper::CarrierBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .width_bytes = carrier.total_size_bytes,
      .align_bytes = carrier.total_align_bytes,
      .register_bank = carrier.register_bank,
      .register_class = carrier.register_class,
      .register_name = carrier.register_name,
      .slot_id = carrier.slot_id,
      .stack_offset_bytes = carrier.stack_offset_bytes,
  };
}

prepare::PreparedF128RuntimeHelper::AbiRegisterBinding dispatch_f128_helper_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    std::optional<std::size_t> argument_index,
    std::size_t abi_index,
    std::string register_name,
    prepare::PreparedRegisterSlotPool pool) {
  const std::vector<std::string> occupied_register_names{register_name};
  return prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = argument_index,
      .abi_register_index = abi_index,
      .width_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .register_name = std::move(register_name),
      .contiguous_width = 1,
      .occupied_register_names = occupied_register_names,
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .pool = pool,
              .slot_index = abi_index,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedF128RuntimeHelper::AbiRegisterBinding dispatch_f128_cmp_result_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name) {
  return prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = std::nullopt,
      .abi_register_index = 0,
      .width_bytes = 4,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .register_name = "w0",
      .contiguous_width = 1,
      .occupied_register_names = {"w0"},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CallResult,
              .slot_index = 0,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedF128RuntimeHelper::AbiRegisterBinding dispatch_f128_scalar_fp_abi_binding(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type,
    std::optional<std::size_t> argument_index,
    std::string register_name,
    prepare::PreparedRegisterSlotPool pool) {
  const std::size_t width = type == bir::TypeKind::F64 ? std::size_t{8} : std::size_t{4};
  return prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = argument_index,
      .abi_register_index = 0,
      .width_bytes = width,
      .register_bank = prepare::PreparedRegisterBank::Fpr,
      .register_class = prepare::PreparedRegisterClass::Float,
      .register_name = std::move(register_name),
      .contiguous_width = 1,
      .occupied_register_names = {type == bir::TypeKind::F64 ? "d0" : "s0"},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Fpr,
              .pool = pool,
              .slot_index = 0,
              .contiguous_width = 1,
          },
  };
}

prepare::PreparedF128RuntimeHelper::MarshalingMove dispatch_f128_helper_marshaling_move(
    const prepare::PreparedF128RuntimeHelper::CarrierBinding& carrier,
    const prepare::PreparedF128RuntimeHelper::AbiRegisterBinding& binding,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction) {
  return prepare::PreparedF128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .carrier = carrier,
      .abi_register = binding,
  };
}

prepare::PreparedF128RuntimeHelper dispatch_f128_runtime_helper(
    c4c::FunctionNameId function_name,
    std::size_t instruction_index,
    bir::BinaryOpcode opcode,
    prepare::PreparedF128RuntimeHelperKind kind,
    std::string callee_name,
    const prepare::PreparedF128Carrier& result,
    const prepare::PreparedF128Carrier& lhs,
    const prepare::PreparedF128Carrier& rhs) {
  auto helper = prepare::PreparedF128RuntimeHelper{
      .function_name = function_name,
      .block_index = 0,
      .instruction_index = instruction_index,
      .source_binary_opcode = opcode,
      .source_type = bir::TypeKind::F128,
      .result_type = bir::TypeKind::F128,
      .result_value_id = result.value_id,
      .result_value_name = result.value_name,
      .lhs_value_id = lhs.value_id,
      .lhs_value_name = lhs.value_name,
      .rhs_value_id = rhs.value_id,
      .rhs_value_name = rhs.value_name,
      .helper_family = prepare::PreparedF128RuntimeHelperFamily::Arithmetic,
      .helper_kind = kind,
      .callee_name = std::move(callee_name),
      .result_ownership =
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier,
      .lhs_carrier = dispatch_f128_helper_carrier_binding(lhs),
      .rhs_carrier = dispatch_f128_helper_carrier_binding(rhs),
      .result_carrier = dispatch_f128_helper_carrier_binding(result),
      .resource_policy =
          prepare::PreparedF128RuntimeHelper::ResourcePolicy{
              .call_boundary = true,
              .runtime_helper_callee = true,
              .caller_saved_clobbers = true,
              .preserves_source_operation_identity = true,
          },
      .abi_policy =
          prepare::PreparedF128RuntimeHelper::AbiPolicy{
              .transition =
                  prepare::PreparedF128RuntimeHelperAbiTransition::
                      DirectF128ArgumentsAndResult,
              .argument_bank = prepare::PreparedRegisterBank::Vreg,
              .result_bank = prepare::PreparedRegisterBank::Vreg,
              .argument_count = 2,
              .result_count = 1,
              .width_bytes = 16,
          },
      .live_preservation_policy =
          prepare::PreparedF128RuntimeHelper::LivePreservationPolicy{
              .evaluated = true,
              .caller_saved_clobbers_modeled = true,
              .no_additional_live_preservation_required = true,
          },
      .selected_call_ownership =
          prepare::PreparedF128RuntimeHelper::SelectedCallOwnershipPolicy{
              .owns_terminal_call = true,
              .has_callee_identity = true,
              .has_resource_policy = true,
              .has_clobber_policy = true,
              .has_abi_bindings = true,
              .has_marshaling = true,
              .has_live_preservation = true,
          },
      .clobbered_registers =
          {prepare::PreparedClobberedRegister{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .register_name = "q16",
              .contiguous_width = 1,
              .occupied_register_names = {"q16"},
          }},
  };
  helper.lhs_abi_argument = dispatch_f128_helper_abi_binding(
      helper.lhs_value_id, helper.lhs_value_name, std::size_t{0}, 0, "q0",
      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_abi_argument = dispatch_f128_helper_abi_binding(
      helper.rhs_value_id, helper.rhs_value_name, std::size_t{1}, 1, "q1",
      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.result_abi_result = dispatch_f128_helper_abi_binding(
      helper.result_value_id, helper.result_value_name, std::nullopt, 0, "q0",
      prepare::PreparedRegisterSlotPool::CallResult);
  helper.lhs_argument_move = dispatch_f128_helper_marshaling_move(
      *helper.lhs_carrier, *helper.lhs_abi_argument,
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument);
  helper.rhs_argument_move = dispatch_f128_helper_marshaling_move(
      *helper.rhs_carrier, *helper.rhs_abi_argument,
      prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument);
  helper.result_unmarshal_move = dispatch_f128_helper_marshaling_move(
      *helper.result_carrier, *helper.result_abi_result,
      prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier);
  return helper;
}

void dispatch_retarget_f128_helper_as_compare(
    prepare::PreparedF128RuntimeHelper& helper,
    bir::BinaryOpcode opcode,
    prepare::PreparedF128RuntimeHelperKind kind,
    std::string callee_name) {
  helper.source_binary_opcode = opcode;
  helper.result_type = bir::TypeKind::I32;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Comparison;
  helper.helper_kind = kind;
  helper.callee_name = std::move(callee_name);
  helper.result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult;
  helper.result_carrier.reset();
  helper.result_unmarshal_move.reset();
  helper.scalar_result =
      prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
          .value_id = helper.result_value_id,
          .value_name = helper.result_value_name,
          .type = bir::TypeKind::I32,
          .width_bytes = 4,
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .home_kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"w9"},
      };
  helper.result_abi_result =
      dispatch_f128_cmp_result_abi_binding(helper.result_value_id,
                                           helper.result_value_name);
  helper.scalar_result_unmarshal_move =
      prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
          .direction =
              prepare::PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar,
          .scalar_result = *helper.scalar_result,
          .abi_register = *helper.result_abi_result,
      };
  helper.scalar_cmp_result_consumption =
      prepare::PreparedF128RuntimeHelper::ScalarCmpResultConsumption{
          .cmp_type = bir::TypeKind::I32,
          .bir_result_type = bir::TypeKind::I1,
          .zero_test = prepare::PreparedF128CmpResultZeroTest::EqualZero,
          .consumes_helper_cmp_result = true,
          .owns_bir_i1_result = true,
      };
  helper.abi_policy = prepare::PreparedF128RuntimeHelper::AbiPolicy{
      .transition =
          prepare::PreparedF128RuntimeHelperAbiTransition::
              DirectF128ArgumentsAndCmpResult,
      .argument_bank = prepare::PreparedRegisterBank::Vreg,
      .result_bank = prepare::PreparedRegisterBank::Gpr,
      .argument_count = 2,
      .result_count = 1,
      .width_bytes = 4,
  };
}

prepare::PreparedBirModule prepared_with_frame_slot_load(bool include_storage = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.load");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.load.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.load.entry");
  const auto result_name = prepared.names.value_names.intern("%loaded");
  prepared.stack_layout = prepare::PreparedStackLayout{
      .frame_slots =
          {prepare::PreparedFrameSlot{
              .slot_id = prepare::PreparedFrameSlotId{20},
              .object_id = prepare::PreparedObjectId{20},
              .function_name = function_name,
              .offset_bytes = 0,
              .size_bytes = 16,
              .align_bytes = 16,
              .fixed_location = true,
          }},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.load",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.load.entry",
              .insts =
                  {bir::LoadLocalInst{
                      .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
                      .slot_id = c4c::SlotNameId{5},
                      .byte_offset = 8,
                      .align_bytes = 4,
                      .address =
                          bir::MemoryAddress{
                              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                              .byte_offset = 8,
                              .size_bytes = 4,
                              .align_bytes = 4,
                              .address_space = bir::AddressSpace::Fs,
                              .is_volatile = true,
                              .base_slot_id = c4c::SlotNameId{5},
                          },
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
          {prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{10},
              .function_name = function_name,
              .value_name = result_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = "w0",
          }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          include_storage
              ? std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{10}, result_name, "w0")}
              : std::vector<prepare::PreparedStoragePlanValue>{},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = entry_label,
              .inst_index = 0,
              .result_value_name = result_name,
              .address_space = bir::AddressSpace::Fs,
              .is_volatile = true,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{20},
                      .byte_offset = 8,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_i128_frame_slot_load(bool include_carrier = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.i128.load");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.i128.load.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.i128.load.entry");
  const auto result_name = prepared.names.value_names.intern("%loaded.i128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.i128.load",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.i128.load.entry",
              .insts =
                  {bir::LoadLocalInst{
                      .result = bir::Value::named(bir::TypeKind::I128, "%loaded.i128"),
                      .slot_id = c4c::SlotNameId{5},
                      .byte_offset = 16,
                      .align_bytes = 16,
                      .address =
                          bir::MemoryAddress{
                              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                              .byte_offset = 16,
                              .size_bytes = 16,
                              .align_bytes = 16,
                              .address_space = bir::AddressSpace::Default,
                              .base_slot_id = c4c::SlotNameId{5},
                          },
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
          {prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{110},
              .function_name = function_name,
              .value_name = result_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = "x4",
          }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 64,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = entry_label,
              .inst_index = 0,
              .result_value_name = result_name,
              .address_space = bir::AddressSpace::Default,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{20},
                      .byte_offset = 16,
                      .size_bytes = 16,
                      .align_bytes = 16,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });
  if (include_carrier) {
    prepared.i128_carriers.functions.push_back(prepare::PreparedI128CarrierFunction{
        .function_name = function_name,
        .carriers =
            {prepare::PreparedI128Carrier{
                .function_name = function_name,
                .value_id = prepare::PreparedValueId{110},
                .value_name = result_name,
                .source_type = bir::TypeKind::I128,
                .kind = prepare::PreparedI128CarrierKind::RegisterPair,
                .lane_width_bytes = 8,
                .total_size_bytes = 16,
                .total_align_bytes = 16,
                .register_bank = prepare::PreparedRegisterBank::Gpr,
                .register_class = prepare::PreparedRegisterClass::General,
                .contiguous_width = 2,
                .occupied_register_names = {"x4", "x5"},
                .low_lane =
                    prepare::PreparedI128LaneCarrier{
                        .role = prepare::PreparedI128LaneRole::Low,
                        .lane_index = 0,
                        .width_bytes = 8,
                        .register_name = std::string{"x4"},
                    },
                .high_lane =
                    prepare::PreparedI128LaneCarrier{
                        .role = prepare::PreparedI128LaneRole::High,
                        .lane_index = 1,
                        .width_bytes = 8,
                        .register_name = std::string{"x5"},
                    },
            }},
    });
  }
  return prepared;
}

prepare::PreparedBirModule prepared_with_f128_frame_slot_load(bool include_carrier = false) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.f128.load");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.f128.load.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.f128.load.entry");
  const auto result_name = prepared.names.value_names.intern("%loaded.f128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.f128.load",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.f128.load.entry",
              .insts =
                  {bir::LoadLocalInst{
                      .result = bir::Value::named(bir::TypeKind::F128, "%loaded.f128"),
                      .slot_id = c4c::SlotNameId{5},
                      .byte_offset = 16,
                      .align_bytes = 16,
                      .address =
                          bir::MemoryAddress{
                              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                              .byte_offset = 16,
                              .size_bytes = 16,
                              .align_bytes = 16,
                              .address_space = bir::AddressSpace::Default,
                              .base_slot_id = c4c::SlotNameId{5},
                          },
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
          {prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{111},
              .function_name = function_name,
              .value_name = result_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = "q4",
          }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {fpr_storage(prepare::PreparedValueId{111}, result_name, "q4")},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 64,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = entry_label,
              .inst_index = 0,
              .result_value_name = result_name,
              .address_space = bir::AddressSpace::Default,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{20},
                      .byte_offset = 16,
                      .size_bytes = 16,
                      .align_bytes = 16,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });
  if (include_carrier) {
    prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
        .function_name = function_name,
        .carriers =
            {prepare::PreparedF128Carrier{
                .function_name = function_name,
                .value_id = prepare::PreparedValueId{111},
                .value_name = result_name,
                .source_type = bir::TypeKind::F128,
                .kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
                .total_size_bytes = 16,
                .total_align_bytes = 16,
                .register_bank = prepare::PreparedRegisterBank::Vreg,
                .register_class = prepare::PreparedRegisterClass::Vector,
                .contiguous_width = 1,
                .register_name = std::string{"q4"},
                .occupied_register_names = {"q4"},
            }},
    });
  }
  return prepared;
}

prepare::PreparedBirModule prepared_with_f128_frame_slot_store(
    bool include_carrier = true,
    bool complete_carrier = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.f128.store");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.f128.store.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.f128.store.entry");
  const auto stored_name = prepared.names.value_names.intern("%stored.f128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.f128.store",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.f128.store.entry",
              .insts =
                  {bir::StoreLocalInst{
                      .slot_id = c4c::SlotNameId{6},
                      .value = bir::Value::named(bir::TypeKind::F128, "%stored.f128"),
                      .byte_offset = 32,
                      .align_bytes = 16,
                      .address =
                          bir::MemoryAddress{
                              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                              .byte_offset = 32,
                              .size_bytes = 16,
                              .align_bytes = 16,
                              .address_space = bir::AddressSpace::Default,
                              .base_slot_id = c4c::SlotNameId{6},
                          },
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
          {prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{112},
              .function_name = function_name,
              .value_name = stored_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = "q5",
          }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {fpr_storage(prepare::PreparedValueId{112}, stored_name, "q5")},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 80,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = entry_label,
              .inst_index = 0,
              .stored_value_name = stored_name,
              .address_space = bir::AddressSpace::Default,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{21},
                      .byte_offset = 32,
                      .size_bytes = 16,
                      .align_bytes = 16,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });
  if (include_carrier) {
    prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
        .function_name = function_name,
        .carriers =
            {prepare::PreparedF128Carrier{
                .function_name = function_name,
                .value_id = prepare::PreparedValueId{112},
                .value_name = stored_name,
                .source_type = bir::TypeKind::F128,
                .kind = complete_carrier ? prepare::PreparedF128CarrierKind::FullWidthRegister
                                         : prepare::PreparedF128CarrierKind::Missing,
                .total_size_bytes = 16,
                .total_align_bytes = complete_carrier ? std::size_t{16} : std::size_t{8},
                .register_bank = prepare::PreparedRegisterBank::Vreg,
                .register_class = prepare::PreparedRegisterClass::Vector,
                .contiguous_width = 1,
                .register_name = std::string{"q5"},
                .occupied_register_names = {"q5"},
                .missing_required_facts =
                    complete_carrier ? std::vector<std::string>{}
                                     : std::vector<std::string>{"f128_full_width_carrier"},
            }},
    });
  }
  return prepared;
}

prepare::PreparedBirModule prepared_with_f128_load_add_store_route() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.f128.route");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.f128.route.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.f128.route.entry");
  const auto lhs_name = prepared.names.value_names.intern("%route.lhs.f128");
  const auto rhs_name = prepared.names.value_names.intern("%route.rhs.f128");
  const auto sum_name = prepared.names.value_names.intern("%route.sum.f128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.f128.route",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.f128.route.entry",
              .insts =
                  {bir::LoadLocalInst{
                       .result =
                           bir::Value::named(bir::TypeKind::F128, "%route.lhs.f128"),
                       .slot_id = c4c::SlotNameId{10},
                       .align_bytes = 16,
                       .address =
                           bir::MemoryAddress{
                               .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                               .size_bytes = 16,
                               .align_bytes = 16,
                               .address_space = bir::AddressSpace::Default,
                               .base_slot_id = c4c::SlotNameId{10},
                           },
                   },
                   bir::BinaryInst{
                       .opcode = bir::BinaryOpcode::Add,
                       .result =
                           bir::Value::named(bir::TypeKind::F128, "%route.sum.f128"),
                       .operand_type = bir::TypeKind::F128,
                       .lhs = bir::Value::named(bir::TypeKind::F128, "%route.lhs.f128"),
                       .rhs = bir::Value::named(bir::TypeKind::F128, "%route.rhs.f128"),
                   },
                   bir::StoreLocalInst{
                       .slot_id = c4c::SlotNameId{11},
                       .value =
                           bir::Value::named(bir::TypeKind::F128, "%route.sum.f128"),
                       .byte_offset = 16,
                       .align_bytes = 16,
                       .address =
                           bir::MemoryAddress{
                               .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                               .byte_offset = 16,
                               .size_bytes = 16,
                               .align_bytes = 16,
                               .address_space = bir::AddressSpace::Default,
                               .base_slot_id = c4c::SlotNameId{11},
                           },
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

  const auto lhs_carrier = dispatch_f128_register_carrier(
      function_name, prepare::PreparedValueId{270}, lhs_name, "q6");
  const auto rhs_carrier = dispatch_f128_register_carrier(
      function_name, prepare::PreparedValueId{271}, rhs_name, "q8");
  const auto sum_carrier = dispatch_f128_register_carrier(
      function_name, prepare::PreparedValueId{272}, sum_name, "q4");
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
               .value_id = lhs_carrier.value_id,
               .function_name = function_name,
               .value_name = lhs_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "q6",
           },
           prepare::PreparedValueHome{
               .value_id = rhs_carrier.value_id,
               .function_name = function_name,
               .value_name = rhs_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "q8",
           },
           prepare::PreparedValueHome{
               .value_id = sum_carrier.value_id,
               .function_name = function_name,
               .value_name = sum_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "q4",
           }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {fpr_storage(lhs_carrier.value_id, lhs_name, "q6"),
                 fpr_storage(rhs_carrier.value_id, rhs_name, "q8"),
                 fpr_storage(sum_carrier.value_id, sum_name, "q4")},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 96,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
               .function_name = function_name,
               .block_label = entry_label,
               .inst_index = 0,
               .result_value_name = lhs_name,
               .address_space = bir::AddressSpace::Default,
               .address =
                   prepare::PreparedAddress{
                       .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                       .frame_slot_id = prepare::PreparedFrameSlotId{30},
                       .size_bytes = 16,
                       .align_bytes = 16,
                       .can_use_base_plus_offset = true,
                   },
           },
           prepare::PreparedMemoryAccess{
               .function_name = function_name,
               .block_label = entry_label,
               .inst_index = 2,
               .stored_value_name = sum_name,
               .address_space = bir::AddressSpace::Default,
               .address =
                   prepare::PreparedAddress{
                       .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                       .frame_slot_id = prepare::PreparedFrameSlotId{31},
                       .byte_offset = 16,
                       .size_bytes = 16,
                       .align_bytes = 16,
                       .can_use_base_plus_offset = true,
                   },
           }},
  });
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers = {sum_carrier, lhs_carrier, rhs_carrier},
  });
  prepared.f128_runtime_helpers.functions.push_back(
      prepare::PreparedF128RuntimeHelperFunction{
          .function_name = function_name,
          .helpers = {dispatch_f128_runtime_helper(function_name,
                                                   1,
                                                   bir::BinaryOpcode::Add,
                                                   prepare::PreparedF128RuntimeHelperKind::Add,
                                                   "__addtf3",
                                                   sum_carrier,
                                                   lhs_carrier,
                                                   rhs_carrier)},
      });
  return prepared;
}

prepare::PreparedBirModule prepared_with_i128_pair_operation(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Add,
    bool include_rhs_carrier = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.i128.pair");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.i128.pair.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.i128.pair.entry");
  const auto result_name = prepared.names.value_names.intern("%result.i128");
  const auto lhs_name = prepared.names.value_names.intern("%lhs.i128");
  const auto rhs_name = prepared.names.value_names.intern("%rhs.i128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.i128.pair",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.i128.pair.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = opcode,
                      .result = bir::Value::named(bir::TypeKind::I128, "%result.i128"),
                      .operand_type = bir::TypeKind::I128,
                      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs.i128"),
                      .rhs = bir::Value::named(bir::TypeKind::I128, "%rhs.i128"),
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
  std::vector<prepare::PreparedI128Carrier> carriers = {
      dispatch_i128_register_pair_carrier(function_name,
                                          prepare::PreparedValueId{120},
                                          result_name,
                                          6),
      dispatch_i128_register_pair_carrier(function_name,
                                          prepare::PreparedValueId{121},
                                          lhs_name,
                                          8),
  };
  if (include_rhs_carrier) {
    carriers.push_back(dispatch_i128_register_pair_carrier(function_name,
                                                          prepare::PreparedValueId{122},
                                                          rhs_name,
                                                          10));
  }
  prepared.i128_carriers.functions.push_back(prepare::PreparedI128CarrierFunction{
      .function_name = function_name,
      .carriers = std::move(carriers),
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_i128_copy_operation(bool include_source_carrier = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.i128.copy");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.i128.copy.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.i128.copy.entry");
  const auto result_name = prepared.names.value_names.intern("%copy.i128");
  const auto source_name = prepared.names.value_names.intern("%source.i128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.i128.copy",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.i128.copy.entry",
              .insts =
                  {bir::CastInst{
                      .opcode = bir::CastOpcode::Bitcast,
                      .result = bir::Value::named(bir::TypeKind::I128, "%copy.i128"),
                      .operand = bir::Value::named(bir::TypeKind::I128, "%source.i128"),
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
  std::vector<prepare::PreparedI128Carrier> carriers = {
      dispatch_i128_register_pair_carrier(function_name,
                                          prepare::PreparedValueId{130},
                                          result_name,
                                          12),
  };
  if (include_source_carrier) {
    carriers.push_back(dispatch_i128_register_pair_carrier(function_name,
                                                          prepare::PreparedValueId{131},
                                                          source_name,
                                                          14));
  }
  prepared.i128_carriers.functions.push_back(prepare::PreparedI128CarrierFunction{
      .function_name = function_name,
      .carriers = std::move(carriers),
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_i128_runtime_helper_operation(
    bool include_helper = true,
    bool include_clobber_policy = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.i128.helper");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.i128.helper.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.i128.helper.entry");
  const auto result_name = prepared.names.value_names.intern("%helper.result.i128");
  const auto lhs_name = prepared.names.value_names.intern("%helper.lhs.i128");
  const auto rhs_name = prepared.names.value_names.intern("%helper.rhs.i128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.i128.helper",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.i128.helper.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = bir::BinaryOpcode::SRem,
                      .result =
                          bir::Value::named(bir::TypeKind::I128, "%helper.result.i128"),
                      .operand_type = bir::TypeKind::I128,
                      .lhs = bir::Value::named(bir::TypeKind::I128, "%helper.lhs.i128"),
                      .rhs = bir::Value::named(bir::TypeKind::I128, "%helper.rhs.i128"),
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
  std::vector<prepare::PreparedI128Carrier> carriers = {
      dispatch_i128_register_pair_carrier(function_name,
                                          prepare::PreparedValueId{150},
                                          result_name,
                                          6),
      dispatch_i128_register_pair_carrier(function_name,
                                          prepare::PreparedValueId{151},
                                          lhs_name,
                                          8),
      dispatch_i128_register_pair_carrier(function_name,
                                          prepare::PreparedValueId{152},
                                          rhs_name,
                                          10),
  };
  if (include_helper) {
    auto helper = dispatch_i128_runtime_helper(
        function_name,
        0,
        bir::BinaryOpcode::SRem,
        prepare::PreparedI128RuntimeHelperKind::SignedRem,
        "__modti3",
        carriers[0],
        carriers[1],
        carriers[2]);
    if (!include_clobber_policy) {
      helper.clobbered_registers.clear();
    }
    prepared.i128_runtime_helpers.functions.push_back(
        prepare::PreparedI128RuntimeHelperFunction{
            .function_name = function_name,
            .helpers = {std::move(helper)},
        });
  }
  prepared.i128_carriers.functions.push_back(prepare::PreparedI128CarrierFunction{
      .function_name = function_name,
      .carriers = std::move(carriers),
  });
  return prepared;
}

prepare::PreparedF128Carrier dispatch_f128_register_carrier(
    c4c::FunctionNameId function_name,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const char* register_name) {
  return prepare::PreparedF128Carrier{
      .function_name = function_name,
      .value_id = value_id,
      .value_name = value_name,
      .source_type = bir::TypeKind::F128,
      .kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .contiguous_width = 1,
      .register_name = std::string{register_name},
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedBirModule prepared_with_f128_runtime_helper_operation(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Add,
    bool include_helper = true,
    bool include_clobber_policy = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.f128.helper");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.f128.helper.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.f128.helper.entry");
  const auto result_name = prepared.names.value_names.intern("%helper.result.f128");
  const auto lhs_name = prepared.names.value_names.intern("%helper.lhs.f128");
  const auto rhs_name = prepared.names.value_names.intern("%helper.rhs.f128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.f128.helper",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.f128.helper.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = opcode,
                      .result =
                          bir::Value::named(bir::TypeKind::F128, "%helper.result.f128"),
                      .operand_type = bir::TypeKind::F128,
                      .lhs = bir::Value::named(bir::TypeKind::F128, "%helper.lhs.f128"),
                      .rhs = bir::Value::named(bir::TypeKind::F128, "%helper.rhs.f128"),
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
  std::vector<prepare::PreparedF128Carrier> carriers = {
      dispatch_f128_register_carrier(
          function_name, prepare::PreparedValueId{250}, result_name, "q4"),
      dispatch_f128_register_carrier(
          function_name, prepare::PreparedValueId{251}, lhs_name, "q6"),
      dispatch_f128_register_carrier(
          function_name, prepare::PreparedValueId{252}, rhs_name, "q8"),
  };
  if (include_helper) {
    auto helper_kind = prepare::PreparedF128RuntimeHelperKind::Add;
    std::string callee = "__addtf3";
    if (opcode == bir::BinaryOpcode::Sub) {
      helper_kind = prepare::PreparedF128RuntimeHelperKind::Sub;
      callee = "__subtf3";
    } else if (opcode == bir::BinaryOpcode::Mul) {
      helper_kind = prepare::PreparedF128RuntimeHelperKind::Mul;
      callee = "__multf3";
    } else if (opcode == bir::BinaryOpcode::SDiv) {
      helper_kind = prepare::PreparedF128RuntimeHelperKind::Div;
      callee = "__divtf3";
    }
    auto helper = dispatch_f128_runtime_helper(function_name,
                                               0,
                                               opcode,
                                               helper_kind,
                                               callee,
                                               carriers[0],
                                               carriers[1],
                                               carriers[2]);
    if (!include_clobber_policy) {
      helper.clobbered_registers.clear();
    }
    prepared.f128_runtime_helpers.functions.push_back(
        prepare::PreparedF128RuntimeHelperFunction{
            .function_name = function_name,
            .helpers = {std::move(helper)},
        });
  }
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers = std::move(carriers),
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_f128_cast_helper_operation(
    bir::CastOpcode opcode = bir::CastOpcode::FPExt,
    bir::TypeKind source_type = bir::TypeKind::F64,
    bir::TypeKind result_type = bir::TypeKind::F128) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.f128.cast");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.f128.cast.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.f128.cast.entry");
  const auto result_name = prepared.names.value_names.intern("%cast.result");
  const auto operand_name = prepared.names.value_names.intern("%cast.operand");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.f128.cast",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.f128.cast.entry",
              .insts =
                  {bir::CastInst{
                      .opcode = opcode,
                      .result = bir::Value::named(result_type, "%cast.result"),
                      .operand = bir::Value::named(source_type, "%cast.operand"),
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

  std::vector<prepare::PreparedF128Carrier> carriers;
  if (result_type == bir::TypeKind::F128) {
    carriers.push_back(dispatch_f128_register_carrier(
        function_name, prepare::PreparedValueId{260}, result_name, "q4"));
  }
  if (source_type == bir::TypeKind::F128) {
    carriers.push_back(dispatch_f128_register_carrier(
        function_name, prepare::PreparedValueId{261}, operand_name, "q6"));
  }

  prepare::PreparedF128RuntimeHelper helper{
      .function_name = function_name,
      .block_index = 0,
      .instruction_index = 0,
      .source_cast_opcode = opcode,
      .source_type = source_type,
      .result_type = result_type,
      .result_value_id = result_type == bir::TypeKind::F128 ? prepare::PreparedValueId{260}
                                                            : prepare::PreparedValueId{262},
      .result_value_name = result_name,
      .operand_value_id = source_type == bir::TypeKind::F128 ? prepare::PreparedValueId{261}
                                                             : prepare::PreparedValueId{263},
      .operand_value_name = operand_name,
      .helper_family = prepare::PreparedF128RuntimeHelperFamily::Cast,
      .helper_kind = result_type == bir::TypeKind::F128
                         ? prepare::PreparedF128RuntimeHelperKind::F64ToF128
                         : prepare::PreparedF128RuntimeHelperKind::F128ToF32,
      .callee_name = result_type == bir::TypeKind::F128 ? "__extenddftf2" : "__trunctfsf2",
      .result_ownership =
          result_type == bir::TypeKind::F128
              ? prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier
              : prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue,
      .resource_policy =
          prepare::PreparedF128RuntimeHelper::ResourcePolicy{
              .call_boundary = true,
              .runtime_helper_callee = true,
              .caller_saved_clobbers = true,
              .preserves_source_operation_identity = true,
          },
      .abi_policy =
          prepare::PreparedF128RuntimeHelper::AbiPolicy{
              .transition =
                  result_type == bir::TypeKind::F128
                      ? prepare::PreparedF128RuntimeHelperAbiTransition::
                            DirectScalarArgumentAndF128Result
                      : prepare::PreparedF128RuntimeHelperAbiTransition::
                            DirectF128ArgumentAndScalarResult,
              .argument_bank = result_type == bir::TypeKind::F128
                                   ? prepare::PreparedRegisterBank::Fpr
                                   : prepare::PreparedRegisterBank::Vreg,
              .result_bank = result_type == bir::TypeKind::F128
                                 ? prepare::PreparedRegisterBank::Vreg
                                 : prepare::PreparedRegisterBank::Fpr,
              .argument_count = 1,
              .result_count = 1,
              .width_bytes = result_type == bir::TypeKind::F128 ? std::size_t{16}
                                                                : std::size_t{4},
          },
      .live_preservation_policy =
          prepare::PreparedF128RuntimeHelper::LivePreservationPolicy{
              .evaluated = true,
              .caller_saved_clobbers_modeled = true,
              .no_additional_live_preservation_required = true,
          },
      .selected_call_ownership =
          prepare::PreparedF128RuntimeHelper::SelectedCallOwnershipPolicy{
              .owns_terminal_call = true,
              .has_callee_identity = true,
              .has_resource_policy = true,
              .has_clobber_policy = true,
              .has_abi_bindings = true,
              .has_marshaling = true,
              .has_live_preservation = true,
          },
      .clobbered_registers =
          {prepare::PreparedClobberedRegister{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .register_name = "q16",
              .contiguous_width = 1,
              .occupied_register_names = {"q16"},
          }},
  };
  if (result_type == bir::TypeKind::F128) {
    helper.result_carrier = dispatch_f128_helper_carrier_binding(carriers.front());
    helper.scalar_operand =
        prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
            .value_id = helper.operand_value_id,
            .value_name = helper.operand_value_name,
            .type = source_type,
            .width_bytes = 8,
            .register_bank = prepare::PreparedRegisterBank::Fpr,
            .home_kind = prepare::PreparedValueHomeKind::Register,
            .register_name = std::string{"d9"},
        };
    helper.scalar_operand_abi_argument =
        dispatch_f128_scalar_fp_abi_binding(
            helper.operand_value_id,
            helper.operand_value_name,
            source_type,
            std::size_t{0},
            "d0",
            prepare::PreparedRegisterSlotPool::CallArgument);
    helper.result_abi_result = dispatch_f128_helper_abi_binding(
        helper.result_value_id, helper.result_value_name, std::nullopt, 0, "q0",
        prepare::PreparedRegisterSlotPool::CallResult);
    helper.scalar_operand_argument_move =
        prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
            .direction = prepare::PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument,
            .scalar_result = *helper.scalar_operand,
            .abi_register = *helper.scalar_operand_abi_argument,
        };
    helper.result_unmarshal_move = dispatch_f128_helper_marshaling_move(
        *helper.result_carrier, *helper.result_abi_result,
        prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier);
  } else {
    helper.lhs_value_id = helper.operand_value_id;
    helper.lhs_value_name = helper.operand_value_name;
    helper.lhs_carrier = dispatch_f128_helper_carrier_binding(carriers.front());
    helper.lhs_abi_argument = dispatch_f128_helper_abi_binding(
        helper.lhs_value_id, helper.lhs_value_name, std::size_t{0}, 0, "q0",
        prepare::PreparedRegisterSlotPool::CallArgument);
    helper.lhs_argument_move = dispatch_f128_helper_marshaling_move(
        *helper.lhs_carrier, *helper.lhs_abi_argument,
        prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument);
    helper.scalar_result =
        prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
            .value_id = helper.result_value_id,
            .value_name = helper.result_value_name,
            .type = result_type,
            .width_bytes = 4,
            .register_bank = prepare::PreparedRegisterBank::Fpr,
            .home_kind = prepare::PreparedValueHomeKind::Register,
            .register_name = std::string{"s9"},
        };
    helper.result_abi_result = dispatch_f128_scalar_fp_abi_binding(
        helper.result_value_id,
        helper.result_value_name,
        result_type,
        std::nullopt,
        "s0",
        prepare::PreparedRegisterSlotPool::CallResult);
    helper.scalar_result_unmarshal_move =
        prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
            .direction = prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar,
            .scalar_result = *helper.scalar_result,
            .abi_register = *helper.result_abi_result,
        };
  }
  prepared.f128_runtime_helpers.functions.push_back(
      prepare::PreparedF128RuntimeHelperFunction{
          .function_name = function_name,
          .helpers = {std::move(helper)},
      });
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers = std::move(carriers),
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_f128_comparison_helper_operation(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Eq,
    bool include_helper = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.f128.cmp");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.f128.cmp.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.f128.cmp.entry");
  const auto result_name = prepared.names.value_names.intern("%cmp.result");
  const auto lhs_name = prepared.names.value_names.intern("%cmp.lhs.f128");
  const auto rhs_name = prepared.names.value_names.intern("%cmp.rhs.f128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.f128.cmp",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.f128.cmp.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = opcode,
                      .result = bir::Value::named(bir::TypeKind::I1, "%cmp.result"),
                      .operand_type = bir::TypeKind::F128,
                      .lhs = bir::Value::named(bir::TypeKind::F128, "%cmp.lhs.f128"),
                      .rhs = bir::Value::named(bir::TypeKind::F128, "%cmp.rhs.f128"),
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
  std::vector<prepare::PreparedF128Carrier> carriers = {
      dispatch_f128_register_carrier(
          function_name, prepare::PreparedValueId{351}, lhs_name, "q6"),
      dispatch_f128_register_carrier(
          function_name, prepare::PreparedValueId{352}, rhs_name, "q8"),
  };
  if (include_helper) {
    auto helper = dispatch_f128_runtime_helper(function_name,
                                               0,
                                               bir::BinaryOpcode::Add,
                                               prepare::PreparedF128RuntimeHelperKind::Add,
                                               "__addtf3",
                                               carriers[0],
                                               carriers[0],
                                               carriers[1]);
    helper.result_value_id = prepare::PreparedValueId{350};
    helper.result_value_name = result_name;
    dispatch_retarget_f128_helper_as_compare(
        helper, opcode, prepare::PreparedF128RuntimeHelperKind::Eq, "__eqtf2");
    prepared.f128_runtime_helpers.functions.push_back(
        prepare::PreparedF128RuntimeHelperFunction{
            .function_name = function_name,
            .helpers = {std::move(helper)},
        });
  }
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers = std::move(carriers),
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_i128_shift_operation() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.i128.shift");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.i128.shift.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.i128.shift.entry");
  const auto result_name = prepared.names.value_names.intern("%shifted.i128");
  const auto source_name = prepared.names.value_names.intern("%source.i128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.i128.shift",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.i128.shift.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = bir::BinaryOpcode::LShr,
                      .result = bir::Value::named(bir::TypeKind::I128, "%shifted.i128"),
                      .operand_type = bir::TypeKind::I128,
                      .lhs = bir::Value::named(bir::TypeKind::I128, "%source.i128"),
                      .rhs = bir::Value::immediate_i32(12),
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
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
  });
  prepared.i128_carriers.functions.push_back(prepare::PreparedI128CarrierFunction{
      .function_name = function_name,
      .carriers =
          {dispatch_i128_register_pair_carrier(function_name,
                                               prepare::PreparedValueId{130},
                                               result_name,
                                               12),
           dispatch_i128_register_pair_carrier(function_name,
                                               prepare::PreparedValueId{131},
                                               source_name,
                                               14)},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_i128_compare_operation(bool include_result_storage = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.i128.compare");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.i128.compare.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.i128.compare.entry");
  const auto result_name = prepared.names.value_names.intern("%cmp.i128");
  const auto lhs_name = prepared.names.value_names.intern("%lhs.cmp.i128");
  const auto rhs_name = prepared.names.value_names.intern("%rhs.cmp.i128");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.i128.compare",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.i128.compare.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = bir::BinaryOpcode::Ult,
                      .result = bir::Value::named(bir::TypeKind::I1, "%cmp.i128"),
                      .operand_type = bir::TypeKind::I128,
                      .lhs = bir::Value::named(bir::TypeKind::I128, "%lhs.cmp.i128"),
                      .rhs = bir::Value::named(bir::TypeKind::I128, "%rhs.cmp.i128"),
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
          include_result_storage
              ? std::vector<prepare::PreparedValueHome>{prepare::PreparedValueHome{
                    .value_id = prepare::PreparedValueId{140},
                    .function_name = function_name,
                    .value_name = result_name,
                    .kind = prepare::PreparedValueHomeKind::Register,
                    .register_name = std::string{"w3"},
                }}
              : std::vector<prepare::PreparedValueHome>{},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          include_result_storage
              ? std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{140}, result_name, "w3")}
              : std::vector<prepare::PreparedStoragePlanValue>{},
  });
  prepared.i128_carriers.functions.push_back(prepare::PreparedI128CarrierFunction{
      .function_name = function_name,
      .carriers =
          {dispatch_i128_register_pair_carrier(function_name,
                                               prepare::PreparedValueId{141},
                                               lhs_name,
                                               16),
           dispatch_i128_register_pair_carrier(function_name,
                                               prepare::PreparedValueId{142},
                                               rhs_name,
                                               18)},
  });
  return prepared;
}

enum class StoreDispatchFixtureKind {
  FrameSlot,
  PointerValue,
  GlobalSymbol,
  StringConstant,
};

prepare::PreparedBirModule prepared_with_store(StoreDispatchFixtureKind kind,
                                               bool include_storage = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.store");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.store.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.store.entry");
  const auto stored_name = prepared.names.value_names.intern("%stored");
  const auto pointer_name = prepared.names.value_names.intern("%ptr");
  const auto global_name = prepared.names.link_names.intern("g.store");
  const auto string_name = prepared.names.link_names.intern(".L.store");
  prepared.stack_layout = prepare::PreparedStackLayout{
      .frame_slots =
          {prepare::PreparedFrameSlot{
              .slot_id = prepare::PreparedFrameSlotId{21},
              .object_id = prepare::PreparedObjectId{21},
              .function_name = function_name,
              .offset_bytes = 0,
              .size_bytes = 16,
              .align_bytes = 16,
              .fixed_location = true,
          }},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
  };

  bir::Inst store_inst;
  prepare::PreparedAddress address;
  bir::AddressSpace address_space = bir::AddressSpace::Fs;
  bool is_volatile = true;
  if (kind == StoreDispatchFixtureKind::FrameSlot) {
    store_inst = bir::StoreLocalInst{
        .slot_id = c4c::SlotNameId{5},
        .value = bir::Value::named(bir::TypeKind::I32, "%stored"),
        .byte_offset = 12,
        .align_bytes = 4,
        .address =
            bir::MemoryAddress{
                .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                .byte_offset = 12,
                .size_bytes = 4,
                .align_bytes = 4,
                .address_space = bir::AddressSpace::Fs,
                .is_volatile = true,
                .base_slot_id = c4c::SlotNameId{5},
            },
    };
    address = prepare::PreparedAddress{
        .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
        .frame_slot_id = prepare::PreparedFrameSlotId{21},
        .byte_offset = 12,
        .size_bytes = 4,
        .align_bytes = 4,
        .can_use_base_plus_offset = true,
    };
  } else if (kind == StoreDispatchFixtureKind::PointerValue) {
    store_inst = bir::StoreLocalInst{
        .slot_id = c4c::SlotNameId{5},
        .value = bir::Value::named(bir::TypeKind::I32, "%stored"),
        .byte_offset = 24,
        .align_bytes = 8,
        .address =
            bir::MemoryAddress{
                .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ptr"),
                .byte_offset = 24,
                .size_bytes = 8,
                .align_bytes = 8,
                .address_space = bir::AddressSpace::Tls,
            },
    };
    address_space = bir::AddressSpace::Tls;
    is_volatile = false;
    address = prepare::PreparedAddress{
        .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
        .pointer_value_name = pointer_name,
        .byte_offset = 24,
        .size_bytes = 8,
        .align_bytes = 8,
        .can_use_base_plus_offset = true,
    };
  } else if (kind == StoreDispatchFixtureKind::GlobalSymbol) {
    store_inst = bir::StoreGlobalInst{
        .global_name_id = global_name,
        .value = bir::Value::named(bir::TypeKind::I32, "%stored"),
        .byte_offset = 16,
        .align_bytes = 4,
        .address =
            bir::MemoryAddress{
                .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                .byte_offset = 16,
                .size_bytes = 4,
                .align_bytes = 4,
                .address_space = bir::AddressSpace::Gs,
                .is_volatile = true,
                .base_link_name_id = global_name,
            },
    };
    address_space = bir::AddressSpace::Gs;
    address = prepare::PreparedAddress{
        .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
        .symbol_name = global_name,
        .byte_offset = 16,
        .size_bytes = 4,
        .align_bytes = 4,
        .can_use_base_plus_offset = true,
    };
  } else {
    store_inst = bir::StoreLocalInst{
        .slot_id = c4c::SlotNameId{5},
        .value = bir::Value::named(bir::TypeKind::I32, "%stored"),
        .byte_offset = 0,
        .align_bytes = 4,
        .address =
            bir::MemoryAddress{
                .base_kind = bir::MemoryAddress::BaseKind::StringConstant,
                .base_name = ".L.store",
                .byte_offset = 0,
                .size_bytes = 4,
                .align_bytes = 4,
                .address_space = bir::AddressSpace::Default,
                .base_link_name_id = string_name,
            },
    };
    address_space = bir::AddressSpace::Default;
    is_volatile = false;
    address = prepare::PreparedAddress{
        .base_kind = prepare::PreparedAddressBaseKind::StringConstant,
        .symbol_name = string_name,
        .byte_offset = 0,
        .size_bytes = 4,
        .align_bytes = 4,
        .can_use_base_plus_offset = true,
    };
  }

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.store",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.store.entry",
              .insts = {store_inst},
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
          {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{11},
               .function_name = function_name,
               .value_name = stored_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "w1",
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{12},
               .function_name = function_name,
               .value_name = pointer_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "x2",
           }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          include_storage
              ? std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{11}, stored_name, "w1"),
                    register_storage(prepare::PreparedValueId{12}, pointer_name, "x2")}
              : std::vector<prepare::PreparedStoragePlanValue>{},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = entry_label,
              .inst_index = 0,
              .stored_value_name = stored_name,
              .address_space = address_space,
              .is_volatile = is_volatile,
              .address = address,
          }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_simple_integer_cast(
    bir::CastOpcode opcode = bir::CastOpcode::SExt,
    bir::TypeKind source_type = bir::TypeKind::I32,
    bir::TypeKind result_type = bir::TypeKind::I64,
    bool include_result_storage = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.cast");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.cast.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.cast.entry");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto result_name = prepared.names.value_names.intern("%cast");
  const char* source_register = source_type == bir::TypeKind::I64 ? "x1" : "w1";
  const char* result_register = result_type == bir::TypeKind::I64 ? "x0" : "w0";

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.cast",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.cast.entry",
              .insts =
                  {bir::CastInst{
                      .opcode = opcode,
                      .result = bir::Value::named(result_type, "%cast"),
                      .operand = bir::Value::named(source_type, "%src"),
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
          {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{20},
               .function_name = function_name,
               .value_name = source_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = source_register,
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{21},
               .function_name = function_name,
               .value_name = result_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = result_register,
           }},
  });
  std::vector<prepare::PreparedStoragePlanValue> values = {
      register_storage(prepare::PreparedValueId{20}, source_name, source_register),
  };
  if (include_result_storage) {
    values.push_back(
        register_storage(prepare::PreparedValueId{21}, result_name, result_register));
  }
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = values,
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_f64_scalar_alu(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Mul,
    bool use_fpr_storage = true,
    bir::TypeKind type = bir::TypeKind::F64) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.fp");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.fp.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.fp.entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto result_name = prepared.names.value_names.intern("%fp");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.fp",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.fp.entry",
              .insts =
                  {bir::BinaryInst{
                      .opcode = opcode,
                      .result = bir::Value::named(type, "%fp"),
                      .operand_type = type,
                      .lhs = bir::Value::named(type, "%lhs"),
                      .rhs = bir::Value::named(type, "%rhs"),
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
  const char* lhs_register = type == bir::TypeKind::F32 ? "s1" : "d1";
  const char* rhs_register = type == bir::TypeKind::F32 ? "s2" : "d2";
  const char* result_register = type == bir::TypeKind::F32 ? "s0" : "d0";
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{30},
               .function_name = function_name,
               .value_name = lhs_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = lhs_register,
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{31},
               .function_name = function_name,
               .value_name = rhs_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = rhs_register,
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{32},
               .function_name = function_name,
               .value_name = result_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = result_register,
           }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          use_fpr_storage
              ? std::vector<prepare::PreparedStoragePlanValue>{
                    fpr_storage(prepare::PreparedValueId{30}, lhs_name, lhs_register),
                    fpr_storage(prepare::PreparedValueId{31}, rhs_name, rhs_register),
                    fpr_storage(prepare::PreparedValueId{32}, result_name, result_register)}
              : std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{30}, lhs_name, lhs_register),
                    register_storage(prepare::PreparedValueId{31}, rhs_name, rhs_register),
                    register_storage(prepare::PreparedValueId{32}, result_name, result_register)},
  });
  return prepared;
}

const char* dispatch_register_for_type(bir::TypeKind type, unsigned index) {
  if (type == bir::TypeKind::F64) {
    return index == 0 ? "d0" : "d1";
  }
  if (type == bir::TypeKind::F32) {
    return index == 0 ? "s0" : "s1";
  }
  if (type == bir::TypeKind::I64 || type == bir::TypeKind::Ptr) {
    return index == 0 ? "x0" : "x1";
  }
  return index == 0 ? "w0" : "w1";
}

prepare::PreparedStoragePlanValue dispatch_storage_for_type(prepare::PreparedValueId value_id,
                                                            c4c::ValueNameId value_name,
                                                            bir::TypeKind type,
                                                            const char* register_name) {
  if (type == bir::TypeKind::F32 || type == bir::TypeKind::F64) {
    return fpr_storage(value_id, value_name, register_name);
  }
  return register_storage(value_id, value_name, register_name);
}

prepare::PreparedBirModule prepared_with_conversion_cast(
    bir::CastOpcode opcode = bir::CastOpcode::SIToFP,
    bir::TypeKind source_type = bir::TypeKind::I32,
    bir::TypeKind result_type = bir::TypeKind::F64,
    bool use_correct_banks = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.convert");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.convert.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.convert.entry");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto result_name = prepared.names.value_names.intern("%cast");
  const char* source_register = dispatch_register_for_type(source_type, 1);
  const char* result_register = dispatch_register_for_type(result_type, 0);

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.convert",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.convert.entry",
              .insts =
                  {bir::CastInst{
                      .opcode = opcode,
                      .result = bir::Value::named(result_type, "%cast"),
                      .operand = bir::Value::named(source_type, "%src"),
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
          {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{40},
               .function_name = function_name,
               .value_name = source_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = source_register,
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{41},
               .function_name = function_name,
               .value_name = result_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = result_register,
           }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          use_correct_banks
              ? std::vector<prepare::PreparedStoragePlanValue>{
                    dispatch_storage_for_type(
                        prepare::PreparedValueId{40}, source_name, source_type, source_register),
                    dispatch_storage_for_type(
                        prepare::PreparedValueId{41}, result_name, result_type, result_register)}
              : std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{40}, source_name, source_register),
                    register_storage(prepare::PreparedValueId{41}, result_name, result_register)},
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

int block_dispatch_selects_complete_inline_asm_machine_record() {
  auto prepared =
      prepared_with_inline_asm_carrier(
          InlineAsmCarrierFixtureKind::SupportedTemplateModifier);
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected complete inline-asm carrier plus return without diagnostics");
  }
  const auto* assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &block.instructions.front().target.payload);
  if (assembler == nullptr ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::Assembler ||
      block.instructions.front().target.surface !=
          aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected) {
    return fail("expected selected inline-asm assembler machine record");
  }
  if (!assembler->has_inline_asm_payload ||
      assembler->inline_asm_template != "add %w0, %w1, %w2\nmov %x0, %x1" ||
      assembler->inline_asm_constraints != "=r,0,r,i" ||
      !assembler->side_effects || !assembler->inline_asm_has_template_modifiers ||
      assembler->inline_asm_has_named_operand_references) {
    return fail("expected inline-asm record to preserve raw payload facts");
  }
  if (!assembler->inline_asm_result.has_value() ||
      !assembler->inline_asm_result_value_name.has_value() ||
      !assembler->inline_asm_result_home.has_value()) {
    return fail("expected inline-asm output facts on selected record");
  }
  if (assembler->operands.size() != 4 ||
      assembler->inline_asm_operands.size() != 4 ||
      block.instructions.front().target.defs.size() != 1 ||
      block.instructions.front().target.uses.size() != 3) {
    return fail("expected inline-asm selected operands, defs, and uses");
  }
  if (assembler->inline_asm_operands[0].kind !=
          bir::InlineAsmOperandKind::RegisterOutput ||
      assembler->inline_asm_operands[0].constraint != "=r" ||
      !assembler->inline_asm_operands[0].output_index.has_value() ||
      !assembler->inline_asm_operands[0].home.has_value() ||
      assembler->inline_asm_operands[0].home->register_name.value_or("") != "w3") {
    return fail("expected register output placeholder and home");
  }
  if (assembler->inline_asm_operands[1].kind !=
          bir::InlineAsmOperandKind::TiedInput ||
      !assembler->inline_asm_operands[1].tied_output_index.has_value() ||
      !assembler->inline_asm_operands[1].home.has_value() ||
      assembler->inline_asm_operands[1].home->register_name.value_or("") != "w3") {
    return fail("expected tied inline-asm input facts");
  }
  if (assembler->inline_asm_operands[2].kind !=
          bir::InlineAsmOperandKind::RegisterInput ||
      assembler->inline_asm_operands[2].name.value_or("") != "rhs" ||
      !assembler->inline_asm_operands[2].home.has_value() ||
      assembler->inline_asm_operands[2].home->register_name.value_or("") != "w5") {
    return fail("expected named register input fact when retained metadata has one");
  }
  if (assembler->inline_asm_operands[3].kind !=
          bir::InlineAsmOperandKind::IntegerImmediateInput ||
      assembler->inline_asm_operands[3].immediate_value.value_or(0) != 7 ||
      assembler->inline_asm_operands[3].selected_operand->kind !=
          aarch64_codegen::OperandKind::Immediate) {
    return fail("expected integer immediate inline-asm operand fact");
  }
  if (block.instructions.front().target.side_effects.size() != 1 ||
      block.instructions.front().target.side_effects.front() !=
          aarch64_codegen::MachineSideEffectKind::InlineAssembly) {
    return fail("expected inline-asm side-effect marker on selected record");
  }
  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(
      block.instructions.front().target);
  if (!printed.ok || printed.instruction_lines.size() != 2 ||
      printed.instruction_lines[0] != "add w3, w3, w5" ||
      printed.instruction_lines[1] != "mov x3, x3") {
    return fail("expected supported inline-asm modifiers to print after dispatch");
  }
  if (!std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.back().target.payload)) {
    return fail("expected return after selected inline-asm record");
  }
  return 0;
}

int block_dispatch_selects_alias_aware_inline_asm_tied_home() {
  auto prepared =
      prepared_with_inline_asm_carrier(
          InlineAsmCarrierFixtureKind::AliasAwareTiedInputHome);
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected alias-aware inline-asm tied home to select");
  }
  const auto* assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &block.instructions.front().target.payload);
  if (assembler == nullptr || assembler->inline_asm_operands.size() < 2 ||
      assembler->inline_asm_operands[1].home->register_name.value_or("") !=
          "x3" ||
      !assembler->inline_asm_operands[1]
           .home->target_register_identity.has_value() ||
      assembler->inline_asm_operands[1]
              .home->target_register_identity->physical_index != 3) {
    return fail("expected selected tied input to preserve alias-aware authority");
  }
  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(
      block.instructions.front().target);
  if (!printed.ok || printed.instruction_lines.size() != 1 ||
      printed.instruction_lines.front() != "add w3, x3, w5") {
    return fail("expected alias-aware tied inline-asm operands to print");
  }
  return 0;
}

int block_dispatch_selects_inline_asm_structured_clobbers() {
  auto prepared =
      prepared_with_inline_asm_carrier(InlineAsmCarrierFixtureKind::SupportedClobbers);
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected inline-asm structured clobbers to select without diagnostics");
  }
  const auto& instruction = block.instructions.front().target;
  const auto* assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(&instruction.payload);
  if (assembler == nullptr || assembler->inline_asm_clobbers.size() != 3 ||
      assembler->inline_asm_clobbers[0] != "x7" ||
      assembler->inline_asm_clobbers[1] != "memory" ||
      assembler->inline_asm_clobbers[2] != "cc") {
    return fail("expected selected inline-asm record to carry structured clobber list");
  }
  if (instruction.clobbers.size() != 3 ||
      instruction.clobbers[0].kind !=
          aarch64_codegen::MachineEffectResourceKind::Register ||
      instruction.clobbers[0].reg != aarch64_abi::x_register(7) ||
      instruction.clobbers[1].kind !=
          aarch64_codegen::MachineEffectResourceKind::Memory ||
      instruction.clobbers[2].kind !=
          aarch64_codegen::MachineEffectResourceKind::Flags) {
    return fail("expected inline-asm structured clobbers to become machine effects");
  }

  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(
      instruction);
  if (!printed.ok || printed.instruction_lines.size() != 1 ||
      printed.instruction_lines.front() != "add w3, w3, w5") {
    return fail("expected inline-asm printer to accept selected clobber facts");
  }
  return 0;
}

int block_dispatch_selects_inline_asm_prepared_memory_and_address_operands() {
  auto prepared =
      prepared_with_inline_asm_carrier(InlineAsmCarrierFixtureKind::SupportedMemoryInputSelection);
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
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.empty()) {
    return fail("expected prepared inline-asm memory input to lower without diagnostics");
  }
  const auto* assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &block.instructions.front().target.payload);
  if (assembler == nullptr || assembler->inline_asm_operands.size() != 4 ||
      assembler->inline_asm_operands[2].kind != bir::InlineAsmOperandKind::MemoryInput ||
      !assembler->inline_asm_operands[2].selected_operand.has_value()) {
    return fail("expected selected inline-asm memory operand record");
  }
  const auto* memory = std::get_if<aarch64_codegen::MemoryOperand>(
      &assembler->inline_asm_operands[2].selected_operand->payload);
  if (assembler->inline_asm_operands[2].selected_operand->kind !=
          aarch64_codegen::OperandKind::Memory ||
      memory == nullptr ||
      memory->base_kind != aarch64_codegen::MemoryBaseKind::PointerValue ||
      !memory->base_register.has_value() ||
      !memory->pointer_value_name.has_value() ||
      memory->byte_offset != 8 ||
      memory->size_bytes != 4 ||
      memory->align_bytes != 4) {
    return fail("expected prepared pointer address to become a selected memory operand");
  }
  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(
      block.instructions.front().target);
  if (!printed.ok || printed.instruction_lines.size() != 1 ||
      printed.instruction_lines.front() != "ldr w3, [x5, #8]") {
    return fail("expected selected prepared inline-asm memory address to print");
  }

  prepared =
      prepared_with_inline_asm_carrier(InlineAsmCarrierFixtureKind::SupportedAddressInputSelection);
  const auto& address_function_cf = prepared.control_flow.functions.front();
  const auto& address_block_cf = address_function_cf.blocks.front();
  const auto address_function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, address_function_cf);
  const auto address_block_context =
      aarch64_codegen::make_block_lowering_context(
          address_function_context, address_block_cf, 0);

  aarch64_module::MachineBlock address_block;
  aarch64_module::ModuleLoweringDiagnostics address_diagnostics;
  const auto address_result =
      aarch64_codegen::dispatch_prepared_block(
          address_block_context, address_block, address_diagnostics);
  if (!address_diagnostics.empty() || address_result.visited_operations != 1 ||
      address_result.emitted_instructions != 2 || address_block.instructions.empty()) {
    return fail("expected prepared inline-asm address input to lower without diagnostics");
  }
  const auto* address_assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &address_block.instructions.front().target.payload);
  if (address_assembler == nullptr ||
      address_assembler->inline_asm_operands.size() != 4 ||
      address_assembler->inline_asm_operands[2].kind !=
          bir::InlineAsmOperandKind::AddressInput ||
      !address_assembler->inline_asm_operands[2].selected_operand.has_value()) {
    return fail("expected selected inline-asm address operand record");
  }
  const auto* address_memory = std::get_if<aarch64_codegen::MemoryOperand>(
      &address_assembler->inline_asm_operands[2].selected_operand->payload);
  if (address_assembler->inline_asm_operands[2].selected_operand->kind !=
          aarch64_codegen::OperandKind::Memory ||
      address_memory == nullptr ||
      address_memory->base_kind != aarch64_codegen::MemoryBaseKind::PointerValue ||
      !address_memory->base_register.has_value() ||
      !address_memory->pointer_value_name.has_value() ||
      address_memory->byte_offset != 16 ||
      address_memory->size_bytes != 8 ||
      address_memory->align_bytes != 8) {
    return fail("expected prepared pointer address input to become a selected address operand");
  }
  const auto address_printed = aarch64_codegen::print_machine_instruction_line_payloads(
      address_block.instructions.front().target);
  if (!address_printed.ok || address_printed.instruction_lines.size() != 1 ||
      address_printed.instruction_lines.front() != "adr x3, [x5, #16]") {
    return fail("expected selected prepared inline-asm address input to print");
  }
  return 0;
}

int block_dispatch_keeps_malformed_inline_asm_carriers_fail_closed() {
  const std::array cases{
      std::pair{InlineAsmCarrierFixtureKind::Incomplete,
                std::string_view{"missing_result_home"}},
      std::pair{InlineAsmCarrierFixtureKind::MissingResultHome,
                std::string_view{"missing_result_register_home"}},
      std::pair{InlineAsmCarrierFixtureKind::MissingTiedInputHome,
                std::string_view{"missing_tied_input_register_home"}},
      std::pair{InlineAsmCarrierFixtureKind::MissingTiedOutputIndex,
                std::string_view{"missing_tied_output_index"}},
      std::pair{InlineAsmCarrierFixtureKind::AllocatorDependentTiedInputHome,
                std::string_view{
                    "tied_input_output_home_requires_concrete_registers"}},
      std::pair{InlineAsmCarrierFixtureKind::TargetInvalidTiedInputHome,
                std::string_view{"target_invalid_tied_input_register_home"}},
      std::pair{InlineAsmCarrierFixtureKind::ClassInvalidTiedInputHome,
                std::string_view{
                    "tied_input_output_home_incompatible_register_class"}},
      std::pair{InlineAsmCarrierFixtureKind::MismatchedTiedInputHome,
                std::string_view{"tied_input_output_home_mismatch"}},
      std::pair{InlineAsmCarrierFixtureKind::MissingTiedHomeAuthority,
                std::string_view{"missing_tied_home_coallocation_authority"}},
      std::pair{InlineAsmCarrierFixtureKind::AuthorityOutputMismatchedTiedInputHome,
                std::string_view{
                    "tied_home_coallocation_authority_output_mismatch"}},
      std::pair{InlineAsmCarrierFixtureKind::AuthorityMismatchedTiedInputHome,
                std::string_view{
                    "tied_home_coallocation_authority_home_mismatch"}},
      std::pair{InlineAsmCarrierFixtureKind::UnsupportedOperand,
                std::string_view{"unsupported_inline_asm_operand_kind"}},
      std::pair{InlineAsmCarrierFixtureKind::UnsupportedMemoryInputSelection,
                std::string_view{"unsupported_inline_asm_memory_address_selection"}},
      std::pair{InlineAsmCarrierFixtureKind::UnsupportedAddressInputSelection,
                std::string_view{"unsupported_inline_asm_address_selection"}},
      std::pair{InlineAsmCarrierFixtureKind::UnsupportedClobber,
                std::string_view{"unsupported_inline_asm_clobber_operand_kind"}},
      std::pair{InlineAsmCarrierFixtureKind::UnsupportedTemplateModifier,
                std::string_view{"unsupported_template_modifiers"}},
      std::pair{InlineAsmCarrierFixtureKind::UnknownClobber,
                std::string_view{"unknown_inline_asm_clobber"}},
      std::pair{InlineAsmCarrierFixtureKind::MalformedClobber,
                std::string_view{"malformed_inline_asm_clobber"}},
      std::pair{InlineAsmCarrierFixtureKind::TargetInvalidClobber,
                std::string_view{"target_invalid_inline_asm_clobber"}},
  };

  for (const auto& [kind, expected_fact] : cases) {
    auto prepared = prepared_with_inline_asm_carrier(kind);
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

    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.size() != 1) {
      return fail("expected malformed inline-asm carrier to remain diagnostic-only");
    }
    if (diagnostics.entries.front().kind !=
            aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
        diagnostics.entries.front().instruction_family !=
            aarch64_module::InstructionLoweringFamily::Call ||
        diagnostics.entries.front().message.find(expected_fact) ==
            std::string::npos) {
      std::cerr << "[FAIL] expected fact: " << expected_fact
                << "\n[FAIL] actual diagnostic: "
                << diagnostics.entries.front().message << "\n";
      return fail("expected inline-asm fail-closed diagnostic fact");
    }
    if (!std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
            block.instructions.front().target.payload)) {
      return fail("expected malformed inline-asm carrier to emit only return");
    }
  }
  return 0;
}

int block_dispatch_selects_named_inline_asm_machine_record() {
  auto prepared =
      prepared_with_inline_asm_carrier(InlineAsmCarrierFixtureKind::NamedReferences);
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected named inline-asm carrier plus return without diagnostics");
  }
  const auto* assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &block.instructions.front().target.payload);
  if (assembler == nullptr || !assembler->inline_asm_has_named_operand_references) {
    return fail("expected selected inline-asm record to preserve named reference fact");
  }
  if (assembler->inline_asm_operands.size() != 4 ||
      assembler->inline_asm_operands[2].name.value_or("") != "rhs" ||
      assembler->inline_asm_operands[3].name.value_or("") != "imm") {
    return fail("expected retained inline-asm operand names on selected record");
  }
  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(
      block.instructions.front().target);
  if (!printed.ok || printed.instruction_lines.size() != 2 ||
      printed.instruction_lines[0] != "add w3, w3, w5" ||
      printed.instruction_lines[1] != "mov x3, #7") {
    return fail("expected named inline-asm operands to print after dispatch");
  }
  return 0;
}

int block_dispatch_selects_ordered_atomic_load_store_and_fence_records() {
  auto prepared = prepared_with_atomic_memory_carriers(true);
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

  if (result.visited_operations != 6 || !result.visited_terminator ||
      result.emitted_instructions != 7 || block.instructions.size() != 7 ||
      !diagnostics.empty()) {
    return fail("expected selected atomic records plus return without diagnostics");
  }

  const auto* load =
      std::get_if<aarch64_codegen::AtomicMemoryInstructionRecord>(
          &block.instructions[0].target.payload);
  const auto* store =
      std::get_if<aarch64_codegen::AtomicMemoryInstructionRecord>(
          &block.instructions[1].target.payload);
  const auto* fence =
      std::get_if<aarch64_codegen::AtomicMemoryInstructionRecord>(
          &block.instructions[2].target.payload);
  const auto* rmw =
      std::get_if<aarch64_codegen::AtomicMemoryInstructionRecord>(
          &block.instructions[3].target.payload);
  const auto* compare_bool =
      std::get_if<aarch64_codegen::AtomicMemoryInstructionRecord>(
          &block.instructions[4].target.payload);
  const auto* compare_old =
      std::get_if<aarch64_codegen::AtomicMemoryInstructionRecord>(
          &block.instructions[5].target.payload);
  if (load == nullptr || store == nullptr || fence == nullptr || rmw == nullptr ||
      compare_bool == nullptr || compare_old == nullptr) {
    return fail("expected atomic selected payloads");
  }
  if (block.instructions[0].target.opcode != aarch64_codegen::MachineOpcode::AtomicLoad ||
      block.instructions[1].target.opcode != aarch64_codegen::MachineOpcode::AtomicStore ||
      block.instructions[2].target.opcode != aarch64_codegen::MachineOpcode::AtomicFence ||
      block.instructions[3].target.opcode != aarch64_codegen::MachineOpcode::AtomicRmw ||
      block.instructions[4].target.opcode !=
          aarch64_codegen::MachineOpcode::AtomicCompareExchange ||
      block.instructions[5].target.opcode !=
          aarch64_codegen::MachineOpcode::AtomicCompareExchange) {
    return fail("expected atomic-specific machine opcodes");
  }
  if (load->atomic_kind != aarch64_codegen::AtomicMemoryInstructionKind::Load ||
      load->value_type != bir::TypeKind::I32 ||
      load->width_bytes != 4 ||
      load->ordering != bir::AtomicOrdering::Acquire ||
      !load->acquire_semantics ||
      load->release_semantics ||
      load->address_space != bir::AddressSpace::Tls ||
      !load->result_value_id.has_value() ||
      !load->pointer_value_id.has_value() ||
      !load->result_register.has_value() ||
      load->result_register->occupied_registers.empty() ||
      load->result_register->occupied_registers.front() != "w1" ||
      !load->pointer_register.has_value() ||
      load->pointer_register->occupied_registers.empty() ||
      load->pointer_register->occupied_registers.front() != "x0" ||
      load->source_carrier == nullptr) {
    return fail("expected atomic load to preserve structured carrier fields");
  }
  if (store->atomic_kind != aarch64_codegen::AtomicMemoryInstructionKind::Store ||
      store->ordering != bir::AtomicOrdering::Release ||
      store->acquire_semantics ||
      !store->release_semantics ||
      store->address_space != bir::AddressSpace::Gs ||
      !store->stored_value_id.has_value() ||
      !store->stored_register.has_value() ||
      store->stored_register->occupied_registers.empty() ||
      store->stored_register->occupied_registers.front() != "w2" ||
      block.instructions[1].target.side_effects.size() != 2 ||
      block.instructions[1].target.side_effects[0] !=
          aarch64_codegen::MachineSideEffectKind::MemoryWrite ||
      block.instructions[1].target.side_effects[1] !=
          aarch64_codegen::MachineSideEffectKind::AtomicMemoryAccess) {
    return fail("expected atomic store to preserve release/value/side-effect facts");
  }
  if (fence->atomic_kind != aarch64_codegen::AtomicMemoryInstructionKind::Fence ||
      fence->ordering != bir::AtomicOrdering::SeqCst ||
      !fence->acquire_semantics ||
      !fence->release_semantics ||
      !fence->sequentially_consistent ||
      !fence->memory_barrier_required ||
      block.instructions[2].target.side_effects.size() != 3) {
    return fail("expected atomic fence to preserve barrier semantics");
  }
  if (rmw->atomic_kind != aarch64_codegen::AtomicMemoryInstructionKind::RmwLoop ||
      rmw->ordering != bir::AtomicOrdering::AcqRel ||
      rmw->rmw_opcode != bir::AtomicRmwOpcode::Add ||
      rmw->result_mode != bir::AtomicResultMode::OldValue ||
      !rmw->exclusive_retry_loop ||
      !rmw->acquire_semantics ||
      !rmw->release_semantics ||
      !rmw->result_register.has_value() ||
      rmw->result_register->occupied_registers.empty() ||
      rmw->result_register->occupied_registers.front() != "w4" ||
      !rmw->stored_register.has_value() ||
      rmw->stored_register->occupied_registers.empty() ||
      rmw->stored_register->occupied_registers.front() != "w2" ||
      !rmw->rmw_new_value_register.has_value() ||
      rmw->rmw_new_value_register->role !=
          aarch64_codegen::RegisterOperandRole::ReservedMirScratch ||
      rmw->rmw_new_value_register->occupied_registers.empty() ||
      rmw->rmw_new_value_register->occupied_registers.front() != "w9" ||
      !rmw->exclusive_status_register.has_value() ||
      rmw->exclusive_status_register->role !=
          aarch64_codegen::RegisterOperandRole::ReservedMirScratch ||
      rmw->exclusive_status_register->occupied_registers.empty() ||
      rmw->exclusive_status_register->occupied_registers.front() != "w10" ||
      block.instructions[3].target.side_effects.size() != 3) {
    return fail("expected atomic rmw loop to preserve old-value, retry, and scratch facts");
  }
  if (compare_bool->atomic_kind !=
          aarch64_codegen::AtomicMemoryInstructionKind::CompareExchangeLoop ||
      compare_bool->ordering != bir::AtomicOrdering::SeqCst ||
      compare_bool->failure_ordering != bir::AtomicOrdering::Acquire ||
      compare_bool->result_mode != bir::AtomicResultMode::BooleanSuccess ||
      !compare_bool->compare_exchange_result_is_boolean ||
      compare_bool->compare_exchange_result_is_old_value ||
      !compare_bool->exclusive_retry_loop ||
      !compare_bool->compare_exchange_failure_clears_monitor ||
      !compare_bool->expected_register.has_value() ||
      compare_bool->expected_register->occupied_registers.empty() ||
      compare_bool->expected_register->occupied_registers.front() != "w1" ||
      !compare_bool->desired_register.has_value() ||
      compare_bool->desired_register->occupied_registers.empty() ||
      compare_bool->desired_register->occupied_registers.front() != "w2" ||
      !compare_bool->result_register.has_value() ||
      compare_bool->result_register->occupied_registers.empty() ||
      compare_bool->result_register->occupied_registers.front() != "w3" ||
      !compare_bool->compare_loaded_register.has_value() ||
      compare_bool->compare_loaded_register->role !=
          aarch64_codegen::RegisterOperandRole::ReservedMirScratch ||
      compare_bool->compare_loaded_register->occupied_registers.empty() ||
      compare_bool->compare_loaded_register->occupied_registers.front() != "w9" ||
      !compare_bool->exclusive_status_register.has_value() ||
      compare_bool->exclusive_status_register->role !=
          aarch64_codegen::RegisterOperandRole::ReservedMirScratch ||
      compare_bool->exclusive_status_register->occupied_registers.empty() ||
      compare_bool->exclusive_status_register->occupied_registers.front() != "w10") {
    return fail("expected boolean compare-exchange loop facts to survive selection");
  }
  if (compare_old->atomic_kind !=
          aarch64_codegen::AtomicMemoryInstructionKind::CompareExchangeLoop ||
      compare_old->ordering != bir::AtomicOrdering::Acquire ||
      compare_old->failure_ordering != bir::AtomicOrdering::Relaxed ||
      compare_old->result_mode != bir::AtomicResultMode::OldValue ||
      compare_old->compare_exchange_result_is_boolean ||
      !compare_old->compare_exchange_result_is_old_value ||
      !compare_old->exclusive_retry_loop ||
      !compare_old->compare_exchange_failure_clears_monitor ||
      !compare_old->result_register.has_value() ||
      compare_old->result_register->occupied_registers.empty() ||
      compare_old->result_register->occupied_registers.front() != "w4" ||
      compare_old->compare_loaded_register.has_value() ||
      !compare_old->exclusive_status_register.has_value() ||
      compare_old->exclusive_status_register->role !=
          aarch64_codegen::RegisterOperandRole::ReservedMirScratch ||
      compare_old->exclusive_status_register->occupied_registers.empty() ||
      compare_old->exclusive_status_register->occupied_registers.front() != "w9") {
    return fail("expected old-value compare-exchange loop facts to survive selection");
  }
  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok) {
    return fail("expected prepared atomic route to print from selected loop facts: " +
                printed.diagnostic);
  }
  if (printed.assembly.find("add w9, w4, w2") == std::string::npos ||
      printed.assembly.find("stlxr w10, w9, [x0]") == std::string::npos ||
      printed.assembly.find("ldaxr w9, [x0]") == std::string::npos ||
      printed.assembly.find("stlxr w10, w2, [x0]") == std::string::npos ||
      printed.assembly.find("stxr w9, w2, [x0]") == std::string::npos) {
    return fail("expected real prepared atomic route to print allocated loop facts");
  }
  return 0;
}

int block_dispatch_rejects_incomplete_and_deferred_atomic_carriers() {
  auto prepared = prepared_with_atomic_memory_carriers(false);
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
    return fail("expected rejected atomic carriers to emit only the return node");
  }
  if (diagnostics.entries.size() != 6) {
    return fail("expected one diagnostic per rejected atomic carrier");
  }
  const std::array<std::string_view, 6> expected_errors = {
      "incomplete_prepared_atomic_operation",
      "unsupported_width",
      "unsupported_ordering",
      "unsupported_ordering",
      "unsupported_rmw_opcode",
      "unsupported_failure_ordering",
  };
  for (std::size_t i = 0; i < expected_errors.size(); ++i) {
    if (diagnostics.entries[i].kind !=
            aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
        diagnostics.entries[i].instruction_index != i ||
        diagnostics.entries[i].message.find(expected_errors[i]) == std::string::npos) {
      return fail("expected atomic carrier rejection to stay fail-closed and explicit");
    }
  }
  if (!std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected rejected atomic block to preserve ordinary return lowering");
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
      call->preserved_values[1].stack_offset_bytes != std::optional<std::size_t>{96} ||
      call->preserved_values[1].stack_size_bytes != std::optional<std::size_t>{16} ||
      call->preserved_values[1].stack_align_bytes != std::optional<std::size_t>{8}) {
    return fail("expected direct call node to retain stack-slot preserved-value provenance");
  }
  if (call_instruction.target.preserves.size() != 2 ||
      call_instruction.target.preserves[0].kind !=
          aarch64_module::codegen::MachineEffectResourceKind::Register ||
      call_instruction.target.preserves[0].reg != aarch64_module::abi::x_register(19) ||
      call_instruction.target.preserves[0].value_id != prepare::PreparedValueId{42} ||
      call_instruction.target.preserves[0].value_name != c4c::ValueNameId{17} ||
      !call_instruction.target.preserves[0].operand.has_value()) {
    return fail("expected direct call node to expose prepared preserved-value register effect");
  }
  const auto* preserved = std::get_if<aarch64_module::codegen::RegisterOperand>(
      &call_instruction.target.preserves[0].operand->payload);
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
  if (call_instruction.target.preserves[1].kind !=
          aarch64_module::codegen::MachineEffectResourceKind::Memory ||
      call_instruction.target.preserves[1].value_id != prepare::PreparedValueId{43} ||
      call_instruction.target.preserves[1].value_name != c4c::ValueNameId{18} ||
      call_instruction.target.preserves[1].frame_slot_id != prepare::PreparedFrameSlotId{11} ||
      !call_instruction.target.preserves[1].operand.has_value()) {
    return fail("expected direct call node to expose prepared stack-slot preserve effect");
  }
  const auto* preserved_memory = std::get_if<aarch64_module::codegen::MemoryOperand>(
      &call_instruction.target.preserves[1].operand->payload);
  if (preserved_memory == nullptr ||
      preserved_memory->base_kind != aarch64_module::codegen::MemoryBaseKind::FrameSlot ||
      preserved_memory->frame_slot_id != prepare::PreparedFrameSlotId{11} ||
      preserved_memory->byte_offset != 96 ||
      preserved_memory->size_bytes != 16 ||
      preserved_memory->align_bytes != 8) {
    return fail("expected stack-slot preserve effect to use prepared extent facts");
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

int block_dispatch_keeps_intrinsic_spelling_without_carrier_fail_closed() {
  auto prepared = prepared_with_direct_call_plan();
  auto& bir_call = std::get<bir::CallInst>(
      prepared.module.functions.front().blocks.front().insts.front());
  bir_call.callee = "llvm.x86.aesni.aesenc";
  bir_call.callee_link_name_id = c4c::kInvalidLinkName;
  prepared.call_plans.functions.front().calls.front().direct_callee_name =
      std::string{"llvm.x86.aesni.aesenc"};

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
    return fail("expected unsupported intrinsic spelling to remain an ordinary prepared call");
  }

  const auto& call_instruction = block.instructions.front();
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &call_instruction.target.payload);
  if (call == nullptr ||
      call_instruction.target.family != aarch64_module::codegen::InstructionFamily::Call ||
      call_instruction.target.opcode != aarch64_module::codegen::MachineOpcode::DirectCall ||
      call->direct_callee_label != "llvm.x86.aesni.aesenc" ||
      call->wrapper_kind != prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      call->source_call != &function_context.call_plans->calls.front()) {
    return fail("expected intrinsic spelling not to fabricate an AArch64 intrinsic record");
  }

  prepared.call_plans.functions.clear();
  const auto missing_function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto missing_block_context =
      aarch64_codegen::make_block_lowering_context(missing_function_context, block_cf, 0);
  aarch64_module::MachineBlock missing_block;
  aarch64_module::ModuleLoweringDiagnostics missing_diagnostics;
  const auto missing_result = aarch64_codegen::dispatch_prepared_block(
      missing_block_context, missing_block, missing_diagnostics);

  if (missing_result.visited_operations != 1 || !missing_result.visited_terminator ||
      missing_result.emitted_instructions != 1 || missing_block.instructions.size() != 1 ||
      missing_diagnostics.entries.size() != 1 ||
      missing_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan ||
      missing_diagnostics.entries.front().instruction_family !=
          aarch64_module::InstructionLoweringFamily::Call ||
      missing_diagnostics.entries.front().message.find("PreparedCallPlan") ==
          std::string::npos) {
    return fail("expected incomplete intrinsic call facts to stay fail-closed");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          missing_block.instructions.front().target.payload)) {
    return fail("expected incomplete intrinsic call facts to preserve return lowering only");
  }
  return 0;
}

bool block_emits_unresolved_dynamic_stack_helper_call(
    const aarch64_module::MachineBlock& block) {
  for (const auto& instruction : block.instructions) {
    const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
        &instruction.target.payload);
    if (call != nullptr &&
        (call->direct_callee_label == "llvm.stacksave" ||
         call->direct_callee_label == "llvm.dynamic_alloca.i8" ||
         call->direct_callee_label == "llvm.stackrestore")) {
      return true;
    }
  }
  return false;
}

void set_dynamic_stack_count_home_register(prepare::PreparedBirModule& prepared,
                                           std::string register_name,
                                           std::size_t physical_index) {
  auto& value_home = prepared.value_locations.functions.front().value_homes[1];
  value_home.register_name = std::move(register_name);
  value_home.target_register_identity =
      prepare::PreparedTargetRegisterIdentity{
          .target_arch = c4c::TargetArch::Aarch64,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_class = prepare::PreparedRegisterClass::General,
          .physical_index = physical_index,
      };
  auto& argument =
      prepared.call_plans.functions.front().calls[1].arguments.front();
  argument.source_register_name = value_home.register_name;
  argument.source_register_placement.reset();
}

int dynamic_stack_helper_calls_missing_prepared_authority_fail_closed() {
  auto prepared = prepared_with_dynamic_stack_helper_calls();
  prepared.dynamic_stack_plan.functions.clear();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  if (function_context.dynamic_stack_plan != nullptr) {
    return fail("expected unsupported dynamic-stack fixture to omit prepared authority");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 3 || !result.visited_terminator) {
    return fail("expected dynamic-stack helper fixture to visit all retained helper calls");
  }

  bool emitted_dynamic_stack_rejection = false;
  for (const auto& instruction : block.instructions) {
    if (instruction.target.selection.status ==
            aarch64_module::codegen::MachineNodeSelectionStatus::DeferredUnsupported &&
        instruction.target.selection.diagnostic.find(
            "requires prepared dynamic-stack operation authority") != std::string::npos) {
      emitted_dynamic_stack_rejection = true;
    }
  }

  if (diagnostics.empty()) {
    return fail("expected dynamic-stack helper rejection diagnostic during lowering");
  }
  if (block_emits_unresolved_dynamic_stack_helper_call(block)) {
    return fail(
        "expected dynamic-stack rejection to occur before unresolved LLVM helper "
        "calls reach machine output");
  }
  if (!emitted_dynamic_stack_rejection) {
    return fail(
        "expected dynamic-stack helper rejection to be represented as a fail-closed "
        "machine node, not only a side-channel diagnostic");
  }

  const auto printed = print_route_block(function_cf.function_name, block);
  if (printed.ok) {
    return fail(
        "expected fail-closed AArch64 dynamic-stack helper route to stop before "
        "assembly output");
  }
  if (printed.diagnostic.find("AArch64 dynamic-stack helper") == std::string::npos ||
      printed.diagnostic.find("deferred_unsupported") == std::string::npos) {
    return fail("expected printable failure to name the AArch64 dynamic-stack rejection: " +
                printed.diagnostic);
  }
  return 0;
}

int dynamic_stack_supported_helper_calls_lower_to_target_owned_output() {
  auto prepared = prepared_with_dynamic_stack_helper_calls();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  if (function_context.dynamic_stack_plan == nullptr ||
      function_context.dynamic_stack_plan->operations.size() != 3 ||
      !function_context.dynamic_stack_plan->requires_stack_save_restore ||
      function_context.value_locations == nullptr ||
      function_context.value_locations->value_homes.size() != 3) {
    return fail("expected fixture to carry supported prepared dynamic-stack authority");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);
  const auto printed = print_route_block(function_cf.function_name, block);

  if (result.visited_operations != 3 || !result.visited_terminator) {
    return fail("expected supported dynamic-stack fixture to visit all retained helpers");
  }
  if (!diagnostics.empty()) {
    return fail("expected supported prepared dynamic-stack helpers to lower without "
                "diagnostics; first diagnostic: " +
                diagnostics.entries.front().message +
                "; printer diagnostic: " + printed.diagnostic);
  }
  if (block_emits_unresolved_dynamic_stack_helper_call(block)) {
    return fail(
        "expected supported dynamic-stack lowering to avoid unresolved LLVM helper "
        "calls in machine output");
  }
  if (!printed.ok) {
    return fail("expected supported dynamic-stack lowering to print target-owned "
                "AArch64 output: " +
                printed.diagnostic);
  }
  if (printed.assembly.find("llvm.stacksave") != std::string::npos ||
      printed.assembly.find("llvm.dynamic_alloca") != std::string::npos ||
      printed.assembly.find("llvm.stackrestore") != std::string::npos) {
    return fail("expected supported dynamic-stack assembly to contain no unresolved "
                "LLVM helper references");
  }
  if (printed.assembly.find("sp") == std::string::npos ||
      printed.assembly.find("x20") == std::string::npos ||
      printed.assembly.find("x13") == std::string::npos) {
    return fail("expected supported dynamic-stack assembly to materialize saved stack "
                "pointer and allocation-result register behavior");
  }

  auto scratch_overlap_prepared = prepared_with_dynamic_stack_helper_calls();
  set_dynamic_stack_count_home_register(scratch_overlap_prepared, "x17", 17);
  const auto& overlap_function_cf =
      scratch_overlap_prepared.control_flow.functions.front();
  const auto& overlap_block_cf = overlap_function_cf.blocks.front();
  const auto overlap_function_context =
      aarch64_codegen::make_function_lowering_context(
          scratch_overlap_prepared,
          scratch_overlap_prepared.target_profile,
          overlap_function_cf);
  const auto overlap_block_context =
      aarch64_codegen::make_block_lowering_context(
          overlap_function_context, overlap_block_cf, 0);
  aarch64_module::MachineBlock overlap_block;
  aarch64_module::ModuleLoweringDiagnostics overlap_diagnostics;
  const auto overlap_result = aarch64_codegen::dispatch_prepared_block(
      overlap_block_context, overlap_block, overlap_diagnostics);
  const auto overlap_printed =
      print_route_block(overlap_function_cf.function_name, overlap_block);
  if (overlap_result.visited_operations != 3 || !overlap_result.visited_terminator ||
      !overlap_diagnostics.empty() || !overlap_printed.ok) {
    return fail("expected x17 dynamic-stack count home to lower without diagnostics: " +
                overlap_printed.diagnostic);
  }
  const auto preserved_count = overlap_printed.assembly.find("mov x16, x17");
  const auto reused_scratch = overlap_printed.assembly.find("mov x17, sp");
  if (preserved_count == std::string::npos || reused_scratch == std::string::npos ||
      preserved_count > reused_scratch ||
      overlap_printed.assembly.find("sub x17, x17, x16") == std::string::npos) {
    return fail("expected dynamic alloca count in x17 to be preserved before x17 "
                "is reused for stack-pointer computation");
  }

  auto unsafe_stack_home_prepared = prepared_with_dynamic_stack_helper_calls();
  auto& unsafe_count_home =
      unsafe_stack_home_prepared.value_locations.functions.front().value_homes[1];
  unsafe_count_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  unsafe_count_home.register_name.reset();
  unsafe_count_home.target_register_identity.reset();
  unsafe_count_home.slot_id = prepare::PreparedFrameSlotId{5};
  unsafe_count_home.offset_bytes = std::size_t{16};
  unsafe_count_home.size_bytes = std::size_t{8};
  unsafe_count_home.align_bytes = std::size_t{8};
  unsafe_stack_home_prepared.frame_plan.functions.front()
      .uses_frame_pointer_for_fixed_slots = false;
  const auto& unsafe_function_cf =
      unsafe_stack_home_prepared.control_flow.functions.front();
  const auto& unsafe_block_cf = unsafe_function_cf.blocks.front();
  const auto unsafe_function_context =
      aarch64_codegen::make_function_lowering_context(
          unsafe_stack_home_prepared,
          unsafe_stack_home_prepared.target_profile,
          unsafe_function_cf);
  const auto unsafe_block_context =
      aarch64_codegen::make_block_lowering_context(
          unsafe_function_context, unsafe_block_cf, 0);
  aarch64_module::MachineBlock unsafe_block;
  aarch64_module::ModuleLoweringDiagnostics unsafe_diagnostics;
  const auto unsafe_result = aarch64_codegen::dispatch_prepared_block(
      unsafe_block_context, unsafe_block, unsafe_diagnostics);
  if (unsafe_result.visited_operations != 3 || !unsafe_result.visited_terminator ||
      unsafe_diagnostics.empty() ||
      unsafe_diagnostics.entries.front().message.find("stable frame-pointer base") ==
          std::string::npos ||
      unsafe_block.instructions[1].target.selection.diagnostic.find(
          "stable frame-pointer base") == std::string::npos) {
    return fail("expected dynamic stack-slot count home without stable frame-pointer "
                "base to fail closed before invalid assembly");
  }

  return 0;
}

prepare::PreparedBirModule prepared_with_scalar_fp_unary_fabs_intrinsic(
    bir::TypeKind type = bir::TypeKind::F64,
    prepare::PreparedIntrinsicCarrierKind carrier_kind =
        prepare::PreparedIntrinsicCarrierKind::Complete,
    bool use_fpr_storage = true,
    bool has_prepared_call_plan = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.fabs");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.fabs.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.fabs.entry");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto result_name = prepared.names.value_names.intern("%fabs");
  const auto fabs_link =
      prepared.names.link_names.intern(type == bir::TypeKind::F32 ? "llvm.fabs.float"
                                                                  : "llvm.fabs.double");
  const char* source_register = type == bir::TypeKind::F32 ? "s1" : "d1";
  const char* result_register = type == bir::TypeKind::F32 ? "s0" : "d0";

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.fabs",
      .return_type = bir::TypeKind::Void,
      .blocks =
          {bir::Block{
              .label = "dispatch.fabs.entry",
              .insts =
                  {bir::CallInst{
                      .result = bir::Value::named(type, "%fabs"),
                      .callee = type == bir::TypeKind::F32 ? "llvm.fabs.float"
                                                           : "llvm.fabs.double",
                      .callee_link_name_id = fabs_link,
                      .args = {bir::Value::named(type, "%src")},
                      .arg_types = {type},
                      .return_type = type,
                      .intrinsic =
                          bir::IntrinsicOperation{
                              .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
                              .operation = bir::IntrinsicOperationKind::FAbs,
                              .operand_type = type,
                              .result_type = type,
                              .has_side_effects = false,
                          },
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
  if (has_prepared_call_plan) {
    prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
        .function_name = function_name,
        .calls = {prepare::PreparedCallPlan{
            .block_index = 0,
            .instruction_index = 0,
            .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
            .direct_callee_name = type == bir::TypeKind::F32 ? std::string{"llvm.fabs.float"}
                                                             : std::string{"llvm.fabs.double"},
            .arguments = {prepare::PreparedCallArgumentPlan{
                .arg_index = 0,
                .value_bank = prepare::PreparedRegisterBank::Fpr,
                .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                .source_value_id = prepare::PreparedValueId{50},
                .source_register_name = source_register,
                .source_register_bank = prepare::PreparedRegisterBank::Fpr,
                .destination_register_name = source_register,
                .destination_register_bank = prepare::PreparedRegisterBank::Fpr,
            }},
            .result = prepare::PreparedCallResultPlan{
                .value_bank = prepare::PreparedRegisterBank::Fpr,
                .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
                .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                .destination_value_id = prepare::PreparedValueId{51},
                .source_register_name = result_register,
                .source_register_bank = prepare::PreparedRegisterBank::Fpr,
                .destination_register_name = result_register,
                .destination_register_bank = prepare::PreparedRegisterBank::Fpr,
            },
        }},
    });
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{50},
               .function_name = function_name,
               .value_name = source_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = source_register,
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{51},
               .function_name = function_name,
               .value_name = result_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = result_register,
           }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          use_fpr_storage
              ? std::vector<prepare::PreparedStoragePlanValue>{
                    fpr_storage(prepare::PreparedValueId{50}, source_name, source_register),
                    fpr_storage(prepare::PreparedValueId{51}, result_name, result_register)}
              : std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{50}, source_name, source_register),
                    register_storage(prepare::PreparedValueId{51}, result_name, result_register)},
  });
  prepared.intrinsic_carriers.functions.push_back(prepare::PreparedIntrinsicCarrierFunction{
      .function_name = function_name,
      .carriers = {prepare::PreparedIntrinsicCarrier{
          .function_name = function_name,
          .carrier_kind = carrier_kind,
          .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
          .operation = bir::IntrinsicOperationKind::FAbs,
          .block_index = 0,
          .inst_index = 0,
          .operand_type = type,
          .result_type = type,
          .operand = bir::Value::named(type, "%src"),
          .result = bir::Value::named(type, "%fabs"),
          .operand_value_name = source_name,
          .result_value_name = result_name,
          .has_side_effects = false,
          .requires_feature = false,
          .source_callee_name = type == bir::TypeKind::F32
                                    ? std::optional<std::string>{"llvm.fabs.float"}
                                    : std::optional<std::string>{"llvm.fabs.double"},
          .has_prepared_call_plan = has_prepared_call_plan,
          .missing_required_facts =
              carrier_kind == prepare::PreparedIntrinsicCarrierKind::Complete
                  ? std::vector<std::string>{}
                  : std::vector<std::string>{"missing_prepared_call_plan"},
      }},
  });
  return prepared;
}

prepare::PreparedStoragePlanValue vector_storage(prepare::PreparedValueId value_id,
                                                 c4c::ValueNameId value_name,
                                                 const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Vreg,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedBirModule prepared_with_complete_aarch64_crc32w_intrinsic_carrier() {
  auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(bir::TypeKind::F64);
  const auto function_name = prepared.control_flow.functions.front().function_name;
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions.front().blocks.front().insts.front());
  call.result = bir::Value::named(bir::TypeKind::I32, "%crc");
  call.callee = "llvm.aarch64.crc32w";
  call.callee_link_name_id = prepared.names.link_names.intern("llvm.aarch64.crc32w");
  call.args = {bir::Value::named(bir::TypeKind::I32, "%acc"),
               bir::Value::named(bir::TypeKind::I32, "%data")};
  call.arg_types = {bir::TypeKind::I32, bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  call.intrinsic = bir::IntrinsicOperation{
      .family = bir::IntrinsicFamilyKind::Crc,
      .operation = bir::IntrinsicOperationKind::Crc32W,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Crc,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::I32,
      .operand_roles = {bir::IntrinsicOperandRole::Accumulator,
                        bir::IntrinsicOperandRole::Data},
      .signedness = bir::IntrinsicSignedness::Unsigned,
  };

  const auto acc_name = prepared.names.value_names.intern("%acc");
  const auto data_name = prepared.names.value_names.intern("%data");
  const auto result_name = prepared.names.value_names.intern("%crc");
  auto& homes = prepared.value_locations.functions.front().value_homes;
  homes = {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{60},
               .function_name = function_name,
               .value_name = acc_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "w0",
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{61},
               .function_name = function_name,
               .value_name = data_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "w1",
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{62},
               .function_name = function_name,
               .value_name = result_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = "w0",
           }};
  prepared.storage_plans.functions.front().values = {
      register_storage(prepare::PreparedValueId{60}, acc_name, "w0"),
      register_storage(prepare::PreparedValueId{61}, data_name, "w1"),
      register_storage(prepare::PreparedValueId{62}, result_name, "w0")};
  auto& plan = prepared.call_plans.functions.front().calls.front();
  plan.direct_callee_name = std::string{"llvm.aarch64.crc32w"};
  plan.arguments.clear();
  plan.result = prepare::PreparedCallResultPlan{};

  auto& carrier = prepared.intrinsic_carriers.functions.front().carriers.front();
  carrier = prepare::PreparedIntrinsicCarrier{
      .function_name = function_name,
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::Crc,
      .operation = bir::IntrinsicOperationKind::Crc32W,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Crc,
      .block_index = 0,
      .inst_index = 0,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::I32,
      .operand_roles = {bir::IntrinsicOperandRole::Accumulator,
                        bir::IntrinsicOperandRole::Data},
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .operands = {bir::Value::named(bir::TypeKind::I32, "%acc"),
                   bir::Value::named(bir::TypeKind::I32, "%data")},
      .result = bir::Value::named(bir::TypeKind::I32, "%crc"),
      .operand_value_names = {acc_name, data_name},
      .result_value_name = result_name,
      .operand_homes = {homes[0], homes[1]},
      .result_home = homes[2],
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.crc32w"},
      .has_prepared_call_plan = true,
  };
  return prepared;
}

prepare::PreparedBirModule prepared_with_complete_aarch64_vector_intrinsic_carrier(
    bir::IntrinsicOperationKind operation) {
  auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(bir::TypeKind::F64);
  const auto function_name = prepared.control_flow.functions.front().function_name;
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions.front().blocks.front().insts.front());
  const bool is_load = operation == bir::IntrinsicOperationKind::VectorLoad;
  const auto ptr_name = prepared.names.value_names.intern("%p");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto result_name =
      prepared.names.value_names.intern(is_load ? "%vector" : "%sum");
  const auto operand_roles =
      is_load ? std::vector<bir::IntrinsicOperandRole>{bir::IntrinsicOperandRole::Pointer}
              : std::vector<bir::IntrinsicOperandRole>{
                    bir::IntrinsicOperandRole::VectorLhs,
                    bir::IntrinsicOperandRole::VectorRhs};

  call.result = bir::Value::named(bir::TypeKind::I128, is_load ? "%vector" : "%sum");
  call.callee = is_load ? "llvm.aarch64.neon.ld1.v16i8.p0i8"
                        : "llvm.aarch64.neon.add.v16i8";
  call.callee_link_name_id = prepared.names.link_names.intern(call.callee);
  call.args = is_load ? std::vector<bir::Value>{bir::Value::named(bir::TypeKind::Ptr, "%p")}
                      : std::vector<bir::Value>{bir::Value::named(bir::TypeKind::I128, "%lhs"),
                                                bir::Value::named(bir::TypeKind::I128, "%rhs")};
  call.arg_types = is_load ? std::vector<bir::TypeKind>{bir::TypeKind::Ptr}
                           : std::vector<bir::TypeKind>{bir::TypeKind::I128,
                                                        bir::TypeKind::I128};
  call.return_type = bir::TypeKind::I128;
  call.intrinsic = bir::IntrinsicOperation{
      .family = is_load ? bir::IntrinsicFamilyKind::VectorMemory
                        : bir::IntrinsicFamilyKind::VectorOperation,
      .operation = operation,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
      .operand_type = is_load ? bir::TypeKind::Ptr : bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .operand_roles = operand_roles,
      .vector_element_type = bir::TypeKind::I8,
      .vector_element_width_bytes = 1,
      .vector_lane_count = 16,
      .vector_total_width_bytes = 16,
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .memory_access = is_load ? bir::IntrinsicMemoryAccessKind::Read
                               : bir::IntrinsicMemoryAccessKind::None,
  };

  auto& homes = prepared.value_locations.functions.front().value_homes;
  homes = is_load ? std::vector<prepare::PreparedValueHome>{
                        prepare::PreparedValueHome{
                            .value_id = prepare::PreparedValueId{70},
                            .function_name = function_name,
                            .value_name = ptr_name,
                            .kind = prepare::PreparedValueHomeKind::Register,
                            .register_name = "x0",
                        },
                        prepare::PreparedValueHome{
                            .value_id = prepare::PreparedValueId{71},
                            .function_name = function_name,
                            .value_name = result_name,
                            .kind = prepare::PreparedValueHomeKind::Register,
                            .register_name = "q0",
                        }}
                  : std::vector<prepare::PreparedValueHome>{
                        prepare::PreparedValueHome{
                            .value_id = prepare::PreparedValueId{72},
                            .function_name = function_name,
                            .value_name = lhs_name,
                            .kind = prepare::PreparedValueHomeKind::Register,
                            .register_name = "q1",
                        },
                        prepare::PreparedValueHome{
                            .value_id = prepare::PreparedValueId{73},
                            .function_name = function_name,
                            .value_name = rhs_name,
                            .kind = prepare::PreparedValueHomeKind::Register,
                            .register_name = "q2",
                        },
                        prepare::PreparedValueHome{
                            .value_id = prepare::PreparedValueId{74},
                            .function_name = function_name,
                            .value_name = result_name,
                            .kind = prepare::PreparedValueHomeKind::Register,
                            .register_name = "q0",
                        }};
  prepared.storage_plans.functions.front().values =
      is_load ? std::vector<prepare::PreparedStoragePlanValue>{
                    register_storage(prepare::PreparedValueId{70}, ptr_name, "x0"),
                    vector_storage(prepare::PreparedValueId{71}, result_name, "q0")}
              : std::vector<prepare::PreparedStoragePlanValue>{
                    vector_storage(prepare::PreparedValueId{72}, lhs_name, "q1"),
                    vector_storage(prepare::PreparedValueId{73}, rhs_name, "q2"),
                    vector_storage(prepare::PreparedValueId{74}, result_name, "q0")};
  auto& plan = prepared.call_plans.functions.front().calls.front();
  plan.direct_callee_name = call.callee;
  plan.arguments.clear();
  plan.result = prepare::PreparedCallResultPlan{};

  auto& carrier = prepared.intrinsic_carriers.functions.front().carriers.front();
  carrier = prepare::PreparedIntrinsicCarrier{
      .function_name = function_name,
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = is_load ? bir::IntrinsicFamilyKind::VectorMemory
                        : bir::IntrinsicFamilyKind::VectorOperation,
      .operation = operation,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
      .block_index = 0,
      .inst_index = 0,
      .operand_type = is_load ? bir::TypeKind::Ptr : bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .operand_roles = operand_roles,
      .vector_element_type = bir::TypeKind::I8,
      .vector_element_width_bytes = 1,
      .vector_lane_count = 16,
      .vector_total_width_bytes = 16,
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .memory_access = is_load ? bir::IntrinsicMemoryAccessKind::Read
                               : bir::IntrinsicMemoryAccessKind::None,
      .operands = call.args,
      .result = call.result,
      .operand_value_names = is_load ? std::vector<c4c::ValueNameId>{ptr_name}
                                     : std::vector<c4c::ValueNameId>{lhs_name, rhs_name},
      .result_value_name = result_name,
      .operand_homes = is_load ? std::vector<std::optional<prepare::PreparedValueHome>>{homes[0]}
                               : std::vector<std::optional<prepare::PreparedValueHome>>{
                                     homes[0], homes[1]},
      .result_home = is_load ? homes[1] : homes[2],
      .requires_feature = true,
      .source_callee_name = call.callee,
      .has_prepared_call_plan = true,
  };
  return prepared;
}

prepare::PreparedBirModule prepared_with_complete_aarch64_barrier_dmb_intrinsic_carrier() {
  auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(bir::TypeKind::F64);
  const auto function_name = prepared.control_flow.functions.front().function_name;
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions.front().blocks.front().insts.front());
  call.result = std::nullopt;
  call.callee = "llvm.aarch64.dmb";
  call.callee_link_name_id = prepared.names.link_names.intern("llvm.aarch64.dmb");
  call.args = {bir::Value::immediate_i32(15)};
  call.arg_types = {bir::TypeKind::I32};
  call.return_type = bir::TypeKind::Void;
  call.intrinsic = bir::IntrinsicOperation{
      .family = bir::IntrinsicFamilyKind::Barrier,
      .operation = bir::IntrinsicOperationKind::BarrierDmb,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::Void,
      .operand_roles = {bir::IntrinsicOperandRole::BarrierDomain},
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .barrier_domain = bir::IntrinsicBarrierDomainKind::Sy,
      .has_immediate_operand = true,
      .requires_immediate_operand = true,
      .immediate_value = 15,
      .has_side_effects = true,
  };

  prepared.value_locations.functions.front().value_homes.clear();
  prepared.storage_plans.functions.front().values.clear();
  auto& plan = prepared.call_plans.functions.front().calls.front();
  plan.direct_callee_name = std::string{"llvm.aarch64.dmb"};
  plan.arguments.clear();
  plan.result = std::nullopt;

  auto& carrier = prepared.intrinsic_carriers.functions.front().carriers.front();
  carrier = prepare::PreparedIntrinsicCarrier{
      .function_name = function_name,
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::Barrier,
      .operation = bir::IntrinsicOperationKind::BarrierDmb,
      .block_index = 0,
      .inst_index = 0,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::Void,
      .operand_roles = {bir::IntrinsicOperandRole::BarrierDomain},
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .barrier_domain = bir::IntrinsicBarrierDomainKind::Sy,
      .has_immediate_operand = true,
      .requires_immediate_operand = true,
      .immediate_value = 15,
      .operands = call.args,
      .operand_homes = {std::nullopt},
      .has_side_effects = true,
      .source_callee_name = std::string{"llvm.aarch64.dmb"},
      .has_prepared_call_plan = true,
  };
  return prepared;
}

prepare::PreparedBirModule prepared_with_complete_aarch64_cache_dc_cvau_intrinsic_carrier() {
  auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(bir::TypeKind::F64);
  const auto function_name = prepared.control_flow.functions.front().function_name;
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions.front().blocks.front().insts.front());
  const auto pointer_name = prepared.names.value_names.intern("%p");
  call.result = std::nullopt;
  call.callee = "llvm.aarch64.dc.cvau";
  call.callee_link_name_id = prepared.names.link_names.intern("llvm.aarch64.dc.cvau");
  call.args = {bir::Value::named(bir::TypeKind::Ptr, "%p")};
  call.arg_types = {bir::TypeKind::Ptr};
  call.return_type = bir::TypeKind::Void;
  call.intrinsic = bir::IntrinsicOperation{
      .family = bir::IntrinsicFamilyKind::CacheMaintenance,
      .operation = bir::IntrinsicOperationKind::CacheDcCvau,
      .operand_type = bir::TypeKind::Ptr,
      .result_type = bir::TypeKind::Void,
      .operand_roles = {bir::IntrinsicOperandRole::CacheAddress},
      .memory_operand = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p"),
          .size_bytes = 0,
          .align_bytes = 1,
          .address_space = bir::AddressSpace::Default,
      },
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .has_side_effects = true,
  };

  prepared.value_locations.functions.front().value_homes = {
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{80},
          .function_name = function_name,
          .value_name = pointer_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = "x0",
      },
  };
  prepared.storage_plans.functions.front().values = {
      register_storage(prepare::PreparedValueId{80}, pointer_name, "x0"),
  };
  auto& plan = prepared.call_plans.functions.front().calls.front();
  plan.direct_callee_name = std::string{"llvm.aarch64.dc.cvau"};
  plan.arguments = {prepare::PreparedCallArgumentPlan{}};
  plan.result = std::nullopt;

  auto& carrier = prepared.intrinsic_carriers.functions.front().carriers.front();
  carrier = prepare::PreparedIntrinsicCarrier{
      .function_name = function_name,
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::CacheMaintenance,
      .operation = bir::IntrinsicOperationKind::CacheDcCvau,
      .block_index = 0,
      .inst_index = 0,
      .operand_type = bir::TypeKind::Ptr,
      .result_type = bir::TypeKind::Void,
      .operand_roles = {bir::IntrinsicOperandRole::CacheAddress},
      .memory_operand = call.intrinsic->memory_operand,
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .operands = call.args,
      .operand_value_names = {pointer_name},
      .operand_homes = {prepared.value_locations.functions.front().value_homes.front()},
      .has_side_effects = true,
      .source_callee_name = std::string{"llvm.aarch64.dc.cvau"},
      .has_prepared_call_plan = true,
  };
  return prepared;
}

prepare::PreparedBirModule prepared_with_complete_aarch64_hint_yield_intrinsic_carrier() {
  auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(bir::TypeKind::F64);
  const auto function_name = prepared.control_flow.functions.front().function_name;
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions.front().blocks.front().insts.front());
  call.result = std::nullopt;
  call.callee = "llvm.aarch64.hint";
  call.callee_link_name_id = prepared.names.link_names.intern("llvm.aarch64.hint");
  call.args = {bir::Value::immediate_i32(1)};
  call.arg_types = {bir::TypeKind::I32};
  call.return_type = bir::TypeKind::Void;
  call.intrinsic = bir::IntrinsicOperation{
      .family = bir::IntrinsicFamilyKind::PauseHint,
      .operation = bir::IntrinsicOperationKind::HintYield,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::Void,
      .operand_roles = {bir::IntrinsicOperandRole::HintImmediate},
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .has_immediate_operand = true,
      .requires_immediate_operand = true,
      .immediate_value = 1,
      .has_side_effects = true,
  };

  prepared.value_locations.functions.front().value_homes.clear();
  prepared.storage_plans.functions.front().values.clear();
  auto& plan = prepared.call_plans.functions.front().calls.front();
  plan.direct_callee_name = std::string{"llvm.aarch64.hint"};
  plan.arguments.clear();
  plan.result = std::nullopt;

  auto& carrier = prepared.intrinsic_carriers.functions.front().carriers.front();
  carrier = prepare::PreparedIntrinsicCarrier{
      .function_name = function_name,
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::PauseHint,
      .operation = bir::IntrinsicOperationKind::HintYield,
      .block_index = 0,
      .inst_index = 0,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::Void,
      .operand_roles = {bir::IntrinsicOperandRole::HintImmediate},
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .has_immediate_operand = true,
      .requires_immediate_operand = true,
      .immediate_value = 1,
      .operands = call.args,
      .operand_homes = {std::nullopt},
      .has_side_effects = true,
      .source_callee_name = std::string{"llvm.aarch64.hint"},
      .has_prepared_call_plan = true,
  };
  return prepared;
}

int block_dispatch_selects_complete_scalar_fp_unary_fabs_intrinsic_carrier() {
  for (const auto type : {bir::TypeKind::F32, bir::TypeKind::F64}) {
    auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(type);
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

    if (!diagnostics.empty() || result.visited_operations != 1 ||
        result.emitted_instructions != 2 || block.instructions.size() != 2) {
      return fail("expected complete fabs carrier dispatch to select intrinsic plus return");
    }
    const auto& instruction = block.instructions.front();
    const auto* intrinsic =
        std::get_if<aarch64_codegen::ScalarFpUnaryIntrinsicRecord>(
            &instruction.target.payload);
    if (intrinsic == nullptr ||
        instruction.target.family != aarch64_codegen::InstructionFamily::Intrinsic ||
        instruction.target.opcode !=
            aarch64_codegen::MachineOpcode::ScalarFpUnaryIntrinsic ||
        instruction.target.selection.status !=
            aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        intrinsic->source_carrier != &function_context.prepared->intrinsic_carriers.functions
                                          .front()
                                          .carriers.front() ||
        intrinsic->family != bir::IntrinsicFamilyKind::ScalarFpUnary ||
        intrinsic->operation != bir::IntrinsicOperationKind::FAbs ||
        intrinsic->operand_type != type || intrinsic->result_type != type ||
        intrinsic->has_side_effects || intrinsic->requires_feature ||
        !intrinsic->has_prepared_call_plan ||
        intrinsic->source_callee_name !=
            std::optional<std::string>{type == bir::TypeKind::F32 ? "llvm.fabs.float"
                                                                  : "llvm.fabs.double"} ||
        !intrinsic->result_register.has_value()) {
      return fail("expected fabs intrinsic node to preserve structured carrier facts");
    }
    const auto* operand =
        std::get_if<aarch64_codegen::RegisterOperand>(&intrinsic->operand.payload);
    const auto expected_result =
        type == bir::TypeKind::F32 ? aarch64_abi::s_register(0)
                                   : aarch64_abi::d_register(0);
    const auto expected_operand =
        type == bir::TypeKind::F32 ? aarch64_abi::s_register(1)
                                   : aarch64_abi::d_register(1);
    if (operand == nullptr || operand->reg != expected_operand ||
        intrinsic->result_register->reg != expected_result ||
        instruction.target.defs.size() != 1 ||
        instruction.target.defs.front().reg != expected_result ||
        instruction.target.uses.size() != 1 ||
        instruction.target.uses.front().reg != expected_operand ||
        !instruction.target.side_effects.empty()) {
      return fail("expected fabs intrinsic node to preserve operand/result FPR authority");
    }
  }
  return 0;
}

int block_dispatch_selects_complete_crc_vector_intrinsic_carriers() {
  {
    auto prepared = prepared_with_complete_aarch64_crc32w_intrinsic_carrier();
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

    if (!diagnostics.empty() || result.visited_operations != 1 ||
        result.emitted_instructions != 2 || block.instructions.size() != 2) {
      return fail("expected complete CRC32W carrier dispatch to select intrinsic plus return");
    }
    const auto& instruction = block.instructions.front();
    const auto* intrinsic =
        std::get_if<aarch64_codegen::Crc32WIntrinsicRecord>(
            &instruction.target.payload);
    if (intrinsic == nullptr ||
        instruction.target.family != aarch64_codegen::InstructionFamily::Intrinsic ||
        instruction.target.opcode != aarch64_codegen::MachineOpcode::Crc32WIntrinsic ||
        instruction.target.selection.status !=
            aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        intrinsic->source_carrier != &function_context.prepared->intrinsic_carriers.functions
                                          .front()
                                          .carriers.front() ||
        intrinsic->family != bir::IntrinsicFamilyKind::Crc ||
        intrinsic->operation != bir::IntrinsicOperationKind::Crc32W ||
        intrinsic->required_feature != bir::IntrinsicFeatureKind::AArch64Crc ||
        intrinsic->operand_type != bir::TypeKind::I32 ||
        intrinsic->result_type != bir::TypeKind::I32 ||
        intrinsic->signedness != bir::IntrinsicSignedness::Unsigned ||
        !intrinsic->requires_feature || !intrinsic->has_prepared_call_plan ||
        !intrinsic->result_register.has_value()) {
      return fail("expected selected CRC32W intrinsic to preserve carrier facts");
    }
    const auto* accumulator =
        std::get_if<aarch64_codegen::RegisterOperand>(&intrinsic->accumulator.payload);
    const auto* data =
        std::get_if<aarch64_codegen::RegisterOperand>(&intrinsic->data.payload);
    if (accumulator == nullptr || data == nullptr ||
        accumulator->reg != aarch64_abi::w_register(0) ||
        data->reg != aarch64_abi::w_register(1) ||
        intrinsic->result_register->reg != aarch64_abi::w_register(0) ||
        instruction.target.defs.size() != 1 ||
        instruction.target.defs.front().reg != aarch64_abi::w_register(0) ||
        instruction.target.uses.size() != 2 ||
        instruction.target.uses[0].reg != aarch64_abi::w_register(0) ||
        instruction.target.uses[1].reg != aarch64_abi::w_register(1) ||
        !instruction.target.side_effects.empty()) {
      return fail("expected selected CRC32W intrinsic to preserve register authority");
    }
  }

  {
    auto prepared = prepared_with_complete_aarch64_vector_intrinsic_carrier(
        bir::IntrinsicOperationKind::VectorLoad);
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

    if (!diagnostics.empty() || result.visited_operations != 1 ||
        result.emitted_instructions != 2 || block.instructions.size() != 2) {
      return fail("expected complete vector-load carrier dispatch to select intrinsic plus return");
    }
    const auto& instruction = block.instructions.front();
    const auto* intrinsic =
        std::get_if<aarch64_codegen::VectorLoadIntrinsicRecord>(
            &instruction.target.payload);
    if (intrinsic == nullptr ||
        instruction.target.opcode != aarch64_codegen::MachineOpcode::VectorLoadIntrinsic ||
        instruction.target.selection.status !=
            aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        intrinsic->source_carrier != &function_context.prepared->intrinsic_carriers.functions
                                          .front()
                                          .carriers.front() ||
        intrinsic->family != bir::IntrinsicFamilyKind::VectorMemory ||
        intrinsic->operation != bir::IntrinsicOperationKind::VectorLoad ||
        intrinsic->required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
        intrinsic->vector_element_type != bir::TypeKind::I8 ||
        intrinsic->vector_element_width_bytes != 1 ||
        intrinsic->vector_lane_count != 16 ||
        intrinsic->vector_total_width_bytes != 16 ||
        intrinsic->memory_access != bir::IntrinsicMemoryAccessKind::Read ||
        !intrinsic->requires_feature || !intrinsic->has_prepared_call_plan ||
        !intrinsic->result_register.has_value()) {
      return fail("expected selected vector-load intrinsic to preserve carrier facts");
    }
    const auto* pointer =
        std::get_if<aarch64_codegen::RegisterOperand>(&intrinsic->pointer.payload);
    if (pointer == nullptr || pointer->reg != aarch64_abi::x_register(0) ||
        intrinsic->memory.base_kind != aarch64_codegen::MemoryBaseKind::PointerValue ||
        !intrinsic->memory.base_register.has_value() ||
        intrinsic->memory.base_register->reg != aarch64_abi::x_register(0) ||
        intrinsic->memory.size_bytes != 16 ||
        intrinsic->result_register->reg != aarch64_abi::q_register(0) ||
        instruction.target.defs.size() != 1 ||
        instruction.target.defs.front().reg != aarch64_abi::q_register(0) ||
        instruction.target.uses.size() != 2 ||
        instruction.target.uses.front().reg != aarch64_abi::x_register(0) ||
        instruction.target.side_effects.size() != 1 ||
        instruction.target.side_effects.front() !=
            aarch64_codegen::MachineSideEffectKind::MemoryRead) {
      return fail("expected selected vector-load intrinsic to preserve register and memory authority");
    }
  }

  {
    auto prepared = prepared_with_complete_aarch64_vector_intrinsic_carrier(
        bir::IntrinsicOperationKind::VectorAdd);
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

    if (!diagnostics.empty() || result.visited_operations != 1 ||
        result.emitted_instructions != 2 || block.instructions.size() != 2) {
      return fail("expected complete vector-add carrier dispatch to select intrinsic plus return");
    }
    const auto& instruction = block.instructions.front();
    const auto* intrinsic =
        std::get_if<aarch64_codegen::VectorAddIntrinsicRecord>(
            &instruction.target.payload);
    if (intrinsic == nullptr ||
        instruction.target.opcode != aarch64_codegen::MachineOpcode::VectorAddIntrinsic ||
        instruction.target.selection.status !=
            aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        intrinsic->source_carrier != &function_context.prepared->intrinsic_carriers.functions
                                          .front()
                                          .carriers.front() ||
        intrinsic->family != bir::IntrinsicFamilyKind::VectorOperation ||
        intrinsic->operation != bir::IntrinsicOperationKind::VectorAdd ||
        intrinsic->required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
        intrinsic->vector_element_type != bir::TypeKind::I8 ||
        intrinsic->vector_lane_count != 16 ||
        intrinsic->memory_access != bir::IntrinsicMemoryAccessKind::None ||
        !intrinsic->requires_feature || !intrinsic->has_prepared_call_plan ||
        !intrinsic->result_register.has_value()) {
      return fail("expected selected vector-add intrinsic to preserve carrier facts");
    }
    const auto* lhs =
        std::get_if<aarch64_codegen::RegisterOperand>(&intrinsic->lhs.payload);
    const auto* rhs =
        std::get_if<aarch64_codegen::RegisterOperand>(&intrinsic->rhs.payload);
    if (lhs == nullptr || rhs == nullptr ||
        lhs->reg != aarch64_abi::q_register(1) ||
        rhs->reg != aarch64_abi::q_register(2) ||
        intrinsic->result_register->reg != aarch64_abi::q_register(0) ||
        instruction.target.defs.size() != 1 ||
        instruction.target.defs.front().reg != aarch64_abi::q_register(0) ||
        instruction.target.uses.size() != 2 ||
        instruction.target.uses[0].reg != aarch64_abi::q_register(1) ||
        instruction.target.uses[1].reg != aarch64_abi::q_register(2) ||
        !instruction.target.side_effects.empty()) {
      return fail("expected selected vector-add intrinsic to preserve register authority");
    }
  }
  return 0;
}

int block_dispatch_keeps_complete_barrier_dmb_carrier_fail_closed() {
  auto prepared = prepared_with_complete_aarch64_barrier_dmb_intrinsic_carrier();
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

  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().message.find("unsupported_intrinsic_family") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected complete barrier DMB carrier to remain non-selected");
  }
  if (std::holds_alternative<aarch64_codegen::ScalarFpUnaryIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::Crc32WIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::VectorLoadIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::VectorAddIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected barrier DMB carrier not to select intrinsic or call");
  }
  return 0;
}

int block_dispatch_keeps_complete_cache_dc_cvau_carrier_fail_closed() {
  auto prepared = prepared_with_complete_aarch64_cache_dc_cvau_intrinsic_carrier();
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

  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().message.find("unsupported_intrinsic_family") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected complete cache DC CVAU carrier to remain non-selected");
  }
  if (std::holds_alternative<aarch64_codegen::ScalarFpUnaryIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::Crc32WIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::VectorLoadIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::VectorAddIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected cache DC CVAU carrier not to select intrinsic or call");
  }
  return 0;
}

int block_dispatch_keeps_complete_hint_yield_carrier_fail_closed() {
  auto prepared = prepared_with_complete_aarch64_hint_yield_intrinsic_carrier();
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

  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().message.find("unsupported_intrinsic_family") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected complete hint yield carrier to remain non-selected");
  }
  if (std::holds_alternative<aarch64_codegen::ScalarFpUnaryIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::Crc32WIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::VectorLoadIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::VectorAddIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected hint yield carrier not to select intrinsic or call");
  }
  return 0;
}

int block_dispatch_keeps_incomplete_scalar_fp_unary_intrinsic_fail_closed() {
  {
    auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(
        bir::TypeKind::F64,
        prepare::PreparedIntrinsicCarrierKind::Missing);
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
    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
        diagnostics.entries.front().message.find("incomplete_prepared_intrinsic_carrier") ==
            std::string::npos ||
        !std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
            block.instructions.front().target.payload)) {
      return fail("expected incomplete fabs carrier to fail closed without ordinary call");
    }
  }
  {
    auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(
        bir::TypeKind::F64,
        prepare::PreparedIntrinsicCarrierKind::Complete,
        false);
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
    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.empty() ||
        !std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
            block.instructions.front().target.payload)) {
      return fail("expected fabs carrier without FPR authority to fail closed");
    }
  }
  return 0;
}

int block_dispatch_keeps_f128_fabs_intrinsic_carrier_fail_closed() {
  auto prepared = prepared_with_scalar_fp_unary_fabs_intrinsic(
      bir::TypeKind::F64,
      prepare::PreparedIntrinsicCarrierKind::Complete);
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions.front().blocks.front().insts.front());
  call.result = bir::Value::named(bir::TypeKind::F128, "%fabs");
  call.callee = "llvm.fabs.f128";
  call.callee_link_name_id = prepared.names.link_names.intern("llvm.fabs.f128");
  call.args = {bir::Value::named(bir::TypeKind::F128, "%src")};
  call.arg_types = {bir::TypeKind::F128};
  call.return_type = bir::TypeKind::F128;
  call.intrinsic = bir::IntrinsicOperation{
      .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
      .operation = bir::IntrinsicOperationKind::FAbs,
      .operand_type = bir::TypeKind::F128,
      .result_type = bir::TypeKind::F128,
      .has_side_effects = false,
  };
  prepared.call_plans.functions.front().calls.front().direct_callee_name =
      std::string{"llvm.fabs.f128"};
  auto& carrier = prepared.intrinsic_carriers.functions.front().carriers.front();
  carrier.carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete;
  carrier.family = bir::IntrinsicFamilyKind::ScalarFpUnary;
  carrier.operation = bir::IntrinsicOperationKind::FAbs;
  carrier.operand_type = bir::TypeKind::F128;
  carrier.result_type = bir::TypeKind::F128;
  carrier.operand = bir::Value::named(bir::TypeKind::F128, "%src");
  carrier.result = bir::Value::named(bir::TypeKind::F128, "%fabs");
  carrier.source_callee_name = std::string{"llvm.fabs.f128"};
  carrier.has_prepared_call_plan = true;
  carrier.missing_required_facts.clear();

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
  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().message.find("unsupported_operand_type") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected F128 fabs-shaped carrier to fail closed without call fallback");
  }
  if (std::holds_alternative<aarch64_codegen::ScalarFpUnaryIntrinsicRecord>(
          block.instructions.front().target.payload) ||
      std::holds_alternative<aarch64_codegen::CallInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected F128 fabs-shaped carrier not to select intrinsic or call");
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

int semantic_symbol_address_argument_avoids_deferred_call_boundary_move() {
  auto prepared = prepared_with_direct_variadic_call_symbol_address_argument();
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
    return fail("expected symbol-address call argument dispatch to emit arg move, call, and return");
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
      move->move.from_value_id != prepare::PreparedValueId{1} ||
      move->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      move->move.destination_register_name != std::optional<std::string>{"x0"} ||
      !move->source_register.has_value() ||
      move->source_register->reg != aarch64_module::abi::x_register(21) ||
      !move->destination_register.has_value() ||
      move->destination_register->reg != aarch64_module::abi::x_register(0) ||
      move->source_bundle != &function_context.value_locations->move_bundles.front() ||
      move->source_move !=
          &function_context.value_locations->move_bundles.front().moves.front()) {
    return fail(
        "expected symbol-address call argument to use authoritative prepared x21 -> x0 move");
  }
  if (call == nullptr || !call->direct_callee.has_value() ||
      call->direct_callee_label != "printf" ||
      call->wrapper_kind != prepare::PreparedCallWrapperKind::DirectExternVariadic ||
      call->prepared_arguments.size() != 1 ||
      call->prepared_arguments.front().source_encoding !=
          prepare::PreparedStorageEncodingKind::SymbolAddress ||
      call->prepared_arguments.front().source_symbol_name !=
          std::optional<std::string>{"@.str0"} ||
      call->prepared_arguments.front().source_register_name !=
          std::optional<std::string>{"x21"} ||
      call->prepared_arguments.front().destination_register_name !=
          std::optional<std::string>{"x0"}) {
    return fail("expected direct variadic call to preserve symbol-address argument facts");
  }

  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok) {
    return fail(
        "expected symbol-address call argument to avoid deferred_unsupported call-boundary "
        "move printing: " +
        printed.diagnostic);
  }
  if (printed.assembly.find("bl printf") == std::string::npos) {
    return fail("expected printable symbol-address call route to keep the direct printf call");
  }
  if (printed.assembly.find("mov x0, x21") == std::string::npos ||
      printed.assembly.find("mov x0, x20") != std::string::npos) {
    return fail(
        "expected printed symbol-address call route to move authoritative x21 into x0");
  }
  return 0;
}

int semantic_stack_call_argument_publishes_destination_offset() {
  auto prepared = prepared_semantic_aarch64_stack_call_argument();
  if (prepared.call_plans.functions.size() != 1 ||
      prepared.call_plans.functions.front().calls.size() != 1 ||
      prepared.call_plans.functions.front().calls.front().arguments.size() != 1) {
    return fail("expected one prepared AArch64 stack call-argument plan");
  }

  const auto& argument =
      prepared.call_plans.functions.front().calls.front().arguments.front();
  if (argument.destination_register_name.has_value() ||
      argument.destination_register_bank.has_value() ||
      argument.destination_stack_offset_bytes != std::optional<std::size_t>{0}) {
    return fail("expected prepared AArch64 stack call argument to publish dest stack offset 0");
  }

  const auto* function_locations = prepare::find_prepared_value_location_function(
      prepared.value_locations, prepared.call_plans.functions.front().function_name);
  const auto* bundle =
      function_locations != nullptr
          ? prepare::find_prepared_move_bundle(*function_locations,
                                               prepare::PreparedMovePhase::BeforeCall,
                                               0,
                                               0)
          : nullptr;
  if (bundle == nullptr || bundle->moves.size() != 1 || bundle->abi_bindings.size() != 1) {
    return fail("expected one before-call stack argument move and ABI binding");
  }

  const auto& move = bundle->moves.front();
  const auto& binding = bundle->abi_bindings.front();
  if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      move.destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      move.destination_abi_index != std::optional<std::size_t>{0} ||
      move.destination_stack_offset_bytes != std::optional<std::size_t>{0} ||
      binding.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      binding.destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      binding.destination_abi_index != std::optional<std::size_t>{0} ||
      binding.destination_stack_offset_bytes != std::optional<std::size_t>{0}) {
    return fail("expected before-call stack argument records to carry dest stack offset 0");
  }

  return 0;
}

int semantic_stack_call_argument_lowers_aggregate_address_stack_copy() {
  constexpr auto function_name = c4c::FunctionNameId{101};
  constexpr auto block_label = c4c::BlockLabelId{102};
  constexpr auto value_id = prepare::PreparedValueId{103};
  constexpr auto value_name = c4c::ValueNameId{104};

  prepare::PreparedBirModule prepared;
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = value_id,
              .function_name = function_name,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x20"},
              .size_bytes = std::size_t{16},
              .align_bytes = std::size_t{8},
          }},
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .block_index = 0,
              .instruction_index = 4,
              .moves =
                  {prepare::PreparedMoveResolution{
                      .from_value_id = value_id,
                      .to_value_id = value_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{0},
                      .destination_stack_offset_bytes = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  }},
              .abi_bindings =
                  {prepare::PreparedAbiBinding{
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{0},
                      .destination_stack_offset_bytes = std::size_t{0},
                  }},
          }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 4,
      .arguments =
          {prepare::PreparedCallArgumentPlan{
              .instruction_index = 4,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::AggregateAddress,
              .source_encoding = prepare::PreparedStorageEncodingKind::Register,
              .source_value_id = value_id,
              .source_register_name = std::string{"x20"},
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_stack_offset_bytes = std::size_t{0},
          }},
  };
  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered =
      aarch64_codegen::lower_before_call_moves(block_context, call_plan, 4, diagnostics);
  if (lowered.size() != 1 || !diagnostics.empty()) {
    return fail("expected aggregate stack argument to lower to one copy node");
  }
  const auto* copy =
      std::get_if<aarch64_module::codegen::AssemblerInstructionRecord>(
          &lowered.front().target.payload);
  if (copy == nullptr ||
      lowered.front().target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      !copy->has_inline_asm_payload || copy->operands.size() != 2) {
    return fail("expected stack call argument to lower to a selected aggregate copy node");
  }
  const auto* source =
      std::get_if<aarch64_module::codegen::MemoryOperand>(
          &copy->operands[0].payload);
  const auto* destination =
      std::get_if<aarch64_module::codegen::MemoryOperand>(
          &copy->operands[1].payload);
  if (source == nullptr || destination == nullptr ||
      source->base_kind != aarch64_module::codegen::MemoryBaseKind::PointerValue ||
      !source->base_register.has_value() ||
      source->base_register->reg != aarch64_module::abi::x_register(20) ||
      source->size_bytes != 16 ||
      destination->base_kind != aarch64_module::codegen::MemoryBaseKind::FrameSlot ||
      destination->byte_offset != 0 ||
      destination->size_bytes != 16 ||
      destination->stored_value_name != source->result_value_name) {
    return fail("expected aggregate stack copy to preserve source pointer and destination stack facts");
  }
  if (copy->inline_asm_template.find("ldr ") == std::string::npos ||
      copy->inline_asm_template.find("str ") == std::string::npos ||
      copy->inline_asm_template.find("[x20]") == std::string::npos ||
      copy->inline_asm_template.find("[x20, #8]") == std::string::npos ||
      copy->inline_asm_template.find("[sp]") == std::string::npos ||
      copy->inline_asm_template.find("[sp, #8]") == std::string::npos) {
    return fail("expected aggregate stack copy to materialize load/store assembly");
  }
  return 0;
}

int prepared_frame_slot_stack_call_argument_lowers_to_selected_store() {
  constexpr auto function_name = c4c::FunctionNameId{91};
  constexpr auto block_label = c4c::BlockLabelId{92};
  constexpr auto value_id = prepare::PreparedValueId{93};
  constexpr auto value_name = c4c::ValueNameId{94};

  prepare::PreparedBirModule prepared;
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
  });

  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = value_id,
              .function_name = function_name,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{5},
              .offset_bytes = std::size_t{32},
              .size_bytes = std::size_t{8},
              .align_bytes = std::size_t{8},
          }},
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .block_index = 0,
              .instruction_index = 4,
              .moves =
                  {prepare::PreparedMoveResolution{
                      .from_value_id = value_id,
                      .to_value_id = value_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{0},
                      .destination_stack_offset_bytes = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  }},
              .abi_bindings =
                  {prepare::PreparedAbiBinding{
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{0},
                      .destination_stack_offset_bytes = std::size_t{0},
                  }},
          }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 4,
      .arguments =
          {prepare::PreparedCallArgumentPlan{
              .instruction_index = 4,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .source_value_id = value_id,
              .source_slot_id = prepare::PreparedFrameSlotId{5},
              .source_stack_offset_bytes = std::size_t{32},
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_stack_offset_bytes = std::size_t{0},
          }},
  };
  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered =
      aarch64_codegen::lower_before_call_moves(block_context, call_plan, 4, diagnostics);
  if (lowered.size() != 1 || !diagnostics.empty()) {
    return fail("expected prepared frame-slot stack argument to lower to one store");
  }
  const auto* store =
      std::get_if<aarch64_module::codegen::MemoryInstructionRecord>(
          &lowered.front().target.payload);
  const auto* source =
      store != nullptr && store->value.has_value()
          ? std::get_if<aarch64_module::codegen::MemoryOperand>(
                &store->value->payload)
          : nullptr;
  if (store == nullptr ||
      lowered.front().target.opcode != aarch64_module::codegen::MachineOpcode::Store ||
      lowered.front().target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      store->address.byte_offset != 0 ||
      store->address.size_bytes != 8 ||
      store->address.stored_value_id != std::optional<prepare::PreparedValueId>{value_id} ||
      store->address.stored_value_name != value_name ||
      source == nullptr ||
      source->frame_slot_id != std::optional<prepare::PreparedFrameSlotId>{5} ||
      source->byte_offset != 32 ||
      source->size_bytes != 8) {
    return fail("expected selected stack call argument store to preserve source and destination facts");
  }
  return 0;
}

int stack_home_symbol_address_argument_materializes_directly_to_call_register() {
  auto prepared = prepared_with_direct_variadic_call_stack_symbol_address_argument();
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
      result.emitted_instructions != 5 || block.instructions.size() != 5 ||
      !diagnostics.empty()) {
    return fail("expected stack-home symbol call argument to materialize before direct call: emitted=" +
                std::to_string(result.emitted_instructions) +
                " block_size=" + std::to_string(block.instructions.size()) +
                " diagnostics=" + std::to_string(diagnostics.entries.size()));
  }

  const auto* payload_address =
      std::get_if<aarch64_module::codegen::AddressMaterializationRecord>(
          &block.instructions[1].target.payload);
  if (payload_address == nullptr ||
      payload_address->kind !=
          aarch64_module::codegen::AddressMaterializationKind::StringConstant ||
      !payload_address->result_register.has_value() ||
      payload_address->result_register->reg != aarch64_module::abi::x_register(1) ||
      payload_address->result_value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{2}}) {
    return fail("expected stack-home string argument address to materialize directly into x1");
  }

  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok) {
    return fail("expected stack-home symbol argument route to print: " + printed.diagnostic);
  }
  if (printed.assembly.find("adrp x1, .payload") == std::string::npos ||
      printed.assembly.find("add x1, x1, :lo12:.payload") == std::string::npos ||
      printed.assembly.find("bl printf") == std::string::npos) {
    return fail("expected printed call route to materialize payload string into x1");
  }
  return 0;
}

int load_global_call_argument_uses_got_for_got_required_global() {
  auto prepared = prepared_with_load_global_call_argument(
      bir::GlobalAddressMaterializationPolicy::GotRequired);
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

  if (result.visited_operations != 2 || !result.visited_terminator ||
      result.emitted_instructions != 4 || block.instructions.size() != 4 ||
      !diagnostics.empty()) {
    return fail("expected GOT-required global load call route to select load/move/call/return");
  }

  const auto* got_load =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &block.instructions[0].target.payload);
  const auto* move =
      std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(
          &block.instructions[1].target.payload);
  if (got_load == nullptr || !got_load->has_inline_asm_payload ||
      got_load->inline_asm_template.find("adrp x2, :got:external_data_symbol") ==
          std::string::npos ||
      got_load->inline_asm_template.find(
          "ldr x2, [x2, :got_lo12:external_data_symbol]") ==
          std::string::npos ||
      got_load->inline_asm_template.find("ldr x2, [x2]") == std::string::npos) {
    return fail("expected GOT-required global load to use GOT page/low12 materialization");
  }
  if (got_load->inline_asm_template.find(":lo12:external_data_symbol") !=
      std::string::npos) {
    return fail("GOT-required global load must not use direct :lo12: relocation");
  }
  if (move == nullptr || !move->source_register.has_value() ||
      !move->destination_register.has_value() ||
      move->source_register->reg != aarch64_abi::x_register(2) ||
      move->destination_register->reg != aarch64_abi::x_register(0)) {
    return fail("expected call move to consume the GOT-materialized global value");
  }

  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok) {
    return fail("expected GOT-required global route to print: " + printed.diagnostic);
  }
  if (printed.assembly.find("adrp x2, :got:external_data_symbol") ==
          std::string::npos ||
      printed.assembly.find("ldr x2, [x2, :got_lo12:external_data_symbol]") ==
          std::string::npos ||
      printed.assembly.find("adrp x2, external_data_symbol") !=
          std::string::npos ||
      printed.assembly.find(":lo12:external_data_symbol") != std::string::npos) {
    return fail("expected printed GOT-required route to avoid direct external relocation");
  }
  return 0;
}

int load_global_call_argument_keeps_direct_for_direct_global() {
  auto prepared = prepared_with_load_global_call_argument(
      bir::GlobalAddressMaterializationPolicy::Direct);
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

  if (result.visited_operations != 2 || !result.visited_terminator ||
      result.emitted_instructions != 4 || block.instructions.size() != 4 ||
      !diagnostics.empty()) {
    return fail("expected direct global load call route to select load/move/call/return: emitted=" +
                std::to_string(result.emitted_instructions) +
                " block_size=" + std::to_string(block.instructions.size()) +
                " diagnostics=" + std::to_string(diagnostics.entries.size()) +
                (diagnostics.entries.empty()
                     ? std::string{}
                     : " first=" + diagnostics.entries.front().message));
  }

  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok) {
    return fail("expected direct global route to print: " + printed.diagnostic);
  }
  if (printed.assembly.find("adrp x9, internal_data_symbol") ==
          std::string::npos ||
      printed.assembly.find("add x9, x9, :lo12:internal_data_symbol") ==
          std::string::npos ||
      printed.assembly.find(":got:internal_data_symbol") != std::string::npos ||
      printed.assembly.find(":got_lo12:internal_data_symbol") !=
          std::string::npos) {
    return fail("expected direct global route to preserve direct page/low12 relocation");
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

int block_dispatch_lowers_prepared_f128_argument_q_register_move_before_direct_call() {
  auto prepared = prepared_with_direct_call_f128_argument_register_move();
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
    return fail("expected f128 call dispatch to emit q-register arg move, call, and return");
  }

  const auto* move =
      std::get_if<aarch64_module::codegen::CallBoundaryMoveInstructionRecord>(
          &block.instructions[0].target.payload);
  if (move == nullptr ||
      block.instructions[0].target.family !=
          aarch64_module::codegen::InstructionFamily::CallBoundary ||
      block.instructions[0].target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      !move->source_register.has_value() || !move->destination_register.has_value() ||
      move->source_register->reg != aarch64_module::abi::q_register(2) ||
      move->destination_register->reg != aarch64_module::abi::q_register(0) ||
      move->source_register->prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      move->destination_register->prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      move->source_register->expected_view != aarch64_module::abi::RegisterView::Q ||
      move->destination_register->expected_view != aarch64_module::abi::RegisterView::Q ||
      move->source_f128_carrier != &prepared.f128_carriers.functions.front().carriers.front() ||
      move->source_f128_carrier->total_size_bytes != 16 ||
      move->source_f128_carrier->total_align_bytes != 16) {
    return fail("expected prepared f128 before-call move to select q2 -> q0");
  }
  return 0;
}

int block_dispatch_exposes_f128_constant_argument_carrier_to_selection() {
  auto prepared = prepared_with_direct_call_f128_constant_argument();
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
    return fail("expected f128 constant call dispatch to emit selected carrier, call, and return");
  }

  const auto* move =
      std::get_if<aarch64_module::codegen::CallBoundaryMoveInstructionRecord>(
          &block.instructions[0].target.payload);
  if (move == nullptr ||
      block.instructions[0].target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      move->source_register.has_value() ||
      !move->destination_register.has_value() ||
      move->destination_register->reg != aarch64_module::abi::q_register(0) ||
      move->source_f128_carrier != &prepared.f128_carriers.functions.front().carriers.front() ||
      !move->source_f128_constant_payload.has_value() ||
      move->source_f128_constant_payload->low_bits != 0x0123456789abcdefULL ||
      move->source_f128_constant_payload->high_bits != 0x3fff800000000000ULL ||
      move->source_f128_carrier->kind != prepare::PreparedF128CarrierKind::Missing ||
      !move->source_f128_carrier->constant_payload.has_value()) {
    return fail("expected selected f128 constant carrier to preserve full-width payload facts");
  }
  return 0;
}

int semantic_f128_constant_argument_reaches_call_boundary_selection() {
  auto prepared = prepared_semantic_f128_constant_call_argument();
  const auto function_id =
      prepared.names.function_names.find("dispatch.semantic.f128.const.arg");
  const prepare::PreparedControlFlowFunction* function_cf = nullptr;
  for (const auto& candidate : prepared.control_flow.functions) {
    if (candidate.function_name == function_id) {
      function_cf = &candidate;
      break;
    }
  }
  if (function_cf == nullptr || function_cf->blocks.empty()) {
    return fail("expected semantic f128 constant fixture to produce prepared control flow");
  }
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, *function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, function_cf->blocks.front(), 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 3 || block.instructions.size() != 3 ||
      !diagnostics.empty()) {
    return fail("expected semantic f128 constant call to dispatch through normal call-boundary route");
  }

  const auto* move =
      std::get_if<aarch64_module::codegen::CallBoundaryMoveInstructionRecord>(
          &block.instructions[0].target.payload);
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &block.instructions[1].target.payload);
  if (move == nullptr ||
      block.instructions[0].target.family !=
          aarch64_module::codegen::InstructionFamily::CallBoundary ||
      block.instructions[0].target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      move->source_register.has_value() ||
      !move->destination_register.has_value() ||
      move->destination_register->reg != aarch64_module::abi::q_register(0) ||
      move->source_f128_carrier == nullptr ||
      move->source_f128_carrier->kind != prepare::PreparedF128CarrierKind::Missing ||
      move->source_f128_carrier->source_type != bir::TypeKind::F128 ||
      move->source_f128_carrier->total_size_bytes != 16 ||
      move->source_f128_carrier->total_align_bytes != 16 ||
      !move->source_f128_carrier->constant_payload.has_value() ||
      !move->source_f128_constant_payload.has_value() ||
      move->source_f128_constant_payload->low_bits != 0x0123456789abcdefULL ||
      move->source_f128_constant_payload->high_bits != 0x3fff800000000000ULL ||
      move->source_f128_constant_payload->low_bits !=
          move->source_f128_carrier->constant_payload->low_bits ||
      move->source_f128_constant_payload->high_bits !=
          move->source_f128_carrier->constant_payload->high_bits) {
    return fail("expected semantic f128 constant to arrive as a full-width structured payload");
  }
  if (call == nullptr || !call->direct_callee.has_value() ||
      call->direct_callee_label != "consume_tf") {
    return fail("expected semantic f128 constant carrier to feed the existing direct call consumer");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions[2].target.payload)) {
    return fail("expected return terminator after semantic f128 constant call");
  }
  return 0;
}

int semantic_f128_constant_helper_operand_reaches_carrier_and_fails_closed() {
  auto prepared = prepared_semantic_f128_constant_helper_operand();
  const auto function_id =
      prepared.names.function_names.find("dispatch.semantic.f128.const.helper");
  const auto* helpers = prepare::find_prepared_f128_runtime_helpers(prepared, function_id);
  const auto* carriers = prepare::find_prepared_f128_carriers(prepared, function_id);
  if (helpers == nullptr || helpers->helpers.empty() || carriers == nullptr) {
    return fail("expected semantic f128 constant helper fixture to produce helper carriers");
  }
  const auto& helper = helpers->helpers.front();
  const auto* rhs_carrier = prepare::find_prepared_f128_carrier(*carriers, helper.rhs_value_id);
  if (rhs_carrier == nullptr ||
      rhs_carrier->kind != prepare::PreparedF128CarrierKind::Missing ||
      !rhs_carrier->constant_payload.has_value() ||
      rhs_carrier->constant_payload->low_bits != 0x0123456789abcdefULL ||
      rhs_carrier->constant_payload->high_bits != 0x3fff800000000000ULL) {
    return fail("expected f128 helper rhs to resolve the structured constant carrier");
  }
  const auto missing_fact = std::find(helper.missing_required_facts.begin(),
                                      helper.missing_required_facts.end(),
                                      "rhs_requires_full_width_f128_carrier");
  if (missing_fact == helper.missing_required_facts.end()) {
    return fail("expected f128 helper constant rhs to stay fail-closed before rematerialization");
  }

  const prepare::PreparedControlFlowFunction* function_cf = nullptr;
  for (const auto& candidate : prepared.control_flow.functions) {
    if (candidate.function_name == function_id) {
      function_cf = &candidate;
      break;
    }
  }
  if (function_cf == nullptr || function_cf->blocks.empty()) {
    return fail("expected semantic f128 constant helper fixture to produce control flow");
  }
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, *function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, function_cf->blocks.front(), 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);
  if (result.visited_operations != 1 || diagnostics.entries.empty()) {
    return fail("expected f128 helper constant operand to fail closed during dispatch");
  }
  const auto diagnostic = std::find_if(
      diagnostics.entries.begin(),
      diagnostics.entries.end(),
      [](const aarch64_module::ModuleLoweringDiagnostic& entry) {
        return entry.message.find("incomplete_prepared_f128_runtime_helper") !=
               std::string::npos;
      });
  if (diagnostic == diagnostics.entries.end()) {
    return fail("expected dispatch diagnostic for incomplete f128 helper constant route");
  }
  return 0;
}

int block_dispatch_rejects_incomplete_f128_constant_argument_carriers() {
  struct Case {
    const char* label;
    prepare::PreparedBirModule prepared;
  };
  std::vector<Case> cases;
  cases.push_back(Case{
      .label = "missing_payload",
      .prepared = prepared_with_direct_call_f128_constant_argument(false),
  });
  cases.push_back(Case{
      .label = "missing_source_value",
      .prepared = prepared_with_direct_call_f128_constant_argument(true, false),
  });
  cases.push_back(Case{
      .label = "scalar_literal",
      .prepared =
          prepared_with_direct_call_f128_constant_argument(true, true, bir::TypeKind::F64),
  });
  for (auto& item : cases) {
    auto& prepared = item.prepared;
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
        diagnostics.entries.size() != 1 ||
        diagnostics.entries.front().message.find(
            "complete structured full-width constant carrier") == std::string::npos ||
        !std::holds_alternative<aarch64_module::codegen::CallInstructionRecord>(
            block.instructions.front().target.payload)) {
      return fail(std::string{"expected f128 constant argument dispatch to reject "} +
                  item.label);
    }
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
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
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
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
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
  const auto dynamic_result = aarch64_codegen::compile_prepared_module(dynamic_frame);
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
  const auto callee_save_result =
      aarch64_codegen::compile_prepared_module(callee_save_frame);
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

int block_dispatch_lowers_prepared_frame_slot_load_with_result_register() {
  auto prepared = prepared_with_frame_slot_load();
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected dispatch to select frame-slot load plus return");
  }

  const auto* memory =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(
          &block.instructions.front().target.payload);
  if (memory == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      memory->memory_kind != aarch64_codegen::MemoryInstructionKind::Load ||
      memory->address.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      memory->address.frame_slot_id != prepare::PreparedFrameSlotId{20} ||
      memory->address.byte_offset != 8 || memory->address.size_bytes != 4 ||
      memory->address.align_bytes != 4 ||
      memory->address.address_space != bir::AddressSpace::Fs ||
      !memory->address.is_volatile ||
      memory->result_value_id != prepare::PreparedValueId{10} ||
      !memory->result_register.has_value() ||
      memory->result_register->occupied_registers.empty() ||
      memory->result_register->occupied_registers.front() != "w0") {
    return fail("expected selected frame-slot load to preserve structured facts");
  }
  if (block.instructions.front().target.defs.empty() ||
      block.instructions.front().target.defs.front().kind !=
          aarch64_codegen::MachineEffectResourceKind::Register ||
      block.instructions.front().target.side_effects.empty() ||
      block.instructions.front().target.side_effects.front() !=
          aarch64_codegen::MachineSideEffectKind::MemoryRead) {
    return fail("expected frame-slot load target to expose register def and memory read");
  }

  return 0;
}

int block_dispatch_lowers_i128_frame_slot_load_from_prepared_carrier() {
  auto prepared = prepared_with_i128_frame_slot_load();
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected dispatch to select i128 frame-slot transport plus return");
  }

  const auto* transport =
      std::get_if<aarch64_codegen::I128TransportRecord>(
          &block.instructions.front().target.payload);
  if (transport == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      transport->transport_kind != aarch64_codegen::I128TransportKind::LoadFromMemory ||
      transport->value_id != prepare::PreparedValueId{110} ||
      transport->carrier_kind != prepare::PreparedI128CarrierKind::RegisterPair ||
      transport->low_lane.role != prepare::PreparedI128LaneRole::Low ||
      transport->high_lane.role != prepare::PreparedI128LaneRole::High ||
      !transport->low_lane.reg.has_value() ||
      !transport->high_lane.reg.has_value() ||
      transport->low_lane.reg->reg != aarch64_abi::x_register(4) ||
      transport->high_lane.reg->reg != aarch64_abi::x_register(5) ||
      !transport->memory.has_value() ||
      transport->memory->frame_slot_id != prepare::PreparedFrameSlotId{20} ||
      transport->memory->byte_offset != 16 ||
      transport->memory->size_bytes != 16 ||
      transport->memory->align_bytes != 16 ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::I128Transport ||
      block.instructions.front().target.side_effects.size() != 1 ||
      block.instructions.front().target.side_effects.front() !=
          aarch64_codegen::MachineSideEffectKind::MemoryRead) {
    return fail("expected i128 transport dispatch to preserve carrier lanes and memory facts");
  }
  return 0;
}

int block_dispatch_reports_missing_i128_carrier_authority() {
  auto prepared = prepared_with_i128_frame_slot_load(false);
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

  if (diagnostics.entries.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.front().message.find("missing_prepared_i128_carrier") ==
          std::string::npos) {
    return fail("expected i128 transport dispatch to fail closed without carrier authority");
  }
  return 0;
}

int block_dispatch_lowers_f128_frame_slot_load_from_prepared_carrier() {
  auto prepared = prepared_with_f128_frame_slot_load(true);
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    for (const auto& diagnostic : diagnostics.entries) {
      std::cerr << diagnostic.message << "\n";
    }
    return fail("expected dispatch to select f128 frame-slot transport plus return");
  }

  const auto* transport =
      std::get_if<aarch64_codegen::F128TransportRecord>(
          &block.instructions.front().target.payload);
  if (transport == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      transport->transport_kind != aarch64_codegen::F128TransportKind::LoadFromMemory ||
      transport->value_id != prepare::PreparedValueId{111} ||
      transport->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      !transport->reg.has_value() ||
      transport->reg->reg != aarch64_abi::q_register(4) ||
      transport->total_size_bytes != 16 ||
      transport->total_align_bytes != 16 ||
      !transport->memory.has_value() ||
      transport->memory->frame_slot_id != prepare::PreparedFrameSlotId{20} ||
      transport->memory->byte_offset != 16 ||
      transport->memory->size_bytes != 16 ||
      transport->memory->align_bytes != 16 ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::F128Transport ||
      block.instructions.front().target.side_effects.size() != 1 ||
      block.instructions.front().target.side_effects.front() !=
          aarch64_codegen::MachineSideEffectKind::MemoryRead) {
    return fail("expected f128 transport dispatch to preserve carrier and memory facts");
  }
  return 0;
}

int block_dispatch_reports_missing_f128_carrier_authority() {
  auto prepared = prepared_with_f128_frame_slot_load();
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
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingValueAuthority ||
      diagnostics.entries.front().instruction_family !=
          aarch64_module::InstructionLoweringFamily::Memory ||
      diagnostics.entries.front().message.find("missing_prepared_f128_carrier") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected f128 memory transport to fail closed without carrier authority");
  }
  return 0;
}

int block_dispatch_lowers_f128_frame_slot_store_from_prepared_carrier() {
  auto prepared = prepared_with_f128_frame_slot_store();
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    for (const auto& diagnostic : diagnostics.entries) {
      std::cerr << diagnostic.message << "\n";
    }
    return fail("expected dispatch to select f128 frame-slot store transport plus return");
  }

  const auto* transport =
      std::get_if<aarch64_codegen::F128TransportRecord>(
          &block.instructions.front().target.payload);
  if (transport == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      transport->transport_kind != aarch64_codegen::F128TransportKind::StoreToMemory ||
      transport->value_id != prepare::PreparedValueId{112} ||
      transport->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      transport->register_bank != prepare::PreparedRegisterBank::Vreg ||
      transport->register_class != prepare::PreparedRegisterClass::Vector ||
      !transport->reg.has_value() ||
      transport->reg->reg != aarch64_abi::q_register(5) ||
      transport->total_size_bytes != 16 ||
      transport->total_align_bytes != 16 ||
      !transport->memory.has_value() ||
      transport->memory->stored_value_id != prepare::PreparedValueId{112} ||
      transport->memory->frame_slot_id != prepare::PreparedFrameSlotId{21} ||
      transport->memory->byte_offset != 32 ||
      transport->memory->size_bytes != 16 ||
      transport->memory->align_bytes != 16 ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::F128Transport ||
      block.instructions.front().target.side_effects.size() != 1 ||
      block.instructions.front().target.side_effects.front() !=
          aarch64_codegen::MachineSideEffectKind::MemoryWrite) {
    return fail("expected f128 store transport dispatch to preserve carrier and memory facts");
  }
  return 0;
}

int block_dispatch_prints_representative_f128_load_helper_store_route() {
  auto prepared = prepared_with_f128_load_add_store_route();
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

  if (!diagnostics.empty() || result.visited_operations != 3 ||
      !result.visited_terminator || result.emitted_instructions != 4 ||
      block.instructions.size() != 4) {
    for (const auto& diagnostic : diagnostics.entries) {
      std::cerr << diagnostic.message << "\n";
    }
    return fail("expected representative f128 route to select load/helper/store/return");
  }

  const auto* load =
      std::get_if<aarch64_codegen::F128TransportRecord>(
          &block.instructions[0].target.payload);
  const auto* helper =
      std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
          &block.instructions[1].target.payload);
  const auto* store =
      std::get_if<aarch64_codegen::F128TransportRecord>(
          &block.instructions[2].target.payload);
  const bool all_nodes_selected = std::all_of(
      block.instructions.begin(),
      block.instructions.end(),
      [](const aarch64_module::MachineInstruction& instruction) {
        return instruction.target.selection.status ==
               aarch64_codegen::MachineNodeSelectionStatus::Selected;
      });
  if (load == nullptr || helper == nullptr || store == nullptr ||
      !all_nodes_selected ||
      load->transport_kind != aarch64_codegen::F128TransportKind::LoadFromMemory ||
      load->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      load->value_type != bir::TypeKind::F128 ||
      load->total_size_bytes != 16 ||
      load->total_align_bytes != 16 ||
      !load->reg.has_value() ||
      !load->memory.has_value() ||
      load->reg->reg != aarch64_abi::q_register(6) ||
      load->reg->prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      load->reg->expected_view != aarch64_abi::RegisterView::Q ||
      load->memory->size_bytes != 16 ||
      load->memory->align_bytes != 16 ||
      helper->callee_name != "__addtf3" ||
      helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Add ||
      helper->source_binary_opcode != bir::BinaryOpcode::Add ||
      helper->source_type != bir::TypeKind::F128 ||
      helper->result_type != bir::TypeKind::F128 ||
      helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      helper->width_bytes != 16 ||
      helper->align_bytes != 16 ||
      !helper->lhs.carrier_register.has_value() ||
      !helper->rhs.carrier_register.has_value() ||
      !helper->result.carrier_register.has_value() ||
      helper->lhs.carrier_register->reg != aarch64_abi::q_register(6) ||
      helper->rhs.carrier_register->reg != aarch64_abi::q_register(8) ||
      helper->result.carrier_register->reg != aarch64_abi::q_register(4) ||
      helper->lhs.carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      helper->rhs.carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      helper->result.carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      helper->lhs.width_bytes != 16 ||
      helper->rhs.width_bytes != 16 ||
      helper->result.width_bytes != 16 ||
      !helper->lhs.abi_register.has_value() ||
      !helper->rhs.abi_register.has_value() ||
      !helper->result.abi_register.has_value() ||
      helper->lhs.abi_register->reg != aarch64_abi::q_register(0) ||
      helper->rhs.abi_register->reg != aarch64_abi::q_register(1) ||
      helper->result.abi_register->reg != aarch64_abi::q_register(0) ||
      helper->abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg ||
      helper->selected_call_ownership.owns_terminal_call != true ||
      !helper->selected_call_ownership.has_marshaling ||
      store->transport_kind != aarch64_codegen::F128TransportKind::StoreToMemory ||
      store->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      store->value_type != bir::TypeKind::F128 ||
      store->total_size_bytes != 16 ||
      store->total_align_bytes != 16 ||
      !store->reg.has_value() ||
      !store->memory.has_value() ||
      store->reg->reg != aarch64_abi::q_register(4) ||
      store->reg->prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      store->reg->expected_view != aarch64_abi::RegisterView::Q ||
      store->memory->size_bytes != 16 ||
      store->memory->align_bytes != 16 ||
      store->memory->stored_value_id != prepare::PreparedValueId{272}) {
    return fail("expected representative f128 route to preserve full-width facts");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions[3].target.payload)) {
    return fail("expected representative f128 route to finish with selected return");
  }

  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok) {
    return fail("expected representative f128 route to print: " + printed.diagnostic);
  }
  const std::string expected =
      "    ldr q6, [sp]\n"
      "    mov v0.16b, v6.16b\n"
      "    mov v1.16b, v8.16b\n"
      "    bl __addtf3\n"
      "    mov v4.16b, v0.16b\n"
      "    str q4, [sp, #16]\n"
      "    ret\n";
  if (printed.assembly != expected) {
    std::cerr << printed.assembly;
    return fail("expected representative f128 route to print canonical AArch64 text");
  }
  return 0;
}

int block_dispatch_reports_incomplete_f128_carrier_authority() {
  auto prepared = prepared_with_f128_frame_slot_store(true, false);
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
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingValueAuthority ||
      diagnostics.entries.front().instruction_family !=
          aarch64_module::InstructionLoweringFamily::Memory ||
      diagnostics.entries.front().message.find("incomplete_prepared_f128_carrier") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected f128 memory transport to fail closed with incomplete carrier authority");
  }
  return 0;
}

int block_dispatch_lowers_i128_pair_add_from_prepared_carriers() {
  auto prepared = prepared_with_i128_pair_operation();
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected dispatch to select i128 pair add plus return");
  }

  const auto* pair =
      std::get_if<aarch64_codegen::I128PairOperationRecord>(
          &block.instructions.front().target.payload);
  if (pair == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::I128Pair ||
      pair->operation != aarch64_codegen::I128PairOperationKind::Add ||
      pair->lane_semantics !=
          aarch64_codegen::I128PairLaneSemantics::CarryPropagating ||
      pair->result.value_id != prepare::PreparedValueId{120} ||
      pair->lhs.value_id != prepare::PreparedValueId{121} ||
      pair->rhs.value_id != prepare::PreparedValueId{122} ||
      !pair->result.low_lane.reg.has_value() ||
      !pair->lhs.high_lane.reg.has_value() ||
      !pair->rhs.low_lane.reg.has_value() ||
      pair->result.low_lane.reg->reg != aarch64_abi::x_register(6) ||
      pair->lhs.high_lane.reg->reg != aarch64_abi::x_register(9) ||
      pair->rhs.low_lane.reg->reg != aarch64_abi::x_register(10) ||
      block.instructions.front().target.defs.size() != 2 ||
      block.instructions.front().target.uses.size() != 4) {
    return fail("expected selected i128 pair add to preserve source/result carriers");
  }
  return 0;
}

int block_dispatch_lowers_i128_pair_bitwise_from_prepared_carriers() {
  auto prepared = prepared_with_i128_pair_operation(bir::BinaryOpcode::And);
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

  const auto* pair =
      !block.instructions.empty()
          ? std::get_if<aarch64_codegen::I128PairOperationRecord>(
                &block.instructions.front().target.payload)
          : nullptr;
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || pair == nullptr ||
      pair->operation != aarch64_codegen::I128PairOperationKind::And ||
      pair->lane_semantics !=
          aarch64_codegen::I128PairLaneSemantics::IndependentBitwise) {
    return fail("expected dispatch to select i128 bitwise pair operation");
  }
  return 0;
}

int block_dispatch_lowers_i128_copy_from_prepared_carriers() {
  auto prepared = prepared_with_i128_copy_operation();
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

  const auto* copy =
      !block.instructions.empty()
          ? std::get_if<aarch64_codegen::I128TransportRecord>(
                &block.instructions.front().target.payload)
          : nullptr;
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      copy == nullptr ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::I128Transport ||
      copy->transport_kind != aarch64_codegen::I128TransportKind::CopyRegisterPair ||
      copy->value_id != prepare::PreparedValueId{130} ||
      copy->source_value_id != prepare::PreparedValueId{131} ||
      !copy->low_lane.reg.has_value() ||
      !copy->high_lane.reg.has_value() ||
      !copy->source_low_lane.reg.has_value() ||
      !copy->source_high_lane.reg.has_value() ||
      copy->low_lane.reg->reg != aarch64_abi::x_register(12) ||
      copy->high_lane.reg->reg != aarch64_abi::x_register(13) ||
      copy->source_low_lane.reg->reg != aarch64_abi::x_register(14) ||
      copy->source_high_lane.reg->reg != aarch64_abi::x_register(15) ||
      block.instructions.front().target.defs.size() != 2 ||
      block.instructions.front().target.uses.size() != 2 ||
      !block.instructions.front().target.side_effects.empty()) {
    return fail("expected i128 copy dispatch to preserve low/high source and result lanes");
  }
  const auto printed = print_route_block(function_cf.function_name, block);
  if (!printed.ok ||
      printed.assembly !=
          "    mov x12, x14\n"
          "    mov x13, x15\n"
          "    ret\n") {
    return fail("expected i128 copy route to print low and high lane moves");
  }
  return 0;
}

int block_dispatch_reports_missing_i128_pair_carrier_authority() {
  auto prepared = prepared_with_i128_pair_operation(bir::BinaryOpcode::Sub, false);
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

  if (diagnostics.entries.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.front().message.find("missing_prepared_i128_carrier") ==
          std::string::npos) {
    return fail("expected i128 pair dispatch to fail closed without source carrier");
  }
  return 0;
}

int block_dispatch_lowers_i128_runtime_helper_from_prepared_authority() {
  auto prepared = prepared_with_i128_runtime_helper_operation();
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

  const auto* helper =
      block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::I128RuntimeHelperBoundaryRecord>(
                &block.instructions.front().target.payload);
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      helper == nullptr ||
      block.instructions.front().target.opcode !=
          aarch64_codegen::MachineOpcode::I128RuntimeHelper ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::CallBoundary ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      helper->helper_kind != prepare::PreparedI128RuntimeHelperKind::SignedRem ||
      helper->callee_name != "__modti3" ||
      helper->source_binary_opcode != bir::BinaryOpcode::SRem ||
      helper->result_ownership !=
          prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes ||
      helper->result.low_lane.reg->reg != aarch64_abi::x_register(6) ||
      helper->lhs.high_lane.reg->reg != aarch64_abi::x_register(9) ||
      !helper->resource_policy.call_boundary ||
      helper->abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr ||
      !helper->live_preservation_policy.evaluated ||
      !helper->live_preservation_policy.no_additional_live_preservation_required ||
      !helper->selected_call_ownership.owns_terminal_call ||
      !helper->selected_call_ownership.has_live_preservation ||
      helper->clobbered_registers.empty() ||
      block.instructions.front().target.clobbers.empty()) {
    return fail("expected dispatch to select i128 helper boundary from prepared authority");
  }
  return 0;
}

int block_dispatch_reports_missing_i128_runtime_helper_authority() {
  auto missing_helper_prepared = prepared_with_i128_runtime_helper_operation(false);
  const auto& function_cf = missing_helper_prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      missing_helper_prepared, missing_helper_prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics missing_helper_diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, missing_helper_diagnostics);
  if (result.visited_operations != 1 ||
      result.emitted_instructions != 1 ||
      block.instructions.size() != 1 ||
      missing_helper_diagnostics.entries.size() != 1 ||
      missing_helper_diagnostics.entries.front().message.find(
          "missing_prepared_i128_runtime_helper") == std::string::npos) {
    return fail("expected i128 helper dispatch to fail closed without helper authority");
  }

  auto missing_live_prepared = prepared_with_i128_runtime_helper_operation();
  auto& helper = missing_live_prepared.i128_runtime_helpers.functions.front().helpers.front();
  helper.live_preservation_policy.no_additional_live_preservation_required = false;
  helper.selected_call_ownership.owns_terminal_call = false;
  helper.selected_call_ownership.has_live_preservation = false;
  const auto& live_function_cf = missing_live_prepared.control_flow.functions.front();
  const auto& live_block_cf = live_function_cf.blocks.front();
  const auto live_function_context = aarch64_codegen::make_function_lowering_context(
      missing_live_prepared, missing_live_prepared.target_profile, live_function_cf);
  const auto live_block_context =
      aarch64_codegen::make_block_lowering_context(live_function_context, live_block_cf, 0);

  aarch64_module::MachineBlock live_block;
  aarch64_module::ModuleLoweringDiagnostics missing_live_diagnostics;
  const auto live_result =
      aarch64_codegen::dispatch_prepared_block(
          live_block_context, live_block, missing_live_diagnostics);
  if (live_result.visited_operations != 1 ||
      live_result.emitted_instructions != 1 ||
      live_block.instructions.size() != 1 ||
      missing_live_diagnostics.entries.size() != 1 ||
      missing_live_diagnostics.entries.front().message.find(
          "incomplete_prepared_i128_runtime_helper") == std::string::npos) {
    return fail("expected i128 helper dispatch to fail closed without live preservation");
  }

  auto missing_clobber_prepared =
      prepared_with_i128_runtime_helper_operation(true, false);
  const auto& clobber_function_cf = missing_clobber_prepared.control_flow.functions.front();
  const auto& clobber_block_cf = clobber_function_cf.blocks.front();
  const auto clobber_function_context = aarch64_codegen::make_function_lowering_context(
      missing_clobber_prepared,
      missing_clobber_prepared.target_profile,
      clobber_function_cf);
  const auto clobber_block_context =
      aarch64_codegen::make_block_lowering_context(
          clobber_function_context, clobber_block_cf, 0);

  aarch64_module::MachineBlock clobber_block;
  aarch64_module::ModuleLoweringDiagnostics missing_clobber_diagnostics;
  const auto clobber_result =
      aarch64_codegen::dispatch_prepared_block(
          clobber_block_context, clobber_block, missing_clobber_diagnostics);
  if (clobber_result.visited_operations != 1 ||
      clobber_result.emitted_instructions != 1 ||
      clobber_block.instructions.size() != 1 ||
      missing_clobber_diagnostics.entries.size() != 1 ||
      missing_clobber_diagnostics.entries.front().message.find("missing_clobber_policy") ==
          std::string::npos) {
    return fail("expected i128 helper dispatch to fail closed without clobber policy");
  }
  return 0;
}

int block_dispatch_lowers_f128_runtime_helper_from_prepared_authority(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Add,
    prepare::PreparedF128RuntimeHelperKind expected_kind =
        prepare::PreparedF128RuntimeHelperKind::Add,
    std::string_view expected_callee = "__addtf3") {
  auto prepared = prepared_with_f128_runtime_helper_operation(opcode);
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

  const auto* helper =
      block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
                &block.instructions.front().target.payload);
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      helper == nullptr ||
      block.instructions.front().target.opcode !=
          aarch64_codegen::MachineOpcode::F128RuntimeHelper ||
      block.instructions.front().target.family !=
          aarch64_codegen::InstructionFamily::CallBoundary ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      helper->helper_kind != expected_kind ||
      helper->callee_name != std::string(expected_callee) ||
      helper->source_binary_opcode != opcode ||
      helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      helper->result.carrier_register->reg != aarch64_abi::q_register(4) ||
      helper->result.abi_register->reg != aarch64_abi::q_register(0) ||
      helper->lhs.carrier_register->reg != aarch64_abi::q_register(6) ||
      helper->rhs.abi_register->reg != aarch64_abi::q_register(1) ||
      !helper->resource_policy.call_boundary ||
      helper->abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg ||
      !helper->live_preservation_policy.evaluated ||
      !helper->live_preservation_policy.no_additional_live_preservation_required ||
      !helper->selected_call_ownership.owns_terminal_call ||
      !helper->selected_call_ownership.has_live_preservation ||
      helper->clobbered_registers.empty() ||
      block.instructions.front().target.clobbers.empty()) {
    return fail("expected dispatch to select f128 helper boundary from prepared authority");
  }
  return 0;
}

int block_dispatch_reports_missing_f128_runtime_helper_authority() {
  auto missing_helper_prepared =
      prepared_with_f128_runtime_helper_operation(bir::BinaryOpcode::Add, false);
  const auto& function_cf = missing_helper_prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      missing_helper_prepared, missing_helper_prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics missing_helper_diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, missing_helper_diagnostics);
  if (result.visited_operations != 1 ||
      result.emitted_instructions != 1 ||
      block.instructions.size() != 1 ||
      missing_helper_diagnostics.entries.size() != 1 ||
      missing_helper_diagnostics.entries.front().message.find(
          "missing_prepared_f128_runtime_helper") == std::string::npos) {
    return fail("expected f128 helper dispatch to fail closed without helper authority");
  }

  auto missing_live_prepared = prepared_with_f128_runtime_helper_operation();
  auto& helper = missing_live_prepared.f128_runtime_helpers.functions.front().helpers.front();
  helper.selected_call_ownership.owns_terminal_call = false;
  helper.selected_call_ownership.has_live_preservation = false;
  helper.live_preservation_policy.no_additional_live_preservation_required = false;
  const auto& live_function_cf = missing_live_prepared.control_flow.functions.front();
  const auto& live_block_cf = live_function_cf.blocks.front();
  const auto live_function_context = aarch64_codegen::make_function_lowering_context(
      missing_live_prepared, missing_live_prepared.target_profile, live_function_cf);
  const auto live_block_context =
      aarch64_codegen::make_block_lowering_context(live_function_context, live_block_cf, 0);

  aarch64_module::MachineBlock live_block;
  aarch64_module::ModuleLoweringDiagnostics missing_live_diagnostics;
  const auto live_result =
      aarch64_codegen::dispatch_prepared_block(
          live_block_context, live_block, missing_live_diagnostics);
  if (live_result.visited_operations != 1 ||
      live_result.emitted_instructions != 1 ||
      live_block.instructions.size() != 1 ||
      missing_live_diagnostics.entries.size() != 1 ||
      missing_live_diagnostics.entries.front().message.find(
          "incomplete_prepared_f128_runtime_helper") == std::string::npos) {
    return fail("expected f128 helper dispatch to fail closed without selected ownership");
  }

  auto missing_clobber_prepared =
      prepared_with_f128_runtime_helper_operation(bir::BinaryOpcode::Add, true, false);
  const auto& clobber_function_cf = missing_clobber_prepared.control_flow.functions.front();
  const auto& clobber_block_cf = clobber_function_cf.blocks.front();
  const auto clobber_function_context = aarch64_codegen::make_function_lowering_context(
      missing_clobber_prepared,
      missing_clobber_prepared.target_profile,
      clobber_function_cf);
  const auto clobber_block_context =
      aarch64_codegen::make_block_lowering_context(
          clobber_function_context, clobber_block_cf, 0);

  aarch64_module::MachineBlock clobber_block;
  aarch64_module::ModuleLoweringDiagnostics missing_clobber_diagnostics;
  const auto clobber_result =
      aarch64_codegen::dispatch_prepared_block(
          clobber_block_context, clobber_block, missing_clobber_diagnostics);
  if (clobber_result.visited_operations != 1 ||
      clobber_result.emitted_instructions != 1 ||
      clobber_block.instructions.size() != 1 ||
      missing_clobber_diagnostics.entries.size() != 1 ||
      missing_clobber_diagnostics.entries.front().message.find("missing_clobber_policy") ==
          std::string::npos) {
    return fail("expected f128 helper dispatch to fail closed without clobber policy");
  }
  return 0;
}

int block_dispatch_keeps_f128_sign_bit_candidate_fail_closed() {
  auto missing_helper_prepared =
      prepared_with_f128_runtime_helper_operation(bir::BinaryOpcode::Xor, false);
  const auto& function_cf = missing_helper_prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      missing_helper_prepared, missing_helper_prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics missing_helper_diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, missing_helper_diagnostics);
  if (result.visited_operations != 1 ||
      result.emitted_instructions != 1 ||
      block.instructions.size() != 1 ||
      missing_helper_diagnostics.entries.size() != 1 ||
      missing_helper_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingValueAuthority ||
      missing_helper_diagnostics.entries.front().message.find(
          "missing_prepared_f128_runtime_helper") == std::string::npos) {
    return fail("expected f128 sign-bit candidate to require prepared full-width authority");
  }

  auto mismatched_prepared =
      prepared_with_f128_runtime_helper_operation(bir::BinaryOpcode::Xor);
  const auto& mismatched_function_cf =
      mismatched_prepared.control_flow.functions.front();
  const auto& mismatched_block_cf = mismatched_function_cf.blocks.front();
  const auto mismatched_function_context =
      aarch64_codegen::make_function_lowering_context(
          mismatched_prepared, mismatched_prepared.target_profile, mismatched_function_cf);
  const auto mismatched_block_context =
      aarch64_codegen::make_block_lowering_context(
          mismatched_function_context, mismatched_block_cf, 0);

  aarch64_module::MachineBlock mismatched_block;
  aarch64_module::ModuleLoweringDiagnostics mismatched_diagnostics;
  const auto mismatched_result =
      aarch64_codegen::dispatch_prepared_block(
          mismatched_block_context, mismatched_block, mismatched_diagnostics);
  if (mismatched_result.visited_operations != 1 ||
      mismatched_result.emitted_instructions != 1 ||
      mismatched_block.instructions.size() != 1 ||
      mismatched_diagnostics.entries.size() != 1 ||
      mismatched_diagnostics.entries.front().message.find(
          "unsupported_source_operation") == std::string::npos) {
    return fail("expected f128 sign-bit candidate to reject arithmetic helper guesses");
  }
  return 0;
}

int block_dispatch_reports_unsupported_f128_helper_family_diagnostics() {
  const std::array unsupported_ops{
      bir::BinaryOpcode::UDiv,
      bir::BinaryOpcode::SRem,
      bir::BinaryOpcode::URem,
      bir::BinaryOpcode::And,
      bir::BinaryOpcode::Or,
      bir::BinaryOpcode::Shl,
  };
  for (const auto opcode : unsupported_ops) {
    auto missing_helper_prepared =
        prepared_with_f128_runtime_helper_operation(opcode, false);
    const auto& function_cf = missing_helper_prepared.control_flow.functions.front();
    const auto& block_cf = function_cf.blocks.front();
    const auto function_context = aarch64_codegen::make_function_lowering_context(
        missing_helper_prepared, missing_helper_prepared.target_profile, function_cf);
    const auto block_context =
        aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

    aarch64_module::MachineBlock block;
    aarch64_module::ModuleLoweringDiagnostics missing_helper_diagnostics;
    const auto result =
        aarch64_codegen::dispatch_prepared_block(
            block_context, block, missing_helper_diagnostics);
    if (result.visited_operations != 1 ||
        result.emitted_instructions != 1 ||
        block.instructions.size() != 1 ||
        missing_helper_diagnostics.entries.size() != 1 ||
        missing_helper_diagnostics.entries.front().kind !=
            aarch64_module::ModuleLoweringDiagnosticKind::MissingValueAuthority ||
        missing_helper_diagnostics.entries.front().message.find(
            "missing_prepared_f128_runtime_helper") == std::string::npos) {
      return fail("expected unsupported f128 helper family to require prepared helper authority");
    }

    auto guessed_helper_prepared =
        prepared_with_f128_runtime_helper_operation(opcode);
    const auto& guessed_function_cf =
        guessed_helper_prepared.control_flow.functions.front();
    const auto& guessed_block_cf = guessed_function_cf.blocks.front();
    const auto guessed_function_context =
        aarch64_codegen::make_function_lowering_context(
            guessed_helper_prepared, guessed_helper_prepared.target_profile,
            guessed_function_cf);
    const auto guessed_block_context =
        aarch64_codegen::make_block_lowering_context(
            guessed_function_context, guessed_block_cf, 0);

    aarch64_module::MachineBlock guessed_block;
    aarch64_module::ModuleLoweringDiagnostics guessed_diagnostics;
    const auto guessed_result =
        aarch64_codegen::dispatch_prepared_block(
            guessed_block_context, guessed_block, guessed_diagnostics);
    const auto* selected_helper =
        guessed_block.instructions.empty()
            ? nullptr
            : std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
                  &guessed_block.instructions.front().target.payload);
    if (guessed_result.visited_operations != 1 ||
        guessed_result.emitted_instructions != 1 ||
        guessed_block.instructions.size() != 1 ||
        selected_helper != nullptr ||
        guessed_diagnostics.entries.size() != 1 ||
        guessed_diagnostics.entries.front().message.find(
            "unsupported_source_operation") == std::string::npos) {
      return fail("expected unsupported f128 helper family to reject dispatch-side helper guesses");
    }
  }
  return 0;
}

int block_dispatch_lowers_f128_compare_helper_boundary_into_i1_result() {
  auto prepared = prepared_with_f128_comparison_helper_operation();
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

  const auto* helper =
      block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
                &block.instructions.front().target.payload);
  if (!diagnostics.empty() ||
      result.visited_operations != 1 ||
      result.emitted_instructions != 2 ||
      block.instructions.size() != 2 ||
      helper == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Comparison ||
      helper->result_type != bir::TypeKind::I32 ||
      helper->result_value_name != prepared.names.value_names.find("%cmp.result") ||
      helper->scalar_result.type != bir::TypeKind::I32 ||
      helper->scalar_result.abi_register->reg != aarch64_abi::w_register(0) ||
      helper->scalar_result.materialized_i1_register->reg != aarch64_abi::w_register(9) ||
      !helper->scalar_result.cmp_result_consumption.has_value() ||
      helper->scalar_result.cmp_result_consumption->bir_result_type != bir::TypeKind::I1 ||
      helper->scalar_result.cmp_result_consumption->zero_test !=
          prepare::PreparedF128CmpResultZeroTest::EqualZero ||
      !helper->scalar_result.cmp_result_consumption->consumes_helper_cmp_result ||
      !helper->scalar_result.cmp_result_consumption->owns_bir_i1_result ||
      block.instructions.front().target.defs.size() != 3 ||
      block.instructions.front().target.uses.size() != 5) {
    return fail("expected f128 compare dispatch to consume CMPtype into BIR I1 result");
  }

  auto unsupported = prepared_with_f128_comparison_helper_operation(bir::BinaryOpcode::Ult);
  auto& unsupported_helper =
      unsupported.f128_runtime_helpers.functions.front().helpers.front();
  unsupported_helper.source_binary_opcode = bir::BinaryOpcode::Ult;
  const auto& unsupported_function_cf = unsupported.control_flow.functions.front();
  const auto& unsupported_block_cf = unsupported_function_cf.blocks.front();
  const auto unsupported_function_context =
      aarch64_codegen::make_function_lowering_context(
          unsupported, unsupported.target_profile, unsupported_function_cf);
  const auto unsupported_block_context =
      aarch64_codegen::make_block_lowering_context(
          unsupported_function_context, unsupported_block_cf, 0);
  aarch64_module::MachineBlock unsupported_block;
  aarch64_module::ModuleLoweringDiagnostics unsupported_diagnostics;
  const auto unsupported_result =
      aarch64_codegen::dispatch_prepared_block(
          unsupported_block_context, unsupported_block, unsupported_diagnostics);
  if (unsupported_result.visited_operations != 1 ||
      unsupported_result.emitted_instructions != 1 ||
      unsupported_block.instructions.size() != 1 ||
      unsupported_diagnostics.entries.size() != 1 ||
      unsupported_diagnostics.entries.front().message.find(
          "unsupported_source_operation") ==
          std::string::npos) {
    return fail("expected unmodeled f128 compare predicate to fail closed");
  }
  return 0;
}

int block_dispatch_lowers_f128_cast_helper_from_prepared_authority() {
  auto prepared = prepared_with_f128_cast_helper_operation();
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

  const auto* helper =
      block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
                &block.instructions.front().target.payload);
  if (!diagnostics.empty() ||
      result.visited_operations != 1 ||
      result.emitted_instructions != 2 ||
      block.instructions.size() != 2 ||
      helper == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Cast ||
      helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::F64ToF128 ||
      helper->boundary_kind != aarch64_codegen::F128RuntimeHelperBoundaryKind::F64ToF128 ||
      helper->source_cast_opcode != bir::CastOpcode::FPExt ||
      helper->callee_name != "__extenddftf2" ||
      helper->source_type != bir::TypeKind::F64 ||
      helper->result_type != bir::TypeKind::F128 ||
      helper->scalar_operand.type != bir::TypeKind::F64 ||
      helper->scalar_operand.abi_register->reg != aarch64_abi::d_register(0) ||
      helper->result.abi_register->reg != aarch64_abi::q_register(0) ||
      helper->abi_policy.transition !=
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result ||
      !helper->selected_call_ownership.owns_terminal_call ||
      block.instructions.front().target.defs.size() != 2 ||
      block.instructions.front().target.uses.size() != 2) {
    return fail("expected dispatch to select f64->f128 helper boundary from prepared authority");
  }

  auto trunc_prepared = prepared_with_f128_cast_helper_operation(
      bir::CastOpcode::FPTrunc, bir::TypeKind::F128, bir::TypeKind::F32);
  const auto& trunc_function_cf = trunc_prepared.control_flow.functions.front();
  const auto& trunc_block_cf = trunc_function_cf.blocks.front();
  const auto trunc_function_context = aarch64_codegen::make_function_lowering_context(
      trunc_prepared, trunc_prepared.target_profile, trunc_function_cf);
  const auto trunc_block_context =
      aarch64_codegen::make_block_lowering_context(trunc_function_context, trunc_block_cf, 0);
  aarch64_module::MachineBlock trunc_block;
  aarch64_module::ModuleLoweringDiagnostics trunc_diagnostics;
  const auto trunc_result =
      aarch64_codegen::dispatch_prepared_block(
          trunc_block_context, trunc_block, trunc_diagnostics);
  const auto* trunc_helper =
      trunc_block.instructions.empty()
          ? nullptr
          : std::get_if<aarch64_codegen::F128RuntimeHelperBoundaryRecord>(
                &trunc_block.instructions.front().target.payload);
  if (!trunc_diagnostics.empty() ||
      trunc_result.visited_operations != 1 ||
      trunc_result.emitted_instructions != 2 ||
      trunc_block.instructions.size() != 2 ||
      trunc_helper == nullptr ||
      trunc_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::F128ToF32 ||
      trunc_helper->boundary_kind != aarch64_codegen::F128RuntimeHelperBoundaryKind::F128ToF32 ||
      trunc_helper->source_cast_opcode != bir::CastOpcode::FPTrunc ||
      trunc_helper->callee_name != "__trunctfsf2" ||
      trunc_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue ||
      trunc_helper->lhs.abi_register->reg != aarch64_abi::q_register(0) ||
      trunc_helper->scalar_result.abi_register->reg != aarch64_abi::s_register(0) ||
      trunc_helper->abi_policy.transition !=
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult) {
    return fail("expected dispatch to select f128->f32 helper boundary from prepared authority");
  }

  auto missing = prepared_with_f128_cast_helper_operation();
  missing.f128_runtime_helpers.functions.clear();
  const auto& missing_function_cf = missing.control_flow.functions.front();
  const auto& missing_block_cf = missing_function_cf.blocks.front();
  const auto missing_function_context = aarch64_codegen::make_function_lowering_context(
      missing, missing.target_profile, missing_function_cf);
  const auto missing_block_context =
      aarch64_codegen::make_block_lowering_context(missing_function_context, missing_block_cf, 0);
  aarch64_module::MachineBlock missing_block;
  aarch64_module::ModuleLoweringDiagnostics missing_diagnostics;
  const auto missing_result =
      aarch64_codegen::dispatch_prepared_block(
          missing_block_context, missing_block, missing_diagnostics);
  if (missing_result.visited_operations != 1 ||
      missing_result.emitted_instructions != 1 ||
      missing_block.instructions.size() != 1 ||
      missing_diagnostics.entries.size() != 1 ||
      missing_diagnostics.entries.front().message.find(
          "missing_prepared_f128_runtime_helper") == std::string::npos) {
    return fail("expected f128 cast dispatch to fail closed without prepared helper authority");
  }
  return 0;
}

int block_dispatch_lowers_i128_shift_from_prepared_carriers() {
  auto prepared = prepared_with_i128_shift_operation();
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

  const auto* shift =
      !block.instructions.empty()
          ? std::get_if<aarch64_codegen::I128ShiftRecord>(
                &block.instructions.front().target.payload)
          : nullptr;
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || shift == nullptr ||
      block.instructions.front().target.opcode != aarch64_codegen::MachineOpcode::I128Shift ||
      shift->shift_kind != aarch64_codegen::I128ShiftKind::LogicalRight ||
      shift->lane_semantics !=
          aarch64_codegen::I128ShiftLaneSemantics::CrossLaneLogicalRight ||
      shift->count_kind != aarch64_codegen::I128ShiftCountKind::Immediate ||
      shift->result.value_id != prepare::PreparedValueId{130} ||
      shift->source.value_id != prepare::PreparedValueId{131} ||
      block.instructions.front().target.defs.size() != 2 ||
      block.instructions.front().target.uses.size() != 3) {
    return fail("expected dispatch to select i128 logical shift from prepared carriers");
  }
  return 0;
}

int block_dispatch_lowers_i128_compare_from_prepared_carriers() {
  auto prepared = prepared_with_i128_compare_operation();
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

  const auto* compare =
      !block.instructions.empty()
          ? std::get_if<aarch64_codegen::I128CompareRecord>(
                &block.instructions.front().target.payload)
          : nullptr;
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || compare == nullptr ||
      block.instructions.front().target.opcode != aarch64_codegen::MachineOpcode::I128Compare ||
      compare->signedness != aarch64_codegen::I128CompareSignedness::Unsigned ||
      compare->high_word_semantics !=
          aarch64_codegen::I128CompareHighWordSemantics::UnsignedHighWordFirst ||
      !compare->result_register.has_value() ||
      compare->result_register->reg != aarch64_abi::w_register(3) ||
      compare->lhs.value_id != prepare::PreparedValueId{141} ||
      compare->rhs.value_id != prepare::PreparedValueId{142} ||
      block.instructions.front().target.defs.size() != 1 ||
      block.instructions.front().target.uses.size() != 4) {
    return fail("expected dispatch to select i128 unsigned compare from prepared facts");
  }
  return 0;
}

int block_dispatch_reports_missing_i128_compare_result_authority() {
  auto prepared = prepared_with_i128_compare_operation(false);
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
  if (diagnostics.entries.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      diagnostics.entries.front().message.find("missing_scalar_result_value_home") ==
          std::string::npos) {
    return fail("expected i128 compare dispatch to fail closed without scalar result authority");
  }
  return 0;
}

int block_dispatch_reports_missing_frame_slot_load_destination_authority() {
  auto prepared = prepared_with_frame_slot_load(false);
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

  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.size() != 1) {
    return fail("expected missing load destination authority diagnostic only");
  }
  const auto& diagnostic = diagnostics.entries.front();
  if (diagnostic.instruction_family !=
          aarch64_module::InstructionLoweringFamily::Memory ||
      diagnostic.instruction_index != 0 ||
      diagnostic.message.find("missing_result_storage") == std::string::npos) {
    return fail("expected explicit missing_result_storage memory diagnostic");
  }
  if (!std::holds_alternative<aarch64_codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected missing load authority to keep terminal return lowering");
  }

  return 0;
}

int block_dispatch_lowers_prepared_simple_integer_cast_with_result_register() {
  auto prepared = prepared_with_simple_integer_cast();
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected dispatch to select simple cast plus return");
  }
  const auto* scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(
          &block.instructions.front().target.payload);
  if (scalar == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !scalar->scalar_cast.has_value() || scalar->scalar_alu.has_value() ||
      scalar->source_cast_opcode != bir::CastOpcode::SExt ||
      scalar->result_value_id != prepare::PreparedValueId{21} ||
      !scalar->result_register.has_value() ||
      scalar->result_register->occupied_registers.empty() ||
      scalar->result_register->occupied_registers.front() != "x0") {
    return fail("expected selected simple cast to preserve destination register facts");
  }
  const auto* source =
      std::get_if<aarch64_codegen::RegisterOperand>(
          &scalar->scalar_cast->source.payload);
  if (source == nullptr || source->value_id != prepare::PreparedValueId{20} ||
      source->occupied_registers.empty() || source->occupied_registers.front() != "w1" ||
      block.instructions.front().target.defs.empty() ||
      block.instructions.front().target.defs.front().kind !=
          aarch64_codegen::MachineEffectResourceKind::Register) {
    return fail("expected selected simple cast to preserve source register and def facts");
  }

  return 0;
}

int block_dispatch_defers_unsupported_casts_and_missing_cast_register_facts() {
  {
    auto prepared = prepared_with_simple_integer_cast(
        bir::CastOpcode::FPExt, bir::TypeKind::F32, bir::TypeKind::F64);
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
    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
        diagnostics.entries.front().instruction_family !=
            aarch64_module::InstructionLoweringFamily::Scalar) {
      return fail("expected unsupported FP cast to stay fail-closed in dispatch");
    }
  }

  {
    auto prepared = prepared_with_simple_integer_cast(
        bir::CastOpcode::SExt, bir::TypeKind::I32, bir::TypeKind::I64, false);
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
    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
        diagnostics.entries.front().instruction_family !=
            aarch64_module::InstructionLoweringFamily::Scalar) {
      return fail("expected missing cast result storage to stay fail-closed in dispatch");
    }
  }

  return 0;
}

int block_dispatch_lowers_prepared_f64_scalar_alu_with_fpr_registers() {
  auto prepared = prepared_with_f64_scalar_alu();
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

  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected dispatch to select F64 scalar ALU plus return");
  }
  const auto* scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(
          &block.instructions.front().target.payload);
  if (scalar == nullptr ||
      block.instructions.front().target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !scalar->scalar_alu.has_value() || scalar->source_binary_opcode != bir::BinaryOpcode::Mul ||
      scalar->scalar_alu->operation != aarch64_codegen::ScalarAluOperationKind::Mul ||
      !scalar->scalar_alu->supported_floating_operation ||
      scalar->scalar_alu->supported_integer_operation ||
      !scalar->result_register.has_value() ||
      scalar->result_register->reg != aarch64_abi::d_register(0)) {
    return fail("expected selected F64 scalar ALU to preserve result FPR facts");
  }
  const auto* lhs =
      std::get_if<aarch64_codegen::RegisterOperand>(&scalar->scalar_alu->lhs.payload);
  const auto* rhs =
      std::get_if<aarch64_codegen::RegisterOperand>(&scalar->scalar_alu->rhs.payload);
  if (lhs == nullptr || rhs == nullptr || lhs->reg != aarch64_abi::d_register(1) ||
      rhs->reg != aarch64_abi::d_register(2) ||
      block.instructions.front().target.defs.front().reg != aarch64_abi::d_register(0)) {
    return fail("expected selected F64 scalar ALU to preserve source FPR facts");
  }
  return 0;
}

int block_dispatch_defers_floating_scalar_alu_missing_fpr_facts() {
  auto prepared = prepared_with_f64_scalar_alu(bir::BinaryOpcode::Add, false);
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
  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.empty()) {
    return fail("expected F64 scalar ALU without FPR facts to stay fail-closed");
  }
  bool saw_scalar_diagnostic = false;
  for (const auto& diagnostic : diagnostics.entries) {
    saw_scalar_diagnostic =
        saw_scalar_diagnostic ||
        diagnostic.instruction_family == aarch64_module::InstructionLoweringFamily::Scalar;
  }
  if (!saw_scalar_diagnostic) {
    return fail("expected F64 scalar ALU without FPR facts to stay fail-closed");
  }
  return 0;
}

int block_dispatch_lowers_prepared_conversion_cast_with_bank_transition_facts() {
  auto prepared = prepared_with_conversion_cast();
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
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected dispatch to select conversion cast plus return");
  }
  const auto* scalar =
      std::get_if<aarch64_codegen::ScalarInstructionRecord>(
          &block.instructions.front().target.payload);
  if (scalar == nullptr || !scalar->scalar_cast.has_value() ||
      scalar->scalar_cast->operation !=
          aarch64_codegen::ScalarCastOperationKind::SignedIntToFloat ||
      !scalar->scalar_cast->supported_float_integer_conversion ||
      !scalar->scalar_cast->crosses_register_bank ||
      !scalar->result_register.has_value() ||
      scalar->result_register->reg != aarch64_abi::d_register(0)) {
    return fail("expected selected conversion cast to preserve result bank transition facts");
  }
  const auto* source =
      std::get_if<aarch64_codegen::RegisterOperand>(&scalar->scalar_cast->source.payload);
  if (source == nullptr || source->reg != aarch64_abi::w_register(1) ||
      scalar->scalar_cast->source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      scalar->scalar_cast->result_register_bank != prepare::PreparedRegisterBank::Fpr) {
    return fail("expected selected conversion cast to preserve source bank transition facts");
  }
  return 0;
}

int block_dispatch_defers_conversion_cast_missing_bank_facts() {
  auto prepared = prepared_with_conversion_cast(
      bir::CastOpcode::SIToFP, bir::TypeKind::I32, bir::TypeKind::F64, false);
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
  if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
      block.instructions.size() != 1 || diagnostics.entries.empty()) {
    return fail("expected conversion cast without bank authority to stay fail-closed");
  }
  return 0;
}

int block_dispatch_lowers_prepared_frame_slot_and_pointer_value_stores() {
  for (const auto kind :
       {StoreDispatchFixtureKind::FrameSlot,
        StoreDispatchFixtureKind::PointerValue,
        StoreDispatchFixtureKind::GlobalSymbol}) {
    auto prepared = prepared_with_store(kind);
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

    if (!diagnostics.empty() || result.visited_operations != 1 ||
        result.emitted_instructions != 2 || block.instructions.size() != 2) {
      return fail("expected dispatch to select store plus return");
    }

    const auto* memory =
        std::get_if<aarch64_codegen::MemoryInstructionRecord>(
            &block.instructions.front().target.payload);
    if (memory == nullptr ||
        block.instructions.front().target.selection.status !=
            aarch64_codegen::MachineNodeSelectionStatus::Selected ||
        memory->memory_kind != aarch64_codegen::MemoryInstructionKind::Store ||
        !memory->value.has_value() ||
        memory->value->kind != aarch64_codegen::OperandKind::Register ||
        memory->address.stored_value_id != prepare::PreparedValueId{11} ||
        memory->address.stored_value_name !=
            prepared.names.value_names.find("%stored")) {
      return fail("expected selected store to preserve source identity");
    }
    const auto* stored_register =
        std::get_if<aarch64_codegen::RegisterOperand>(&memory->value->payload);
    if (stored_register == nullptr ||
        stored_register->value_id != prepare::PreparedValueId{11} ||
        stored_register->occupied_registers.empty() ||
        stored_register->occupied_registers.front() != "w1") {
      return fail("expected selected store to carry structured source register");
    }
    if (kind == StoreDispatchFixtureKind::FrameSlot) {
      if (memory->address.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
          memory->address.frame_slot_id != prepare::PreparedFrameSlotId{21} ||
          memory->address.byte_offset != 12 ||
          memory->address.address_space != bir::AddressSpace::Fs ||
          !memory->address.is_volatile) {
        return fail("expected frame-slot store address facts");
      }
    } else if (kind == StoreDispatchFixtureKind::PointerValue) {
      if (memory->address.base_kind !=
              aarch64_codegen::MemoryBaseKind::PointerValue ||
          memory->address.pointer_value_id != prepare::PreparedValueId{12} ||
          memory->address.byte_offset != 24 ||
          memory->address.address_space != bir::AddressSpace::Tls ||
          memory->address.is_volatile) {
        return fail("expected pointer-value store address facts");
      }
    } else if (memory->address.base_kind != aarch64_codegen::MemoryBaseKind::Symbol ||
               memory->address.symbol_label != "g.store" ||
               memory->address.byte_offset != 16 ||
               memory->address.address_space != bir::AddressSpace::Gs ||
               !memory->address.is_volatile) {
      return fail("expected global-symbol store address facts");
    }
    if (block.instructions.front().target.side_effects.empty() ||
        block.instructions.front().target.side_effects.front() !=
            aarch64_codegen::MachineSideEffectKind::MemoryWrite ||
        block.instructions.front().target.uses.size() < 2 ||
        block.instructions.front().target.uses.back().kind !=
            aarch64_codegen::MachineEffectResourceKind::Register) {
      return fail("expected store target to expose register use and memory write");
    }
  }
  return 0;
}

int block_dispatch_reports_missing_store_source_authority_and_unsupported_base() {
  {
    auto prepared = prepared_with_store(StoreDispatchFixtureKind::FrameSlot, false);
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
    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
        diagnostics.entries.front().message.find("missing_stored_storage") ==
            std::string::npos) {
      return fail("expected missing stored-source storage diagnostic");
    }
  }

  {
    auto prepared = prepared_with_store(StoreDispatchFixtureKind::StringConstant);
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
    if (result.visited_operations != 1 || result.emitted_instructions != 1 ||
        block.instructions.size() != 1 || diagnostics.entries.size() != 1 ||
        diagnostics.entries.front().message.find("unsupported_base") == std::string::npos) {
      return fail("expected unsupported string-constant store diagnostic");
    }
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
          block_dispatch_selects_complete_inline_asm_machine_record();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_selects_alias_aware_inline_asm_tied_home();
      status != 0) {
    return status;
  }
  if (const int status = block_dispatch_selects_inline_asm_structured_clobbers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_selects_inline_asm_prepared_memory_and_address_operands();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_selects_named_inline_asm_machine_record();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_malformed_inline_asm_carriers_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_selects_ordered_atomic_load_store_and_fence_records();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_rejects_incomplete_and_deferred_atomic_carriers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_frame_slot_load_with_result_register();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_i128_frame_slot_load_from_prepared_carrier();
      status != 0) {
    return status;
  }
  if (const int status = block_dispatch_reports_missing_i128_carrier_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_frame_slot_load_from_prepared_carrier();
      status != 0) {
    return status;
  }
  if (const int status = block_dispatch_reports_missing_f128_carrier_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_frame_slot_store_from_prepared_carrier();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_prints_representative_f128_load_helper_store_route();
      status != 0) {
    return status;
  }
  if (const int status = block_dispatch_reports_incomplete_f128_carrier_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_i128_pair_add_from_prepared_carriers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_i128_pair_bitwise_from_prepared_carriers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_i128_copy_from_prepared_carriers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_missing_i128_pair_carrier_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_i128_runtime_helper_from_prepared_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_missing_i128_runtime_helper_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_runtime_helper_from_prepared_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_runtime_helper_from_prepared_authority(
              bir::BinaryOpcode::Sub,
              prepare::PreparedF128RuntimeHelperKind::Sub,
              "__subtf3");
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_runtime_helper_from_prepared_authority(
              bir::BinaryOpcode::Mul,
              prepare::PreparedF128RuntimeHelperKind::Mul,
              "__multf3");
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_runtime_helper_from_prepared_authority(
              bir::BinaryOpcode::SDiv,
              prepare::PreparedF128RuntimeHelperKind::Div,
              "__divtf3");
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_missing_f128_runtime_helper_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_f128_sign_bit_candidate_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_unsupported_f128_helper_family_diagnostics();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_compare_helper_boundary_into_i1_result();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_f128_cast_helper_from_prepared_authority();
      status != 0) {
    return status;
  }
  if (const int status = block_dispatch_lowers_i128_shift_from_prepared_carriers();
      status != 0) {
    return status;
  }
  if (const int status = block_dispatch_lowers_i128_compare_from_prepared_carriers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_missing_i128_compare_result_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_missing_frame_slot_load_destination_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_simple_integer_cast_with_result_register();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_defers_unsupported_casts_and_missing_cast_register_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_f64_scalar_alu_with_fpr_registers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_defers_floating_scalar_alu_missing_fpr_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_conversion_cast_with_bank_transition_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_defers_conversion_cast_missing_bank_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_frame_slot_and_pointer_value_stores();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_reports_missing_store_source_authority_and_unsupported_base();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_direct_call_without_reclassifying_abi();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_intrinsic_spelling_without_carrier_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          dynamic_stack_helper_calls_missing_prepared_authority_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          dynamic_stack_supported_helper_calls_lower_to_target_owned_output();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_selects_complete_scalar_fp_unary_fabs_intrinsic_carrier();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_selects_complete_crc_vector_intrinsic_carriers();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_complete_barrier_dmb_carrier_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_complete_cache_dc_cvau_carrier_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_complete_hint_yield_carrier_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_incomplete_scalar_fp_unary_intrinsic_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_keeps_f128_fabs_intrinsic_carrier_fail_closed();
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
          semantic_symbol_address_argument_avoids_deferred_call_boundary_move();
      status != 0) {
    return status;
  }
  if (const int status =
          semantic_stack_call_argument_publishes_destination_offset();
      status != 0) {
    return status;
  }
  if (const int status =
          semantic_stack_call_argument_lowers_aggregate_address_stack_copy();
      status != 0) {
    return status;
  }
  if (const int status =
          prepared_frame_slot_stack_call_argument_lowers_to_selected_store();
      status != 0) {
    return status;
  }
  if (const int status =
          stack_home_symbol_address_argument_materializes_directly_to_call_register();
      status != 0) {
    return status;
  }
  if (const int status =
          load_global_call_argument_uses_got_for_got_required_global();
      status != 0) {
    return status;
  }
  if (const int status =
          load_global_call_argument_keeps_direct_for_direct_global();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_register_result_move_after_direct_call();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_f128_argument_q_register_move_before_direct_call();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_exposes_f128_constant_argument_carrier_to_selection();
      status != 0) {
    return status;
  }
  if (const int status =
          semantic_f128_constant_argument_reaches_call_boundary_selection();
      status != 0) {
    return status;
  }
  if (const int status =
          semantic_f128_constant_helper_operand_reaches_carrier_and_fails_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_rejects_incomplete_f128_constant_argument_carriers();
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
