#include "src/backend/mir/aarch64/codegen/cast_ops.hpp"
#include "src/backend/mir/aarch64/codegen/machine_printer.hpp"
#include "src/backend/mir/aarch64/codegen/returns.hpp"
#include "src/backend/mir/printer.hpp"

#include <iostream>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

int fail(const std::string& message) {
  std::cerr << message << "\n";
  return 1;
}

int expect_equal(std::string_view actual, std::string_view expected, std::string_view context) {
  if (actual != expected) {
    return fail(std::string(context) + " expected '" + std::string(expected) + "', got '" +
                std::string(actual) + "'");
  }
  return 0;
}

std::string join_lines(const std::vector<std::string>& lines) {
  std::string joined;
  for (const auto& line : lines) {
    if (!joined.empty()) {
      joined += "\\n";
    }
    joined += line;
  }
  return joined;
}

int expect_assembly(std::string_view actual,
                    std::string_view expected_from_helpers,
                    std::string_view expected_canonical,
                    std::string_view context) {
  if (expected_from_helpers != expected_canonical) {
    return fail(std::string(context) +
                " helper-derived assembly drifted from canonical expected spelling");
  }
  return expect_equal(actual, expected_from_helpers, context);
}

mir::MachinePrintResult print_common_instruction_nodes(
    std::initializer_list<aarch64_codegen::InstructionRecord> instructions) {
  mir::MachineFunction<aarch64_codegen::InstructionRecord> function;
  function.function_name = c4c::FunctionNameId{2};
  auto& block = function.blocks.emplace_back();
  block.block_label = c4c::BlockLabelId{3};
  block.index = 0;
  for (const auto& instruction : instructions) {
    block.instructions.push_back(
        mir::MachineInstruction<aarch64_codegen::InstructionRecord>{.target = instruction});
  }

  const auto target_printer = aarch64_codegen::MachineInstructionPrinter{};
  return mir::print_machine_function(function, target_printer);
}

aarch64_codegen::RegisterOperand xreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::x_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::SpillAuthority,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::X,
      .contiguous_width = 1,
  };
}

aarch64_codegen::RegisterOperand wreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::w_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = aarch64_abi::RegisterView::W,
      .contiguous_width = 1,
  };
}

aarch64_codegen::RegisterOperand sreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::s_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .prepared_class = prepare::PreparedRegisterClass::Float,
      .prepared_bank = prepare::PreparedRegisterBank::Fpr,
      .expected_view = aarch64_abi::RegisterView::S,
      .contiguous_width = 1,
  };
}

aarch64_codegen::RegisterOperand dreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::d_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .prepared_class = prepare::PreparedRegisterClass::Float,
      .prepared_bank = prepare::PreparedRegisterBank::Fpr,
      .expected_view = aarch64_abi::RegisterView::D,
      .contiguous_width = 1,
  };
}

aarch64_codegen::RegisterOperand qreg(unsigned index) {
  return aarch64_codegen::RegisterOperand{
      .reg = aarch64_abi::q_register(static_cast<std::uint8_t>(index)),
      .role = aarch64_codegen::RegisterOperandRole::PreparedAssignment,
      .prepared_class = prepare::PreparedRegisterClass::Vector,
      .prepared_bank = prepare::PreparedRegisterBank::Vreg,
      .expected_view = aarch64_abi::RegisterView::Q,
      .contiguous_width = 1,
  };
}

aarch64_codegen::InstructionRecord selected_inline_asm_instruction(
    aarch64_codegen::AssemblerInstructionRecord record) {
  auto instruction = aarch64_codegen::make_assembler_instruction(std::move(record));
  instruction.surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode;
  instruction.selection =
      aarch64_codegen::MachineNodeStatusRecord{
          .status = aarch64_codegen::MachineNodeSelectionStatus::Selected};
  return instruction;
}

prepare::PreparedTargetRegisterIdentity gpr_identity(std::size_t physical_index) {
  return prepare::PreparedTargetRegisterIdentity{
      .target_arch = c4c::TargetArch::Aarch64,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .physical_index = physical_index,
  };
}

prepare::PreparedSavedRegister prepared_saved_x19(std::size_t offset_bytes) {
  return prepare::PreparedSavedRegister{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "x19",
      .contiguous_width = 1,
      .occupied_register_names = {"x19"},
      .save_index = 0,
      .placement = prepare::PreparedRegisterPlacement{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
          .slot_index = 0,
          .contiguous_width = 1,
      },
      .slot_placement =
          prepare::PreparedSavedRegisterSlotPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = "x19",
              .contiguous_width = 1,
              .occupied_register_names = {"x19"},
              .save_index = 0,
              .register_placement = prepare::PreparedRegisterPlacement{
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                  .slot_index = 0,
                  .contiguous_width = 1,
              },
              .slot_id = prepare::PreparedFrameSlotId{19},
              .stack_offset_bytes = offset_bytes,
              .size_bytes = std::size_t{8},
              .align_bytes = std::size_t{8},
              .fixed_location = true,
          },
  };
}

aarch64_codegen::AssemblerInstructionRecord selected_inline_asm_record(
    std::string templ = "add %w0, %w1, %w2\nmov %x0, #%3") {
  auto output = wreg(3);
  output.role = aarch64_codegen::RegisterOperandRole::ValueHome;
  output.value_id = prepare::PreparedValueId{50};
  output.value_name = c4c::ValueNameId{50};
  auto tied = wreg(3);
  tied.role = aarch64_codegen::RegisterOperandRole::ValueHome;
  tied.value_id = prepare::PreparedValueId{51};
  tied.value_name = c4c::ValueNameId{51};
  auto input = wreg(5);
  input.role = aarch64_codegen::RegisterOperandRole::ValueHome;
  input.value_id = prepare::PreparedValueId{52};
  input.value_name = c4c::ValueNameId{52};

  const prepare::PreparedValueHome output_home{
      .value_id = prepare::PreparedValueId{50},
      .value_name = c4c::ValueNameId{50},
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"w3"},
      .target_register_identity = gpr_identity(3),
  };
  const prepare::PreparedValueHome tied_home{
      .value_id = prepare::PreparedValueId{51},
      .value_name = c4c::ValueNameId{51},
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"w3"},
      .target_register_identity = gpr_identity(3),
  };
  const prepare::PreparedValueHome input_home{
      .value_id = prepare::PreparedValueId{52},
      .value_name = c4c::ValueNameId{52},
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"w5"},
      .target_register_identity = gpr_identity(5),
  };

  const auto immediate = aarch64_codegen::make_immediate_operand(
      aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::SignedInteger,
          .type = bir::TypeKind::I32,
          .signed_value = 7,
          .unsigned_value = 7,
          .source_value_id = prepare::PreparedValueId{53},
          .source_value_name = c4c::ValueNameId{53},
      });

  return aarch64_codegen::AssemblerInstructionRecord{
      .operands =
          {aarch64_codegen::make_register_operand(output),
           aarch64_codegen::make_register_operand(tied),
           aarch64_codegen::make_register_operand(input),
           immediate},
      .has_inline_asm_payload = true,
      .side_effects = true,
      .inline_asm_template = std::move(templ),
      .inline_asm_constraints = "=r,0,r,i",
      .inline_asm_has_template_modifiers = true,
      .inline_asm_operands =
          {aarch64_codegen::InlineAsmMachineOperandRecord{
               .kind = bir::InlineAsmOperandKind::RegisterOutput,
               .constraint_index = 0,
               .constraint = "=r",
               .output_index = std::size_t{0},
               .name = std::string{"dst"},
               .home = output_home,
               .selected_operand = aarch64_codegen::make_register_operand(output),
           },
           aarch64_codegen::InlineAsmMachineOperandRecord{
               .kind = bir::InlineAsmOperandKind::TiedInput,
               .constraint_index = 1,
               .constraint = "0",
               .arg_index = std::size_t{0},
               .tied_output_index = std::size_t{0},
               .name = std::string{"seed"},
               .home = tied_home,
               .selected_operand = aarch64_codegen::make_register_operand(tied),
           },
           aarch64_codegen::InlineAsmMachineOperandRecord{
               .kind = bir::InlineAsmOperandKind::RegisterInput,
               .constraint_index = 2,
               .constraint = "r",
               .arg_index = std::size_t{1},
               .name = std::string{"rhs"},
               .home = input_home,
               .selected_operand = aarch64_codegen::make_register_operand(input),
           },
           aarch64_codegen::InlineAsmMachineOperandRecord{
               .kind = bir::InlineAsmOperandKind::IntegerImmediateInput,
               .constraint_index = 3,
               .constraint = "i",
               .arg_index = std::size_t{2},
               .name = std::string{"imm"},
               .immediate_value = std::int64_t{7},
               .selected_operand = immediate,
           }},
      .inline_asm_result_home = output_home,
  };
}

const prepare::PreparedAtomicOperationCarrier* complete_atomic_carrier() {
  static const prepare::PreparedAtomicOperationCarrier carrier{
      .carrier_kind = prepare::PreparedAtomicOperationCarrierKind::Complete,
  };
  return &carrier;
}

const prepare::PreparedIntrinsicCarrier* complete_scalar_fp_unary_fabs_carrier(
    bir::TypeKind type) {
  static const prepare::PreparedIntrinsicCarrier f32_carrier{
      .function_name = c4c::FunctionNameId{2},
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
      .operation = bir::IntrinsicOperationKind::FAbs,
      .operand_type = bir::TypeKind::F32,
      .result_type = bir::TypeKind::F32,
      .operand = bir::Value::named(bir::TypeKind::F32, "%arg"),
      .result = bir::Value::named(bir::TypeKind::F32, "%fabs"),
      .operand_value_name = c4c::ValueNameId{82},
      .result_value_name = c4c::ValueNameId{83},
      .source_callee_name = std::string{"llvm.fabs.float"},
      .has_prepared_call_plan = true,
  };
  static const prepare::PreparedIntrinsicCarrier f64_carrier{
      .function_name = c4c::FunctionNameId{2},
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
      .operation = bir::IntrinsicOperationKind::FAbs,
      .operand_type = bir::TypeKind::F64,
      .result_type = bir::TypeKind::F64,
      .operand = bir::Value::named(bir::TypeKind::F64, "%arg"),
      .result = bir::Value::named(bir::TypeKind::F64, "%fabs"),
      .operand_value_name = c4c::ValueNameId{84},
      .result_value_name = c4c::ValueNameId{85},
      .source_callee_name = std::string{"llvm.fabs.double"},
      .has_prepared_call_plan = true,
  };
  return type == bir::TypeKind::F32 ? &f32_carrier : &f64_carrier;
}

const prepare::PreparedIntrinsicCarrier* complete_crc32w_carrier() {
  static const prepare::PreparedIntrinsicCarrier carrier{
      .function_name = c4c::FunctionNameId{2},
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::Crc,
      .operation = bir::IntrinsicOperationKind::Crc32W,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Crc,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::I32,
      .operand_roles = {bir::IntrinsicOperandRole::Accumulator,
                        bir::IntrinsicOperandRole::Data},
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.crc32w"},
      .has_prepared_call_plan = true,
  };
  return &carrier;
}

const prepare::PreparedIntrinsicCarrier* complete_vector_load_carrier() {
  static const prepare::PreparedIntrinsicCarrier carrier{
      .function_name = c4c::FunctionNameId{2},
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::VectorMemory,
      .operation = bir::IntrinsicOperationKind::VectorLoad,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
      .operand_type = bir::TypeKind::Ptr,
      .result_type = bir::TypeKind::I128,
      .operand_roles = {bir::IntrinsicOperandRole::Pointer},
      .vector_element_type = bir::TypeKind::I8,
      .vector_element_width_bytes = 1,
      .vector_lane_count = 16,
      .vector_total_width_bytes = 16,
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .memory_access = bir::IntrinsicMemoryAccessKind::Read,
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.neon.ld1.v16i8.p0i8"},
      .has_prepared_call_plan = true,
  };
  return &carrier;
}

const prepare::PreparedIntrinsicCarrier* complete_vector_add_carrier() {
  static const prepare::PreparedIntrinsicCarrier carrier{
      .function_name = c4c::FunctionNameId{2},
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
      .family = bir::IntrinsicFamilyKind::VectorOperation,
      .operation = bir::IntrinsicOperationKind::VectorAdd,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
      .operand_type = bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .operand_roles = {bir::IntrinsicOperandRole::VectorLhs,
                        bir::IntrinsicOperandRole::VectorRhs},
      .vector_element_type = bir::TypeKind::I8,
      .vector_element_width_bytes = 1,
      .vector_lane_count = 16,
      .vector_total_width_bytes = 16,
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.neon.add.v16i8"},
      .has_prepared_call_plan = true,
  };
  return &carrier;
}

const prepare::PreparedIntrinsicCarrier* complete_barrier_dmb_carrier() {
  static const prepare::PreparedIntrinsicCarrier carrier{
      .function_name = c4c::FunctionNameId{2},
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
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
      .operands = {bir::Value::immediate_i32(15)},
      .operand_homes = {std::nullopt},
      .has_side_effects = true,
      .source_callee_name = std::string{"llvm.aarch64.dmb"},
      .has_prepared_call_plan = true,
  };
  return &carrier;
}

aarch64_codegen::Crc32WIntrinsicRecord crc32w_record(
    aarch64_codegen::RegisterOperand result_register,
    aarch64_codegen::RegisterOperand accumulator_register,
    aarch64_codegen::RegisterOperand data_register) {
  accumulator_register.value_id = prepare::PreparedValueId{91};
  accumulator_register.value_name = c4c::ValueNameId{91};
  data_register.value_id = prepare::PreparedValueId{92};
  data_register.value_name = c4c::ValueNameId{92};
  result_register.value_id = prepare::PreparedValueId{93};
  result_register.value_name = c4c::ValueNameId{93};
  return aarch64_codegen::Crc32WIntrinsicRecord{
      .source_carrier = complete_crc32w_carrier(),
      .family = bir::IntrinsicFamilyKind::Crc,
      .operation = bir::IntrinsicOperationKind::Crc32W,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Crc,
      .operand_type = bir::TypeKind::I32,
      .result_type = bir::TypeKind::I32,
      .operand_roles = {bir::IntrinsicOperandRole::Accumulator,
                        bir::IntrinsicOperandRole::Data},
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .accumulator_value_id = prepare::PreparedValueId{91},
      .accumulator_value_name = c4c::ValueNameId{91},
      .data_value_id = prepare::PreparedValueId{92},
      .data_value_name = c4c::ValueNameId{92},
      .result_value_id = prepare::PreparedValueId{93},
      .result_value_name = c4c::ValueNameId{93},
      .accumulator = aarch64_codegen::make_register_operand(accumulator_register),
      .data = aarch64_codegen::make_register_operand(data_register),
      .result_register = result_register,
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.crc32w"},
      .has_prepared_call_plan = true,
  };
}

aarch64_codegen::VectorLoadIntrinsicRecord vector_load_record(
    aarch64_codegen::RegisterOperand result_register,
    aarch64_codegen::RegisterOperand pointer_register) {
  pointer_register.value_id = prepare::PreparedValueId{101};
  pointer_register.value_name = c4c::ValueNameId{101};
  result_register.value_id = prepare::PreparedValueId{102};
  result_register.value_name = c4c::ValueNameId{102};
  return aarch64_codegen::VectorLoadIntrinsicRecord{
      .source_carrier = complete_vector_load_carrier(),
      .family = bir::IntrinsicFamilyKind::VectorMemory,
      .operation = bir::IntrinsicOperationKind::VectorLoad,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
      .operand_type = bir::TypeKind::Ptr,
      .result_type = bir::TypeKind::I128,
      .operand_roles = {bir::IntrinsicOperandRole::Pointer},
      .vector_element_type = bir::TypeKind::I8,
      .vector_element_width_bytes = 1,
      .vector_lane_count = 16,
      .vector_total_width_bytes = 16,
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .memory_access = bir::IntrinsicMemoryAccessKind::Read,
      .pointer_value_id = prepare::PreparedValueId{101},
      .pointer_value_name = c4c::ValueNameId{101},
      .result_value_id = prepare::PreparedValueId{102},
      .result_value_name = c4c::ValueNameId{102},
      .pointer = aarch64_codegen::make_register_operand(pointer_register),
      .memory =
          aarch64_codegen::MemoryOperand{
              .function_name = c4c::FunctionNameId{2},
              .block_label = c4c::BlockLabelId{3},
              .instruction_index = 4,
              .result_value_id = prepare::PreparedValueId{102},
              .result_value_name = c4c::ValueNameId{102},
              .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
              .base_register = pointer_register,
              .pointer_value_name = c4c::ValueNameId{101},
              .pointer_value_id = prepare::PreparedValueId{101},
              .size_bytes = 16,
              .align_bytes = 16,
              .address_space = bir::AddressSpace::Default,
          },
      .result_register = result_register,
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.neon.ld1.v16i8.p0i8"},
      .has_prepared_call_plan = true,
  };
}

aarch64_codegen::VectorAddIntrinsicRecord vector_add_record(
    aarch64_codegen::RegisterOperand result_register,
    aarch64_codegen::RegisterOperand lhs_register,
    aarch64_codegen::RegisterOperand rhs_register) {
  lhs_register.value_id = prepare::PreparedValueId{111};
  lhs_register.value_name = c4c::ValueNameId{111};
  rhs_register.value_id = prepare::PreparedValueId{112};
  rhs_register.value_name = c4c::ValueNameId{112};
  result_register.value_id = prepare::PreparedValueId{113};
  result_register.value_name = c4c::ValueNameId{113};
  return aarch64_codegen::VectorAddIntrinsicRecord{
      .source_carrier = complete_vector_add_carrier(),
      .family = bir::IntrinsicFamilyKind::VectorOperation,
      .operation = bir::IntrinsicOperationKind::VectorAdd,
      .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
      .operand_type = bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .operand_roles = {bir::IntrinsicOperandRole::VectorLhs,
                        bir::IntrinsicOperandRole::VectorRhs},
      .vector_element_type = bir::TypeKind::I8,
      .vector_element_width_bytes = 1,
      .vector_lane_count = 16,
      .vector_total_width_bytes = 16,
      .signedness = bir::IntrinsicSignedness::Unsigned,
      .memory_access = bir::IntrinsicMemoryAccessKind::None,
      .lhs_value_id = prepare::PreparedValueId{111},
      .lhs_value_name = c4c::ValueNameId{111},
      .rhs_value_id = prepare::PreparedValueId{112},
      .rhs_value_name = c4c::ValueNameId{112},
      .result_value_id = prepare::PreparedValueId{113},
      .result_value_name = c4c::ValueNameId{113},
      .lhs = aarch64_codegen::make_register_operand(lhs_register),
      .rhs = aarch64_codegen::make_register_operand(rhs_register),
      .result_register = result_register,
      .requires_feature = true,
      .source_callee_name = std::string{"llvm.aarch64.neon.add.v16i8"},
      .has_prepared_call_plan = true,
  };
}

aarch64_codegen::ScalarFpUnaryIntrinsicRecord scalar_fp_unary_fabs_record(
    bir::TypeKind type,
    aarch64_codegen::RegisterOperand result_register,
    aarch64_codegen::RegisterOperand operand_register) {
  operand_register.value_id = prepare::PreparedValueId{82};
  operand_register.value_name = c4c::ValueNameId{82};
  result_register.value_id = prepare::PreparedValueId{83};
  result_register.value_name = c4c::ValueNameId{83};
  return aarch64_codegen::ScalarFpUnaryIntrinsicRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .source_carrier = complete_scalar_fp_unary_fabs_carrier(type),
      .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
      .operation = bir::IntrinsicOperationKind::FAbs,
      .operand_type = type,
      .result_type = type,
      .operand_value_id = prepare::PreparedValueId{82},
      .operand_value_name = c4c::ValueNameId{82},
      .result_value_id = prepare::PreparedValueId{83},
      .result_value_name = c4c::ValueNameId{83},
      .operand = aarch64_codegen::make_register_operand(operand_register),
      .result_register = result_register,
      .source_callee_name = type == bir::TypeKind::F32
                                ? std::optional<std::string>{"llvm.fabs.float"}
                                : std::optional<std::string>{"llvm.fabs.double"},
      .has_prepared_call_plan = true,
  };
}

aarch64_codegen::AtomicMemoryInstructionRecord base_atomic_record(
    aarch64_codegen::AtomicMemoryInstructionKind kind,
    std::size_t instruction_index,
    bir::AtomicOrdering ordering) {
  auto pointer = xreg(0);
  pointer.value_id = prepare::PreparedValueId{500};
  pointer.value_name = c4c::ValueNameId{501};
  return aarch64_codegen::AtomicMemoryInstructionRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .atomic_kind = kind,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .block_index = 0,
      .instruction_index = instruction_index,
      .value_type = bir::TypeKind::I32,
      .width_bytes = 4,
      .ordering = ordering,
      .address_space = bir::AddressSpace::Default,
      .pointer_value_id = prepare::PreparedValueId{500},
      .pointer_value_name = c4c::ValueNameId{501},
      .pointer_register = pointer,
      .acquire_semantics = ordering == bir::AtomicOrdering::Acquire ||
                           ordering == bir::AtomicOrdering::AcqRel ||
                           ordering == bir::AtomicOrdering::SeqCst,
      .release_semantics = ordering == bir::AtomicOrdering::Release ||
                           ordering == bir::AtomicOrdering::AcqRel ||
                           ordering == bir::AtomicOrdering::SeqCst,
      .sequentially_consistent = ordering == bir::AtomicOrdering::SeqCst,
      .memory_barrier_required = ordering != bir::AtomicOrdering::Relaxed,
      .source_carrier = complete_atomic_carrier(),
  };
}

aarch64_codegen::MemoryOperand frame_slot(std::int64_t offset) {
  return aarch64_codegen::MemoryOperand{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
      .frame_slot_id = prepare::PreparedFrameSlotId{4},
      .byte_offset = offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Default,
      .can_use_base_plus_offset = true,
  };
}

aarch64_codegen::MemoryOperand f128_frame_slot(std::int64_t offset) {
  auto memory = frame_slot(offset);
  memory.size_bytes = 16;
  memory.align_bytes = 16;
  return memory;
}

aarch64_codegen::I128PairOperandRecord i128_pair_operand(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    unsigned low_register) {
  auto low = xreg(low_register);
  auto high = xreg(low_register + 1);
  low.value_id = value_id;
  low.value_name = value_name;
  high.value_id = value_id;
  high.value_name = value_name;
  return aarch64_codegen::I128PairOperandRecord{
      .value_id = value_id,
      .value_name = value_name,
      .carrier_kind = prepare::PreparedI128CarrierKind::RegisterPair,
      .low_lane =
          aarch64_codegen::I128LaneTransportRecord{
              .role = prepare::PreparedI128LaneRole::Low,
              .lane_index = 0,
              .width_bytes = 8,
              .reg = low,
          },
      .high_lane =
          aarch64_codegen::I128LaneTransportRecord{
              .role = prepare::PreparedI128LaneRole::High,
              .lane_index = 1,
              .width_bytes = 8,
              .reg = high,
          },
      .source_carrier = reinterpret_cast<const prepare::PreparedI128Carrier*>(0x1),
  };
}

aarch64_codegen::F128RuntimeHelperOperandRecord f128_helper_operand(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    unsigned carrier_register,
    unsigned abi_register,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction,
    std::optional<std::size_t> argument_index = std::nullopt) {
  auto carrier = qreg(carrier_register);
  carrier.value_id = value_id;
  carrier.value_name = value_name;
  auto abi = qreg(abi_register);
  abi.value_id = value_id;
  abi.value_name = value_name;
  const auto carrier_name = std::string{"q"} + std::to_string(carrier_register);
  const auto abi_name = std::string{"q"} + std::to_string(abi_register);
  const prepare::PreparedF128RuntimeHelper::CarrierBinding carrier_binding{
      .value_id = value_id,
      .value_name = value_name,
      .carrier_kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
      .width_bytes = 16,
      .align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .register_name = carrier_name,
  };
  const prepare::PreparedF128RuntimeHelper::AbiRegisterBinding abi_binding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = argument_index,
      .abi_register_index = abi_register,
      .width_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .register_name = abi_name,
      .contiguous_width = 1,
      .occupied_register_names = {abi_name},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Vreg,
              .pool = argument_index.has_value()
                          ? prepare::PreparedRegisterSlotPool::CallArgument
                          : prepare::PreparedRegisterSlotPool::CallResult,
              .slot_index = abi_register,
              .contiguous_width = 1,
          },
  };
  return aarch64_codegen::F128RuntimeHelperOperandRecord{
      .value_id = value_id,
      .value_name = value_name,
      .carrier_kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
      .width_bytes = 16,
      .align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .carrier_register = carrier,
      .abi_register = abi,
      .carrier_binding = carrier_binding,
      .abi_binding = abi_binding,
      .marshaling_move =
          prepare::PreparedF128RuntimeHelper::MarshalingMove{
              .direction = direction,
              .carrier = carrier_binding,
              .abi_register = abi_binding,
          },
      .source_carrier = reinterpret_cast<const prepare::PreparedF128Carrier*>(0x1),
  };
}

aarch64_codegen::F128RuntimeHelperScalarResultRecord f128_helper_scalar_record(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type,
    std::size_t width_bytes,
    aarch64_codegen::RegisterOperand materialized,
    aarch64_codegen::RegisterOperand abi,
    std::string abi_name,
    prepare::PreparedRegisterBank bank,
    prepare::PreparedRegisterClass reg_class,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction,
    std::optional<prepare::PreparedF128CmpResultZeroTest> zero_test = std::nullopt) {
  materialized.value_id = value_id;
  materialized.value_name = value_name;
  abi.value_id = value_id;
  abi.value_name = value_name;
  const auto ownership = prepare::PreparedF128RuntimeHelper::ScalarResultOwnership{
      .value_id = value_id,
      .value_name = value_name,
      .type = type,
      .width_bytes = width_bytes,
      .register_bank = bank,
      .home_kind = prepare::PreparedValueHomeKind::Register,
      .register_name = abi_name,
  };
  const auto abi_binding = prepare::PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .abi_register_index = abi.reg.index,
      .width_bytes = width_bytes,
      .register_bank = bank,
      .register_class = reg_class,
      .register_name = abi_name,
      .contiguous_width = 1,
      .occupied_register_names = {abi_name},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = bank,
              .pool = direction ==
                              prepare::PreparedF128RuntimeHelperMarshalDirection::
                                  ScalarToAbiArgument
                          ? prepare::PreparedRegisterSlotPool::CallArgument
                          : prepare::PreparedRegisterSlotPool::CallResult,
              .slot_index = abi.reg.index,
              .contiguous_width = 1,
          },
  };
  return aarch64_codegen::F128RuntimeHelperScalarResultRecord{
      .value_id = value_id,
      .value_name = value_name,
      .type = type,
      .width_bytes = width_bytes,
      .register_bank = bank,
      .home_kind = prepare::PreparedValueHomeKind::Register,
      .materialized_i1_register = materialized,
      .abi_register = abi,
      .scalar_ownership = ownership,
      .marshaling_move =
          prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove{
              .direction = direction,
              .scalar_result = ownership,
              .abi_register = abi_binding,
          },
      .cmp_result_consumption =
          zero_test.has_value()
              ? std::optional<prepare::PreparedF128RuntimeHelper::ScalarCmpResultConsumption>{
                    prepare::PreparedF128RuntimeHelper::ScalarCmpResultConsumption{
                        .cmp_type = bir::TypeKind::I32,
                        .bir_result_type = bir::TypeKind::I1,
                        .zero_test = *zero_test,
                        .consumes_helper_cmp_result = true,
                        .owns_bir_i1_result = true,
                    }}
              : std::nullopt,
  };
}

aarch64_codegen::F128RuntimeHelperBoundaryRecord printable_f128_arithmetic_helper() {
  return aarch64_codegen::F128RuntimeHelperBoundaryRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .boundary_kind = aarch64_codegen::F128RuntimeHelperBoundaryKind::Add,
      .helper_family = prepare::PreparedF128RuntimeHelperFamily::Arithmetic,
      .helper_kind = prepare::PreparedF128RuntimeHelperKind::Add,
      .callee_name = "__addtf3",
      .source_binary_opcode = bir::BinaryOpcode::Add,
      .function_name = c4c::FunctionNameId{2},
      .block_index = 0,
      .instruction_index = 2,
      .source_type = bir::TypeKind::F128,
      .result_type = bir::TypeKind::F128,
      .result_value_id = prepare::PreparedValueId{90},
      .result_value_name = c4c::ValueNameId{90},
      .lhs_value_id = prepare::PreparedValueId{91},
      .lhs_value_name = c4c::ValueNameId{91},
      .rhs_value_id = prepare::PreparedValueId{92},
      .rhs_value_name = c4c::ValueNameId{92},
      .result_ownership =
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier,
      .width_bytes = 16,
      .align_bytes = 16,
      .result =
          f128_helper_operand(prepare::PreparedValueId{90},
                              c4c::ValueNameId{90},
                              4,
                              0,
                              prepare::PreparedF128RuntimeHelperMarshalDirection::
                                  AbiResultToCarrier),
      .lhs =
          f128_helper_operand(prepare::PreparedValueId{91},
                              c4c::ValueNameId{91},
                              6,
                              0,
                              prepare::PreparedF128RuntimeHelperMarshalDirection::
                                  CarrierToAbiArgument,
                              std::size_t{0}),
      .rhs =
          f128_helper_operand(prepare::PreparedValueId{92},
                              c4c::ValueNameId{92},
                              8,
                              1,
                              prepare::PreparedF128RuntimeHelperMarshalDirection::
                                  CarrierToAbiArgument,
                              std::size_t{1}),
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
              .register_name = "q0",
              .contiguous_width = 1,
              .occupied_register_names = {"q0"},
          }},
      .source_helper = reinterpret_cast<const prepare::PreparedF128RuntimeHelper*>(0x1),
  };
}

aarch64_codegen::F128RuntimeHelperBoundaryRecord printable_f128_comparison_helper() {
  auto helper = printable_f128_arithmetic_helper();
  helper.boundary_kind = aarch64_codegen::F128RuntimeHelperBoundaryKind::Eq;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Comparison;
  helper.helper_kind = prepare::PreparedF128RuntimeHelperKind::Eq;
  helper.callee_name = "__eqtf2";
  helper.source_binary_opcode = bir::BinaryOpcode::Eq;
  helper.result_type = bir::TypeKind::I32;
  helper.result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult;
  helper.scalar_result =
      f128_helper_scalar_record(prepare::PreparedValueId{90},
                                c4c::ValueNameId{90},
                                bir::TypeKind::I32,
                                4,
                                wreg(9),
                                wreg(0),
                                "w0",
                                prepare::PreparedRegisterBank::Gpr,
                                prepare::PreparedRegisterClass::General,
                                prepare::PreparedF128RuntimeHelperMarshalDirection::
                                    AbiCmpResultToScalar,
                                prepare::PreparedF128CmpResultZeroTest::EqualZero);
  helper.abi_policy.transition =
      prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult;
  helper.abi_policy.result_bank = prepare::PreparedRegisterBank::Gpr;
  helper.abi_policy.result_count = 1;
  helper.abi_policy.width_bytes = 4;
  return helper;
}

aarch64_codegen::F128RuntimeHelperBoundaryRecord printable_f64_to_f128_cast_helper() {
  auto helper = printable_f128_arithmetic_helper();
  helper.boundary_kind = aarch64_codegen::F128RuntimeHelperBoundaryKind::F64ToF128;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Cast;
  helper.helper_kind = prepare::PreparedF128RuntimeHelperKind::F64ToF128;
  helper.callee_name = "__extenddftf2";
  helper.source_cast_opcode = bir::CastOpcode::FPExt;
  helper.source_type = bir::TypeKind::F64;
  helper.result_type = bir::TypeKind::F128;
  helper.operand_value_id = prepare::PreparedValueId{91};
  helper.operand_value_name = c4c::ValueNameId{91};
  helper.result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier;
  helper.scalar_operand =
      f128_helper_scalar_record(prepare::PreparedValueId{91},
                                c4c::ValueNameId{91},
                                bir::TypeKind::F64,
                                8,
                                dreg(6),
                                dreg(0),
                                "d0",
                                prepare::PreparedRegisterBank::Fpr,
                                prepare::PreparedRegisterClass::Float,
                                prepare::PreparedF128RuntimeHelperMarshalDirection::
                                    ScalarToAbiArgument);
  helper.result =
      f128_helper_operand(prepare::PreparedValueId{90},
                          c4c::ValueNameId{90},
                          4,
                          0,
                          prepare::PreparedF128RuntimeHelperMarshalDirection::
                              AbiResultToCarrier);
  helper.abi_policy.transition =
      prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result;
  helper.abi_policy.argument_bank = prepare::PreparedRegisterBank::Fpr;
  helper.abi_policy.result_bank = prepare::PreparedRegisterBank::Vreg;
  helper.abi_policy.argument_count = 1;
  helper.abi_policy.result_count = 1;
  helper.abi_policy.width_bytes = 16;
  return helper;
}

aarch64_codegen::F128RuntimeHelperBoundaryRecord printable_f128_to_f32_cast_helper() {
  auto helper = printable_f128_arithmetic_helper();
  helper.boundary_kind = aarch64_codegen::F128RuntimeHelperBoundaryKind::F128ToF32;
  helper.helper_family = prepare::PreparedF128RuntimeHelperFamily::Cast;
  helper.helper_kind = prepare::PreparedF128RuntimeHelperKind::F128ToF32;
  helper.callee_name = "__trunctfsf2";
  helper.source_cast_opcode = bir::CastOpcode::FPTrunc;
  helper.source_type = bir::TypeKind::F128;
  helper.result_type = bir::TypeKind::F32;
  helper.operand_value_id = prepare::PreparedValueId{91};
  helper.operand_value_name = c4c::ValueNameId{91};
  helper.result_ownership = prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue;
  helper.lhs =
      f128_helper_operand(prepare::PreparedValueId{91},
                          c4c::ValueNameId{91},
                          6,
                          0,
                          prepare::PreparedF128RuntimeHelperMarshalDirection::
                              CarrierToAbiArgument,
                          std::size_t{0});
  helper.scalar_result =
      f128_helper_scalar_record(prepare::PreparedValueId{90},
                                c4c::ValueNameId{90},
                                bir::TypeKind::F32,
                                4,
                                sreg(4),
                                sreg(0),
                                "s0",
                                prepare::PreparedRegisterBank::Fpr,
                                prepare::PreparedRegisterClass::Float,
                                prepare::PreparedF128RuntimeHelperMarshalDirection::
                                    AbiResultToScalar);
  helper.abi_policy.transition =
      prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult;
  helper.abi_policy.argument_bank = prepare::PreparedRegisterBank::Vreg;
  helper.abi_policy.result_bank = prepare::PreparedRegisterBank::Fpr;
  helper.abi_policy.argument_count = 1;
  helper.abi_policy.result_count = 1;
  helper.abi_policy.width_bytes = 4;
  return helper;
}

prepare::PreparedI128RuntimeHelper::LaneBinding i128_helper_lane(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    prepare::PreparedI128LaneRole role,
    std::size_t lane_index,
    std::string register_name) {
  return prepare::PreparedI128RuntimeHelper::LaneBinding{
      .value_id = value_id,
      .value_name = value_name,
      .carrier_kind = prepare::PreparedI128CarrierKind::RegisterPair,
      .role = role,
      .lane_index = lane_index,
      .width_bytes = 8,
      .register_name = std::move(register_name),
  };
}

prepare::PreparedI128RuntimeHelper::AbiRegisterBinding i128_helper_abi(
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

prepare::PreparedI128RuntimeHelper::MarshalingMove i128_helper_move(
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

aarch64_codegen::I128RuntimeHelperBoundaryKind boundary_kind_for_helper(
    prepare::PreparedI128RuntimeHelperKind kind) {
  switch (kind) {
    case prepare::PreparedI128RuntimeHelperKind::SignedDiv:
      return aarch64_codegen::I128RuntimeHelperBoundaryKind::SignedDiv;
    case prepare::PreparedI128RuntimeHelperKind::UnsignedDiv:
      return aarch64_codegen::I128RuntimeHelperBoundaryKind::UnsignedDiv;
    case prepare::PreparedI128RuntimeHelperKind::SignedRem:
      return aarch64_codegen::I128RuntimeHelperBoundaryKind::SignedRem;
    case prepare::PreparedI128RuntimeHelperKind::UnsignedRem:
      return aarch64_codegen::I128RuntimeHelperBoundaryKind::UnsignedRem;
  }
  return aarch64_codegen::I128RuntimeHelperBoundaryKind::SignedDiv;
}

prepare::PreparedI128RuntimeHelper printable_i128_helper(
    bir::BinaryOpcode opcode,
    prepare::PreparedI128RuntimeHelperKind kind,
    std::string callee,
    prepare::PreparedValueId result_id,
    c4c::ValueNameId result_name,
    unsigned result_low,
    prepare::PreparedValueId lhs_id,
    c4c::ValueNameId lhs_name,
    unsigned lhs_low,
    prepare::PreparedValueId rhs_id,
    c4c::ValueNameId rhs_name,
    unsigned rhs_low) {
  auto helper = prepare::PreparedI128RuntimeHelper{
      .function_name = c4c::FunctionNameId{2},
      .block_index = 0,
      .instruction_index = 0,
      .source_binary_opcode = opcode,
      .source_type = bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .result_value_id = result_id,
      .result_value_name = result_name,
      .lhs_value_id = lhs_id,
      .lhs_value_name = lhs_name,
      .rhs_value_id = rhs_id,
      .rhs_value_name = rhs_name,
      .helper_family = prepare::PreparedI128RuntimeHelperFamily::DivRem,
      .helper_kind = kind,
      .callee_name = std::move(callee),
      .result_ownership =
          prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes,
      .lhs_low_lane =
          i128_helper_lane(lhs_id, lhs_name, prepare::PreparedI128LaneRole::Low, 0,
                           "x" + std::to_string(lhs_low)),
      .lhs_high_lane =
          i128_helper_lane(lhs_id, lhs_name, prepare::PreparedI128LaneRole::High, 1,
                           "x" + std::to_string(lhs_low + 1)),
      .rhs_low_lane =
          i128_helper_lane(rhs_id, rhs_name, prepare::PreparedI128LaneRole::Low, 0,
                           "x" + std::to_string(rhs_low)),
      .rhs_high_lane =
          i128_helper_lane(rhs_id, rhs_name, prepare::PreparedI128LaneRole::High, 1,
                           "x" + std::to_string(rhs_low + 1)),
      .result_low_lane =
          i128_helper_lane(result_id, result_name, prepare::PreparedI128LaneRole::Low, 0,
                           "x" + std::to_string(result_low)),
      .result_high_lane =
          i128_helper_lane(result_id, result_name, prepare::PreparedI128LaneRole::High, 1,
                           "x" + std::to_string(result_low + 1)),
      .live_preservation_policy =
          prepare::PreparedI128RuntimeHelper::LivePreservationPolicy{
              .evaluated = true,
              .caller_saved_clobbers_modeled = true,
              .no_additional_live_preservation_required = true,
          },
      .selected_call_ownership =
          prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{
              .owns_terminal_call = true,
              .has_callee_identity = true,
              .has_resource_policy = true,
              .has_clobber_policy = true,
              .has_abi_bindings = true,
              .has_marshaling = true,
              .has_live_preservation = true,
          },
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
  helper.lhs_low_abi_argument =
      i128_helper_abi(lhs_id, lhs_name, prepare::PreparedI128LaneRole::Low, 0,
                      std::size_t{0}, 0, "x0",
                      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.lhs_high_abi_argument =
      i128_helper_abi(lhs_id, lhs_name, prepare::PreparedI128LaneRole::High, 1,
                      std::size_t{0}, 1, "x1",
                      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_low_abi_argument =
      i128_helper_abi(rhs_id, rhs_name, prepare::PreparedI128LaneRole::Low, 0,
                      std::size_t{1}, 2, "x2",
                      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.rhs_high_abi_argument =
      i128_helper_abi(rhs_id, rhs_name, prepare::PreparedI128LaneRole::High, 1,
                      std::size_t{1}, 3, "x3",
                      prepare::PreparedRegisterSlotPool::CallArgument);
  helper.result_low_abi_result =
      i128_helper_abi(result_id, result_name, prepare::PreparedI128LaneRole::Low, 0,
                      std::nullopt, 0, "x0",
                      prepare::PreparedRegisterSlotPool::CallResult);
  helper.result_high_abi_result =
      i128_helper_abi(result_id, result_name, prepare::PreparedI128LaneRole::High, 1,
                      std::nullopt, 1, "x1",
                      prepare::PreparedRegisterSlotPool::CallResult);
  helper.lhs_low_argument_move = i128_helper_move(
      *helper.lhs_low_lane, *helper.lhs_low_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.lhs_high_argument_move = i128_helper_move(
      *helper.lhs_high_lane, *helper.lhs_high_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.rhs_low_argument_move = i128_helper_move(
      *helper.rhs_low_lane, *helper.rhs_low_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.rhs_high_argument_move = i128_helper_move(
      *helper.rhs_high_lane, *helper.rhs_high_abi_argument,
      prepare::PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument);
  helper.result_low_unmarshal_move = i128_helper_move(
      *helper.result_low_lane, *helper.result_low_abi_result,
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane);
  helper.result_high_unmarshal_move = i128_helper_move(
      *helper.result_high_lane, *helper.result_high_abi_result,
      prepare::PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane);
  return helper;
}

aarch64_codegen::I128RuntimeHelperBoundaryRecord i128_helper_record(
    const prepare::PreparedI128RuntimeHelper* source_helper,
    bir::BinaryOpcode opcode,
    prepare::PreparedI128RuntimeHelperKind kind,
    std::string callee,
    prepare::PreparedValueId result_id,
    c4c::ValueNameId result_name,
    unsigned result_low,
    prepare::PreparedValueId lhs_id,
    c4c::ValueNameId lhs_name,
    unsigned lhs_low,
    prepare::PreparedValueId rhs_id,
    c4c::ValueNameId rhs_name,
    unsigned rhs_low) {
  return aarch64_codegen::I128RuntimeHelperBoundaryRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .boundary_kind = boundary_kind_for_helper(kind),
      .helper_family = prepare::PreparedI128RuntimeHelperFamily::DivRem,
      .helper_kind = kind,
      .callee_name = std::move(callee),
      .source_binary_opcode = opcode,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .source_type = bir::TypeKind::I128,
      .result_type = bir::TypeKind::I128,
      .result_value_id = result_id,
      .result_value_name = result_name,
      .lhs_value_id = lhs_id,
      .lhs_value_name = lhs_name,
      .rhs_value_id = rhs_id,
      .rhs_value_name = rhs_name,
      .result_ownership =
          prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .result = i128_pair_operand(result_id, result_name, result_low),
      .lhs = i128_pair_operand(lhs_id, lhs_name, lhs_low),
      .rhs = i128_pair_operand(rhs_id, rhs_name, rhs_low),
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
      .live_preservation_policy =
          prepare::PreparedI128RuntimeHelper::LivePreservationPolicy{
              .evaluated = true,
              .caller_saved_clobbers_modeled = true,
              .no_additional_live_preservation_required = true,
          },
      .selected_call_ownership =
          prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{
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
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = "x13",
              .contiguous_width = 1,
              .occupied_register_names = {"x13"},
          }},
      .source_helper = source_helper,
  };
}

aarch64_codegen::I128RuntimeHelperBoundaryRecord incomplete_i128_helper_record() {
  auto record = i128_helper_record(nullptr,
                                   bir::BinaryOpcode::SDiv,
                                   prepare::PreparedI128RuntimeHelperKind::SignedDiv,
                                   "__divti3",
                                   prepare::PreparedValueId{70},
                                   c4c::ValueNameId{70},
                                   0,
                                   prepare::PreparedValueId{71},
                                   c4c::ValueNameId{71},
                                   2,
                                   prepare::PreparedValueId{72},
                                   c4c::ValueNameId{72},
                                   4);
  record.source_helper = reinterpret_cast<const prepare::PreparedI128RuntimeHelper*>(0x1);
  record.live_preservation_policy.no_additional_live_preservation_required = false;
  record.selected_call_ownership.owns_terminal_call = false;
  record.selected_call_ownership.has_live_preservation = false;
  return record;
}

int selected_spill_reload_nodes_print_gnu_aarch64_text() {
  int source_op = 0;
  const auto spill = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{10},
          .value_name = c4c::ValueNameId{11},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Spill,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::SpillToSlot,
          .slot = frame_slot(16),
          .scratch = xreg(9),
          .occupied_scratch_register_references = {aarch64_abi::x_register(9)},
          .occupied_scratch_registers = {"x9"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{16},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });
  const auto reload = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{10},
          .value_name = c4c::ValueNameId{11},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Reload,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::ReloadFromSlot,
          .slot = frame_slot(16),
          .scratch = xreg(9),
          .occupied_scratch_register_references = {aarch64_abi::x_register(9)},
          .occupied_scratch_registers = {"x9"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{16},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });
  const auto large_reload = aarch64_codegen::make_spill_reload_instruction(
      aarch64_codegen::SpillReloadInstructionRecord{
          .value_id = prepare::PreparedValueId{12},
          .value_name = c4c::ValueNameId{13},
          .value_type = bir::TypeKind::I64,
          .op_kind = prepare::PreparedSpillReloadOpKind::Reload,
          .pseudo_kind = aarch64_codegen::MachinePseudoKind::ReloadFromSlot,
          .slot = frame_slot(1644),
          .scratch = xreg(13),
          .occupied_scratch_register_references = {aarch64_abi::x_register(13)},
          .occupied_scratch_registers = {"x13"},
          .scratch_register_authority = std::size_t{0},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{1644},
          .stack_offset_is_prepared_snapshot = true,
          .source_spill_reload =
              reinterpret_cast<const prepare::PreparedSpillReloadOp*>(&source_op),
      });

  const auto result = print_common_instruction_nodes({spill, reload, large_reload});
  if (!result.ok || !result.diagnostic.empty()) {
    return fail("expected selected spill/reload nodes to print canonical AArch64 text");
  }
  const auto spill_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(spill);
  const auto reload_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(reload);
  const std::string expected =
      "    " + std::string(spill_mnemonic) + " x9, [sp, #16]\n" +
      "    " + std::string(reload_mnemonic) + " x9, [sp, #16]\n"
      "    add x9, sp, #1644\n"
      "    " + std::string(reload_mnemonic) + " x13, [x9]\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    str x9, [sp, #16]\n"
                                        "    ldr x9, [sp, #16]\n"
                                        "    add x9, sp, #1644\n"
                                        "    ldr x13, [x9]\n",
                                        "spill/reload common-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          spill_mnemonic, "str", "spill helper mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          reload_mnemonic, "ldr", "reload helper mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_branch_and_store_nodes_print_without_semantic_roundtrip() {
  const auto condition = aarch64_codegen::make_register_operand(xreg(1));
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{20},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
              },
          .condition = condition,
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::MaterializedBool,
                  .condition_value_id = prepare::PreparedValueId{20},
                  .condition_value_name = c4c::ValueNameId{21},
                  .condition_type = bir::TypeKind::I1,
              },
          .conditional = true,
      });

  auto store_address = frame_slot(24);
  store_address.stored_value_id = prepare::PreparedValueId{22};
  store_address.stored_value_name = c4c::ValueNameId{23};
  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = store_address,
          .value = aarch64_codegen::make_register_operand(xreg(10)),
          .value_type = bir::TypeKind::I64,
      });

  const auto result = print_common_instruction_nodes({branch, store});
  if (!result.ok) {
    return fail("expected branch and store nodes to print from structured operands: " +
                result.diagnostic);
  }
  const auto conditional_branch_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(branch);
  const auto unconditional_branch_mnemonic = aarch64_codegen::machine_printer_mnemonic_kind_name(
      aarch64_codegen::MachinePrinterMnemonicKind::Branch);
  const auto store_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(store);
  const std::string expected =
      "    " + std::string(conditional_branch_mnemonic) + " x1, .LBB2_7\n" +
      "    " + std::string(unconditional_branch_mnemonic) + " .LBB2_8\n" +
      "    " + std::string(store_mnemonic) + " x10, [sp, #24]\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    cbnz x1, .LBB2_7\n"
                                        "    b .LBB2_8\n"
                                        "    str x10, [sp, #24]\n",
                                        "branch/store common-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          conditional_branch_mnemonic, "cbnz", "conditional branch helper mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          unconditional_branch_mnemonic,
          "b",
          "unconditional branch helper mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          store_mnemonic, "str", "store helper mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int fused_compare_branch_prints_immediate_left_operands_in_aarch64_order() {
  auto instruction = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{20},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
              },
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::FusedCompare,
                  .condition_value_id = prepare::PreparedValueId{20},
                  .condition_value_name = c4c::ValueNameId{20},
                  .condition_type = bir::TypeKind::I32,
                  .predicate =
                      aarch64_codegen::ComparePredicateRecord{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .source_predicate = bir::BinaryOpcode::Slt,
                          .compare_type = bir::TypeKind::I32,
                      },
                  .compare_operands =
                      aarch64_codegen::CompareOperandPairRecord{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .lhs =
                              aarch64_codegen::CompareValueRecord{
                                  .surface =
                                      aarch64_codegen::RecordSurfaceKind::RecordOnly,
                                  .type = bir::TypeKind::I32,
                                  .source_value = bir::Value::immediate_i32(1000),
                              },
                          .rhs =
                              aarch64_codegen::CompareValueRecord{
                                  .surface =
                                      aarch64_codegen::RecordSurfaceKind::RecordOnly,
                                  .value_id = prepare::PreparedValueId{21},
                                  .value_name = c4c::ValueNameId{21},
                                  .type = bir::TypeKind::I32,
                                  .source_value =
                                      bir::Value::named(bir::TypeKind::I32, "%rhs"),
                              },
                          .compare_type = bir::TypeKind::I32,
                      },
                  .can_fuse_with_branch = true,
              },
          .conditional = true,
      });
  if (instruction.operands.size() != 5) {
    return fail("expected fused compare branch to carry compare operand slots");
  }
  instruction.operands[3] =
      aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::SignedInteger,
          .type = bir::TypeKind::I32,
          .signed_value = 1000,
          .unsigned_value = 1000,
      });
  instruction.operands[4] = aarch64_codegen::make_register_operand(wreg(13));

  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(instruction);
  if (!printed.ok || printed.instruction_lines.size() != 3 ||
      printed.instruction_lines[0] != "cmp w13, #1000" ||
      printed.instruction_lines[1] != "b.gt .LBB2_7" ||
      printed.instruction_lines[2] != "b .LBB2_8") {
    return fail("expected immediate-left fused compare branch to print in legal order");
  }
  return 0;
}

int fused_compare_branch_folds_both_immediate_operands_to_direct_branch() {
  auto instruction = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{20},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
              },
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::FusedCompare,
                  .condition_value_id = prepare::PreparedValueId{20},
                  .condition_value_name = c4c::ValueNameId{20},
                  .condition_type = bir::TypeKind::I64,
                  .predicate =
                      aarch64_codegen::ComparePredicateRecord{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .source_predicate = bir::BinaryOpcode::Ult,
                          .compare_type = bir::TypeKind::I64,
                      },
                  .compare_operands =
                      aarch64_codegen::CompareOperandPairRecord{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .lhs =
                              aarch64_codegen::CompareValueRecord{
                                  .surface =
                                      aarch64_codegen::RecordSurfaceKind::RecordOnly,
                                  .type = bir::TypeKind::I64,
                                  .source_value = bir::Value::immediate_i64(4),
                              },
                          .rhs =
                              aarch64_codegen::CompareValueRecord{
                                  .surface =
                                      aarch64_codegen::RecordSurfaceKind::RecordOnly,
                                  .type = bir::TypeKind::I64,
                                  .source_value = bir::Value::immediate_i64(2),
                              },
                          .compare_type = bir::TypeKind::I64,
                      },
                  .can_fuse_with_branch = true,
              },
          .conditional = true,
      });

  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(instruction);
  if (!printed.ok || printed.instruction_lines.size() != 1 ||
      printed.instruction_lines[0] != "b .LBB2_8") {
    return fail("expected both-immediate fused compare branch to fold without cmp");
  }
  return 0;
}

int fused_compare_branch_materializes_nonencodable_immediate_operand() {
  auto instruction = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{7},
                  .function_name = c4c::FunctionNameId{2},
                  .condition_value_id = prepare::PreparedValueId{20},
              },
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{7},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
                  .false_target =
                      aarch64_codegen::BranchTargetOperand{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .block_label = c4c::BlockLabelId{8},
                          .function_name = c4c::FunctionNameId{2},
                          .condition_value_id = prepare::PreparedValueId{20},
                      },
              },
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::FusedCompare,
                  .condition_value_id = prepare::PreparedValueId{20},
                  .condition_value_name = c4c::ValueNameId{20},
                  .condition_type = bir::TypeKind::I64,
                  .predicate =
                      aarch64_codegen::ComparePredicateRecord{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .source_predicate = bir::BinaryOpcode::Slt,
                          .compare_type = bir::TypeKind::I64,
                      },
                  .compare_operands =
                      aarch64_codegen::CompareOperandPairRecord{
                          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                          .lhs =
                              aarch64_codegen::CompareValueRecord{
                                  .surface =
                                      aarch64_codegen::RecordSurfaceKind::RecordOnly,
                                  .value_id = prepare::PreparedValueId{21},
                                  .value_name = c4c::ValueNameId{21},
                                  .type = bir::TypeKind::I64,
                                  .source_value =
                                      bir::Value::named(bir::TypeKind::I64, "%lhs"),
                              },
                          .rhs =
                              aarch64_codegen::CompareValueRecord{
                                  .surface =
                                      aarch64_codegen::RecordSurfaceKind::RecordOnly,
                                  .type = bir::TypeKind::I64,
                                  .source_value = bir::Value::immediate_i64(-2147483648LL),
                              },
                          .compare_type = bir::TypeKind::I64,
                      },
                  .can_fuse_with_branch = true,
              },
          .conditional = true,
      });
  instruction.operands[3] = aarch64_codegen::make_register_operand(xreg(12));
  instruction.operands[4] =
      aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
          .kind = aarch64_codegen::ImmediateKind::SignedInteger,
          .type = bir::TypeKind::I64,
          .signed_value = -2147483648LL,
          .unsigned_value = 0xffffffff80000000ULL,
      });

  const auto printed = aarch64_codegen::print_machine_instruction_line_payloads(instruction);
  if (!printed.ok || printed.instruction_lines.size() != 6 ||
      printed.instruction_lines[0] != "movz x9, #32768, lsl #16" ||
      printed.instruction_lines[1] != "movk x9, #65535, lsl #32" ||
      printed.instruction_lines[2] != "movk x9, #65535, lsl #48" ||
      printed.instruction_lines[3] != "cmp x12, x9" ||
      printed.instruction_lines[4] != "b.lt .LBB2_7" ||
      printed.instruction_lines[5] != "b .LBB2_8") {
    return fail("expected non-encodable fused compare immediate to materialize");
  }
  return 0;
}

int selected_branch_target_requires_matching_block_label_definition() {
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target =
              aarch64_codegen::BranchTargetOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .block_label = c4c::BlockLabelId{8},
                  .function_name = c4c::FunctionNameId{2},
              },
      });

  auto store_address = frame_slot(24);
  store_address.stored_value_id = prepare::PreparedValueId{22};
  store_address.stored_value_name = c4c::ValueNameId{23};
  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = store_address,
          .value = aarch64_codegen::make_register_operand(xreg(10)),
          .value_type = bir::TypeKind::I64,
      });

  mir::MachineFunction<aarch64_codegen::InstructionRecord> function;
  function.function_name = c4c::FunctionNameId{2};
  auto& entry = function.blocks.emplace_back();
  entry.block_label = c4c::BlockLabelId{3};
  entry.index = 0;
  entry.instructions.push_back(
      mir::MachineInstruction<aarch64_codegen::InstructionRecord>{.target = branch});
  entry.successors.push_back(mir::MachineBlockSuccessor{
      .target_label = c4c::BlockLabelId{8},
      .kind = mir::MachineBlockSuccessorKind::Unconditional,
  });
  auto& target = function.blocks.emplace_back();
  target.block_label = c4c::BlockLabelId{8};
  target.index = 1;
  target.instructions.push_back(
      mir::MachineInstruction<aarch64_codegen::InstructionRecord>{.target = store});

  const auto result =
      mir::print_machine_function(function, aarch64_codegen::MachineInstructionPrinter{});
  if (!result.ok) {
    return fail("expected branch target block function to print: " + result.diagnostic);
  }
  if (result.assembly.find("    b .LBB2_8\n") == std::string::npos) {
    return fail("expected branch instruction to reference target block label .LBB2_8");
  }
  if (result.assembly.find(".LBB2_8:\n") == std::string::npos) {
    return fail(
        "expected MIR/AArch64 printer to define target block label .LBB2_8 before target block");
  }
  const auto label_position = result.assembly.find(".LBB2_8:\n");
  const auto store_position = result.assembly.find("    str x10, [sp, #24]\n");
  if (store_position == std::string::npos || label_position > store_position) {
    return fail("expected target block label definition before target block instruction stream");
  }
  return 0;
}

int selected_structured_memory_subset_prints_loads_and_stores() {
  auto load_address = frame_slot(32);
  load_address.result_value_id = prepare::PreparedValueId{24};
  load_address.result_value_name = c4c::ValueNameId{25};
  const auto load = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address = load_address,
          .result_value_id = prepare::PreparedValueId{24},
          .result_value_name = c4c::ValueNameId{25},
          .value_type = bir::TypeKind::I32,
          .result_register = wreg(0),
      });

  auto frame_store_address = frame_slot(40);
  frame_store_address.size_bytes = 4;
  frame_store_address.align_bytes = 4;
  frame_store_address.stored_value_id = prepare::PreparedValueId{26};
  frame_store_address.stored_value_name = c4c::ValueNameId{27};
  const auto frame_store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = frame_store_address,
          .value = aarch64_codegen::make_register_operand(wreg(1)),
          .value_type = bir::TypeKind::I32,
      });

  const auto pointer_base = xreg(2);
  const auto pointer_store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{3},
                  .stored_value_id = prepare::PreparedValueId{28},
                  .stored_value_name = c4c::ValueNameId{29},
                  .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
                  .base_register = pointer_base,
                  .pointer_value_name = c4c::ValueNameId{30},
                  .pointer_value_id = prepare::PreparedValueId{31},
                  .byte_offset = 24,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .address_space = bir::AddressSpace::Tls,
                  .can_use_base_plus_offset = true,
              },
          .value = aarch64_codegen::make_register_operand(wreg(3)),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = print_common_instruction_nodes({load, frame_store, pointer_store});
  if (!result.ok) {
    return fail("expected selected structured memory subset to print: " + result.diagnostic);
  }
  const std::string expected =
      "    ldr w0, [sp, #32]\n"
      "    str w1, [sp, #40]\n"
      "    str w3, [x2, #24]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "structured memory load/store printer drift guard");
}

int selected_large_frame_slot_memory_offsets_materialize_address_scratch() {
  auto load_address = frame_slot(1644);
  load_address.result_value_id = prepare::PreparedValueId{224};
  load_address.result_value_name = c4c::ValueNameId{225};
  const auto load = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address = load_address,
          .result_value_id = prepare::PreparedValueId{224},
          .result_value_name = c4c::ValueNameId{225},
          .value_type = bir::TypeKind::I64,
          .result_register = xreg(13),
      });

  auto store_address = frame_slot(32768);
  store_address.stored_value_id = prepare::PreparedValueId{226};
  store_address.stored_value_name = c4c::ValueNameId{227};
  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = store_address,
          .value = aarch64_codegen::make_register_operand(xreg(9)),
          .value_type = bir::TypeKind::I64,
      });

  const auto result = print_common_instruction_nodes({load, store});
  if (!result.ok) {
    return fail("expected large frame-slot memory offsets to materialize: " +
                result.diagnostic);
  }
  const std::string expected =
      "    add x9, sp, #1644\n"
      "    ldr x13, [x9]\n"
      "    movz x10, #32768\n"
      "    add x10, sp, x10\n"
      "    str x9, [x10]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "large frame-slot memory offset materialization");
}

int selected_byte_stack_source_store_materializes_through_scratch() {
  auto destination = frame_slot(40);
  destination.size_bytes = 1;
  destination.align_bytes = 1;
  destination.stored_value_id = prepare::PreparedValueId{26};
  destination.stored_value_name = c4c::ValueNameId{27};

  auto source = frame_slot(48);
  source.size_bytes = 1;
  source.align_bytes = 1;
  source.result_value_id = prepare::PreparedValueId{28};
  source.result_value_name = c4c::ValueNameId{29};

  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = destination,
          .value = aarch64_codegen::make_memory_operand(source),
          .value_type = bir::TypeKind::I8,
      });

  const auto result = print_common_instruction_nodes({store});
  if (!result.ok) {
    return fail("expected 1-byte stack-source store to print: " + result.diagnostic);
  }
  const std::string expected =
      "    ldrb w9, [sp, #48]\n"
      "    strb w9, [sp, #40]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "byte stack-source store materialization printer");
}

int selected_symbol_stack_source_store_materializes_through_value_scratch() {
  auto destination = frame_slot(0);
  destination.base_kind = aarch64_codegen::MemoryBaseKind::Symbol;
  destination.frame_slot_id = std::nullopt;
  destination.symbol_name = c4c::LinkNameId{34};
  destination.symbol_label = "g.symbol_store";
  destination.byte_offset = 8;
  destination.size_bytes = 4;
  destination.align_bytes = 4;
  destination.stored_value_id = prepare::PreparedValueId{35};
  destination.stored_value_name = c4c::ValueNameId{36};

  auto source = frame_slot(48);
  source.size_bytes = 4;
  source.align_bytes = 4;
  source.result_value_id = prepare::PreparedValueId{37};
  source.result_value_name = c4c::ValueNameId{38};

  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = destination,
          .value = aarch64_codegen::make_memory_operand(source),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = print_common_instruction_nodes({store});
  if (!result.ok) {
    return fail("expected symbol stack-source store to print: " + result.diagnostic);
  }
  const std::string expected =
      "    adrp x9, g.symbol_store+8\n"
      "    add x9, x9, :lo12:g.symbol_store+8\n"
      "    ldr w10, [sp, #48]\n"
      "    str w10, [x9]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "symbol stack-source store materialization printer");
}

int selected_symbol_halfword_immediate_store_materializes_through_value_scratch() {
  auto destination = frame_slot(0);
  destination.base_kind = aarch64_codegen::MemoryBaseKind::Symbol;
  destination.frame_slot_id = std::nullopt;
  destination.symbol_name = c4c::LinkNameId{39};
  destination.symbol_label = "g.symbol_half_store";
  destination.byte_offset = 2;
  destination.size_bytes = 2;
  destination.align_bytes = 2;
  destination.stored_value_id = prepare::PreparedValueId{40};
  destination.stored_value_name = c4c::ValueNameId{41};

  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = destination,
          .value = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I16,
                  .signed_value = 4660,
                  .unsigned_value = 4660,
                  .source_value_id = prepare::PreparedValueId{42},
                  .source_value_name = c4c::ValueNameId{43},
              }),
          .value_type = bir::TypeKind::I16,
      });

  const auto result = print_common_instruction_nodes({store});
  if (!result.ok) {
    return fail("expected 2-byte symbol immediate store to print: " +
                result.diagnostic);
  }
  const std::string expected =
      "    adrp x9, g.symbol_half_store+2\n"
      "    add x9, x9, :lo12:g.symbol_half_store+2\n"
      "    movz w10, #4660\n"
      "    strh w10, [x9]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "symbol halfword immediate store materialization printer");
}

int selected_stack_call_argument_store_prints_from_prepared_stack_facts() {
  auto destination = frame_slot(0);
  destination.size_bytes = 8;
  destination.align_bytes = 8;
  destination.stored_value_id = prepare::PreparedValueId{40};
  destination.stored_value_name = c4c::ValueNameId{41};

  auto source = frame_slot(32);
  source.size_bytes = 8;
  source.align_bytes = 8;
  source.result_value_id = prepare::PreparedValueId{40};
  source.result_value_name = c4c::ValueNameId{41};

  const auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = destination,
          .value = aarch64_codegen::make_memory_operand(source),
          .value_type = bir::TypeKind::I64,
      });

  const auto result = print_common_instruction_nodes({store});
  if (!result.ok) {
    return fail("expected selected stack call-argument store to print: " +
                result.diagnostic);
  }
  const std::string expected =
      "    ldr x9, [sp, #32]\n"
      "    str x9, [sp]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "stack call-argument store printer");
}

int selected_aggregate_stack_copy_prints_from_prepared_memory_operands() {
  const auto source_base = xreg(20);
  const aarch64_codegen::MemoryOperand source{
      .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
      .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 4,
      .result_value_id = prepare::PreparedValueId{42},
      .result_value_name = c4c::ValueNameId{43},
      .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
      .base_register = source_base,
      .pointer_value_name = c4c::ValueNameId{43},
      .pointer_value_id = prepare::PreparedValueId{42},
      .byte_offset = 0,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = 16,
      .align_bytes = 8,
      .address_space = bir::AddressSpace::Default,
      .can_use_base_plus_offset = true,
  };

  auto destination = frame_slot(0);
  destination.surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode;
  destination.instruction_index = 4;
  destination.size_bytes = 16;
  destination.align_bytes = 8;
  destination.stored_value_id = prepare::PreparedValueId{42};
  destination.stored_value_name = c4c::ValueNameId{43};

  const auto source_operand = aarch64_codegen::make_memory_operand(source);
  const auto destination_operand = aarch64_codegen::make_memory_operand(destination);
  const auto copy = aarch64_codegen::InstructionRecord{
      .family = aarch64_codegen::InstructionFamily::Assembler,
      .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
      .opcode = aarch64_codegen::MachineOpcode::Unspecified,
      .selection =
          aarch64_codegen::MachineNodeStatusRecord{
              .status = aarch64_codegen::MachineNodeSelectionStatus::Selected},
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 4,
      .operands = {source_operand, destination_operand},
      .side_effects = {aarch64_codegen::MachineSideEffectKind::MemoryRead,
                       aarch64_codegen::MachineSideEffectKind::MemoryWrite,
                       aarch64_codegen::MachineSideEffectKind::InlineAssembly},
      .payload =
          aarch64_codegen::AssemblerInstructionRecord{
              .operands = {source_operand, destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template =
                  "ldr x9, [x20]\n"
                  "str x9, [sp]\n"
                  "ldr x9, [x20, #8]\n"
                  "str x9, [sp, #8]",
          },
  };

  const auto result = print_common_instruction_nodes({copy});
  if (!result.ok) {
    return fail("expected selected aggregate stack-copy node to print: " +
                result.diagnostic);
  }
  const std::string expected =
      "    ldr x9, [x20]\n"
      "    str x9, [sp]\n"
      "    ldr x9, [x20, #8]\n"
      "    str x9, [sp, #8]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "aggregate stack-copy printer");
}

int call_boundary_stack_printer_shapes_fail_closed_when_not_selected() {
  auto destination = frame_slot(0);
  destination.size_bytes = 8;
  destination.align_bytes = 8;
  destination.stored_value_id = prepare::PreparedValueId{44};
  destination.stored_value_name = c4c::ValueNameId{45};

  auto source = frame_slot(32);
  source.size_bytes = 8;
  source.align_bytes = 8;
  source.result_value_id = prepare::PreparedValueId{44};
  source.result_value_name = c4c::ValueNameId{45};

  auto store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = destination,
          .value = aarch64_codegen::make_memory_operand(source),
          .value_type = bir::TypeKind::I64,
      });
  store.selection =
      aarch64_codegen::MachineNodeStatusRecord{
          .status =
              aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported,
          .diagnostic =
              "call-boundary stack argument move requires AArch64 stack-copy lowering"};
  const auto store_result =
      aarch64_codegen::print_machine_instruction_line_payloads(store);
  if (store_result.ok ||
      store_result.diagnostic.find("printer requires selected machine node") ==
          std::string::npos ||
      store_result.diagnostic.find(
          "call-boundary stack argument move requires AArch64 stack-copy lowering") ==
          std::string::npos) {
    return fail("expected unselected stack call-argument store shape to fail closed");
  }

  const auto aggregate_copy = aarch64_codegen::InstructionRecord{
      .family = aarch64_codegen::InstructionFamily::Assembler,
      .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
      .opcode = aarch64_codegen::MachineOpcode::Unspecified,
      .selection =
          aarch64_codegen::MachineNodeStatusRecord{
              .status = aarch64_codegen::MachineNodeSelectionStatus::Selected},
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 4,
      .payload =
          aarch64_codegen::AssemblerInstructionRecord{
              .has_inline_asm_payload = false,
              .side_effects = true,
          },
  };
  const auto aggregate_result =
      aarch64_codegen::print_machine_instruction_line_payloads(aggregate_copy);
  if (aggregate_result.ok ||
      aggregate_result.diagnostic.find("assembler node is missing inline-asm payload") ==
          std::string::npos) {
    return fail("expected incomplete aggregate stack-copy assembler shape to fail closed");
  }

  return 0;
}

int selected_atomic_load_store_and_fence_print_from_structured_records() {
  auto load_record = base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::Load,
                                        0,
                                        bir::AtomicOrdering::Acquire);
  load_record.result_mode = bir::AtomicResultMode::LoadedValue;
  load_record.result_value_id = prepare::PreparedValueId{510};
  load_record.result_value_name = c4c::ValueNameId{511};
  load_record.result_register = wreg(1);

  auto store_record = base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::Store,
                                         1,
                                         bir::AtomicOrdering::Release);
  store_record.stored_value_id = prepare::PreparedValueId{520};
  store_record.stored_value_name = c4c::ValueNameId{521};
  store_record.stored_register = wreg(2);

  auto fence_record = base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::Fence,
                                         2,
                                         bir::AtomicOrdering::SeqCst);
  fence_record.value_type = bir::TypeKind::Void;
  fence_record.width_bytes = 0;
  fence_record.pointer_value_id.reset();
  fence_record.pointer_value_name.reset();
  fence_record.pointer_register.reset();

  const auto load = aarch64_codegen::make_atomic_memory_instruction(load_record);
  const auto store = aarch64_codegen::make_atomic_memory_instruction(store_record);
  const auto fence = aarch64_codegen::make_atomic_memory_instruction(fence_record);
  const auto result = print_common_instruction_nodes({load, store, fence});
  if (!result.ok) {
    return fail("expected atomic load/store/fence records to print: " + result.diagnostic);
  }
  const std::string expected =
      "    ldar w1, [x0]\n"
      "    stlr w2, [x0]\n"
      "    dmb ish\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "atomic load/store/fence structured printer");
}

int selected_atomic_rmw_loop_prints_from_structured_records() {
  auto rmw_record = base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::RmwLoop,
                                       3,
                                       bir::AtomicOrdering::AcqRel);
  rmw_record.result_mode = bir::AtomicResultMode::OldValue;
  rmw_record.result_value_id = prepare::PreparedValueId{530};
  rmw_record.result_value_name = c4c::ValueNameId{531};
  rmw_record.result_register = wreg(4);
  rmw_record.stored_value_id = prepare::PreparedValueId{532};
  rmw_record.stored_value_name = c4c::ValueNameId{533};
  rmw_record.stored_register = wreg(2);
  rmw_record.rmw_opcode = bir::AtomicRmwOpcode::Add;
  rmw_record.exclusive_retry_loop = true;
  rmw_record.rmw_new_value_register = wreg(5);
  rmw_record.exclusive_status_register = wreg(6);

  const auto rmw = aarch64_codegen::make_atomic_memory_instruction(rmw_record);
  const auto result = print_common_instruction_nodes({rmw});
  if (!result.ok) {
    return fail("expected atomic rmw loop record to print: " + result.diagnostic);
  }
  const std::string expected =
      "    .Latomic_2_3_3_retry:\n"
      "    ldaxr w4, [x0]\n"
      "    add w5, w4, w2\n"
      "    stlxr w6, w5, [x0]\n"
      "    cbnz w6, .Latomic_2_3_3_retry\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "atomic rmw loop structured printer");
}

int selected_atomic_compare_exchange_loops_print_from_structured_records() {
  auto boolean_record =
      base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::CompareExchangeLoop,
                         4,
                         bir::AtomicOrdering::SeqCst);
  boolean_record.result_mode = bir::AtomicResultMode::BooleanSuccess;
  boolean_record.result_value_id = prepare::PreparedValueId{540};
  boolean_record.result_value_name = c4c::ValueNameId{541};
  boolean_record.result_register = wreg(3);
  boolean_record.expected_value_id = prepare::PreparedValueId{542};
  boolean_record.expected_value_name = c4c::ValueNameId{543};
  boolean_record.expected_register = wreg(1);
  boolean_record.desired_value_id = prepare::PreparedValueId{544};
  boolean_record.desired_value_name = c4c::ValueNameId{545};
  boolean_record.desired_register = wreg(2);
  boolean_record.failure_ordering = bir::AtomicOrdering::Acquire;
  boolean_record.exclusive_retry_loop = true;
  boolean_record.compare_exchange_failure_clears_monitor = true;
  boolean_record.compare_exchange_result_is_boolean = true;
  boolean_record.compare_loaded_register = wreg(5);
  boolean_record.exclusive_status_register = wreg(6);

  auto old_value_record =
      base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::CompareExchangeLoop,
                         5,
                         bir::AtomicOrdering::Acquire);
  old_value_record.result_mode = bir::AtomicResultMode::OldValue;
  old_value_record.result_value_id = prepare::PreparedValueId{550};
  old_value_record.result_value_name = c4c::ValueNameId{551};
  old_value_record.result_register = wreg(4);
  old_value_record.expected_value_id = prepare::PreparedValueId{552};
  old_value_record.expected_value_name = c4c::ValueNameId{553};
  old_value_record.expected_register = wreg(1);
  old_value_record.desired_value_id = prepare::PreparedValueId{554};
  old_value_record.desired_value_name = c4c::ValueNameId{555};
  old_value_record.desired_register = wreg(2);
  old_value_record.failure_ordering = bir::AtomicOrdering::Relaxed;
  old_value_record.exclusive_retry_loop = true;
  old_value_record.compare_exchange_failure_clears_monitor = true;
  old_value_record.compare_exchange_result_is_old_value = true;
  old_value_record.exclusive_status_register = wreg(6);

  const auto boolean_exchange =
      aarch64_codegen::make_atomic_memory_instruction(boolean_record);
  const auto old_value_exchange =
      aarch64_codegen::make_atomic_memory_instruction(old_value_record);
  const auto result =
      print_common_instruction_nodes({boolean_exchange, old_value_exchange});
  if (!result.ok) {
    return fail("expected atomic compare-exchange loop records to print: " +
                result.diagnostic);
  }
  const std::string expected =
      "    .Latomic_2_3_4_retry:\n"
      "    ldaxr w5, [x0]\n"
      "    cmp w5, w1\n"
      "    b.ne .Latomic_2_3_4_failure\n"
      "    stlxr w6, w2, [x0]\n"
      "    cbnz w6, .Latomic_2_3_4_retry\n"
      "    mov w3, #1\n"
      "    b .Latomic_2_3_4_done\n"
      "    .Latomic_2_3_4_failure:\n"
      "    clrex\n"
      "    mov w3, #0\n"
      "    .Latomic_2_3_4_done:\n"
      "    .Latomic_2_3_5_retry:\n"
      "    ldxr w4, [x0]\n"
      "    cmp w4, w1\n"
      "    b.ne .Latomic_2_3_5_failure\n"
      "    stxr w6, w2, [x0]\n"
      "    cbnz w6, .Latomic_2_3_5_retry\n"
      "    dmb ishld\n"
      "    .Latomic_2_3_5_failure:\n"
      "    clrex\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "atomic compare-exchange structured printer");
}

int selected_atomic_records_reject_missing_printer_facts() {
  auto rmw_record = base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::RmwLoop,
                                       6,
                                       bir::AtomicOrdering::AcqRel);
  rmw_record.result_mode = bir::AtomicResultMode::OldValue;
  rmw_record.result_value_id = prepare::PreparedValueId{560};
  rmw_record.result_value_name = c4c::ValueNameId{561};
  rmw_record.result_register = wreg(4);
  rmw_record.stored_value_id = prepare::PreparedValueId{562};
  rmw_record.stored_value_name = c4c::ValueNameId{563};
  rmw_record.stored_register = wreg(2);
  rmw_record.rmw_opcode = bir::AtomicRmwOpcode::Add;
  rmw_record.exclusive_retry_loop = true;

  const auto rmw = aarch64_codegen::make_atomic_memory_instruction(rmw_record);
  const auto rmw_result = aarch64_codegen::print_machine_instruction_line_payloads(rmw);
  if (rmw_result.ok ||
      rmw_result.diagnostic.find("scratch, status") == std::string::npos) {
    return fail("expected atomic rmw without loop scratch facts to fail closed");
  }

  auto compare_record =
      base_atomic_record(aarch64_codegen::AtomicMemoryInstructionKind::CompareExchangeLoop,
                         7,
                         bir::AtomicOrdering::SeqCst);
  compare_record.result_mode = bir::AtomicResultMode::BooleanSuccess;
  compare_record.result_value_id = prepare::PreparedValueId{570};
  compare_record.result_value_name = c4c::ValueNameId{571};
  compare_record.result_register = wreg(3);
  compare_record.expected_value_id = prepare::PreparedValueId{572};
  compare_record.expected_value_name = c4c::ValueNameId{573};
  compare_record.expected_register = wreg(1);
  compare_record.desired_value_id = prepare::PreparedValueId{574};
  compare_record.desired_value_name = c4c::ValueNameId{575};
  compare_record.desired_register = wreg(2);
  compare_record.failure_ordering = bir::AtomicOrdering::Acquire;
  compare_record.exclusive_retry_loop = true;
  compare_record.compare_exchange_failure_clears_monitor = true;
  compare_record.compare_exchange_result_is_boolean = true;
  compare_record.exclusive_status_register = wreg(6);

  const auto compare = aarch64_codegen::make_atomic_memory_instruction(compare_record);
  const auto compare_result =
      aarch64_codegen::print_machine_instruction_line_payloads(compare);
  if (compare_result.ok ||
      compare_result.diagnostic.find("loaded-value register") == std::string::npos) {
    return fail("expected atomic compare-exchange without loaded register to fail closed");
  }
  return 0;
}

int selected_scalar_add_sub_and_register_return_print_from_structured_operands() {
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{40},
          .result_value_name = c4c::ValueNameId{41},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .lhs = aarch64_codegen::make_register_operand(xreg(1)),
          .rhs = aarch64_codegen::make_register_operand(xreg(2)),
          .supported_integer_operation = true,
      }));
  const auto sub = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{42},
          .result_value_name = c4c::ValueNameId{43},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_register_operand(wreg(2)),
          .supported_integer_operation = true,
      }));
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_register_operand(wreg(0)),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = print_common_instruction_nodes({add, sub, ret});
  if (!result.ok) {
    return fail("expected scalar add/sub and register return to print from structured operands: " +
                result.diagnostic);
  }
  const auto add_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(add);
  const auto sub_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(sub);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const std::string expected =
      "    " + std::string(add_mnemonic) + " x0, x1, x2\n" +
      "    " + std::string(sub_mnemonic) + " w0, w1, w2\n" +
      "    " + std::string(return_mnemonic) + "\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    add x0, x1, x2\n"
                                        "    sub w0, w1, w2\n"
                                        "    ret\n",
                                        "scalar add/sub common-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(add_mnemonic, "add", "add helper mnemonic"); check != 0) {
    return check;
  }
  if (const int check = expect_equal(sub_mnemonic, "sub", "sub helper mnemonic"); check != 0) {
    return check;
  }
  if (const int check =
          expect_equal(return_mnemonic, "ret", "register return helper primary mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_scalar_fpr_register_return_publishes_abi_register() {
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_register_operand(sreg(13)),
          .value_type = bir::TypeKind::F32,
      });

  const auto result = print_common_instruction_nodes({ret});
  if (!result.ok) {
    return fail("expected scalar FPR register return to print ABI publication: " +
                result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    fmov s0, s13\n"
                         "    ret\n",
                         "    fmov s0, s13\n"
                         "    ret\n",
                         "scalar FPR return ABI publication");
}

int selected_scalar_stack_publication_materializes_large_offset() {
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{240},
          .result_value_name = c4c::ValueNameId{241},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(9),
          .result_stack_offset_bytes = 1644,
          .lhs = aarch64_codegen::make_register_operand(xreg(1)),
          .rhs = aarch64_codegen::make_register_operand(xreg(2)),
          .supported_integer_operation = true,
      }));

  const auto result = print_common_instruction_nodes({add});
  if (!result.ok) {
    return fail("expected scalar stack publication to materialize large offset: " +
                result.diagnostic);
  }
  const std::string expected =
      "    add x9, x1, x2\n"
      "    add x10, sp, #1644\n"
      "    str x9, [x10]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "scalar stack publication large-offset materialization");
}

int selected_scalar_alu_frame_slot_operands_materialize_before_printing() {
  auto rhs_slot = frame_slot(32);
  rhs_slot.size_bytes = 4;
  rhs_slot.align_bytes = 4;
  rhs_slot.result_value_id = prepare::PreparedValueId{251};
  rhs_slot.result_value_name = c4c::ValueNameId{252};
  const auto mul = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Mul,
          .source_binary_opcode = bir::BinaryOpcode::Mul,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{250},
          .result_value_name = c4c::ValueNameId{251},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_memory_operand(rhs_slot),
          .supported_integer_operation = true,
      }));

  auto lhs_slot = frame_slot(40);
  lhs_slot.result_value_id = prepare::PreparedValueId{260};
  lhs_slot.result_value_name = c4c::ValueNameId{261};
  auto add_rhs_slot = frame_slot(48);
  add_rhs_slot.frame_slot_id = prepare::PreparedFrameSlotId{5};
  add_rhs_slot.result_value_id = prepare::PreparedValueId{262};
  add_rhs_slot.result_value_name = c4c::ValueNameId{263};
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{264},
          .result_value_name = c4c::ValueNameId{265},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(13),
          .lhs = aarch64_codegen::make_memory_operand(lhs_slot),
          .rhs = aarch64_codegen::make_memory_operand(add_rhs_slot),
          .supported_integer_operation = true,
      }));

  auto mul_lhs_slot = frame_slot(56);
  mul_lhs_slot.result_value_id = prepare::PreparedValueId{270};
  mul_lhs_slot.result_value_name = c4c::ValueNameId{271};
  auto mul_rhs_slot = frame_slot(64);
  mul_rhs_slot.frame_slot_id = prepare::PreparedFrameSlotId{6};
  mul_rhs_slot.result_value_id = prepare::PreparedValueId{272};
  mul_rhs_slot.result_value_name = c4c::ValueNameId{273};
  const auto stack_mul = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Mul,
          .source_binary_opcode = bir::BinaryOpcode::Mul,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{274},
          .result_value_name = c4c::ValueNameId{275},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(9),
          .result_stack_offset_bytes = 72,
          .lhs = aarch64_codegen::make_memory_operand(mul_lhs_slot),
          .rhs = aarch64_codegen::make_memory_operand(mul_rhs_slot),
          .supported_integer_operation = true,
      }));

  const auto result = print_common_instruction_nodes({mul, add, stack_mul});
  if (!result.ok) {
    return fail("expected scalar ALU frame-slot operands to print: " + result.diagnostic);
  }
  const std::string expected =
      "    ldr w9, [sp, #32]\n"
      "    mul w0, w1, w9\n"
      "    ldr x9, [sp, #40]\n"
      "    ldr x10, [sp, #48]\n"
      "    add x13, x9, x10\n"
      "    ldr w9, [sp, #56]\n"
      "    ldr w10, [sp, #64]\n"
      "    mul w9, w9, w10\n"
      "    str w9, [sp, #72]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "scalar ALU frame-slot operand materialization");
}

int selected_unsigned_power_of_two_reductions_print_from_structured_operands() {
  const auto udiv = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
          .source_binary_opcode = bir::BinaryOpcode::UDiv,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{140},
          .result_value_name = c4c::ValueNameId{141},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .lhs = aarch64_codegen::make_register_operand(xreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I64,
                  .signed_value = 3,
                  .unsigned_value = 3,
              }),
          .supported_integer_operation = true,
      }));
  const auto urem = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::And,
          .source_binary_opcode = bir::BinaryOpcode::URem,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{142},
          .result_value_name = c4c::ValueNameId{143},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(2),
          .lhs = aarch64_codegen::make_register_operand(wreg(3)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 15,
                  .unsigned_value = 15,
              }),
          .supported_integer_operation = true,
      }));

  const auto result = print_common_instruction_nodes({udiv, urem});
  if (!result.ok) {
    return fail("expected unsigned power-of-two reductions to print: " + result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    lsr x0, x1, #3\n"
                         "    and w2, w3, #15\n",
                         "    lsr x0, x1, #3\n"
                         "    and w2, w3, #15\n",
                         "unsigned power-of-two reduction structured printer");
}

int narrow_unsigned_reductions_print_explicit_zero_extension() {
  const auto udiv8 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
          .source_binary_opcode = bir::BinaryOpcode::UDiv,
          .operand_type = bir::TypeKind::I8,
          .result_value_id = prepare::PreparedValueId{150},
          .result_value_name = c4c::ValueNameId{151},
          .result_type = bir::TypeKind::I8,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I8,
                  .signed_value = 2,
                  .unsigned_value = 2,
              }),
          .post_zero_extend_result_bits = 8U,
          .supported_integer_operation = true,
      }));
  const auto urem16 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::And,
          .source_binary_opcode = bir::BinaryOpcode::URem,
          .operand_type = bir::TypeKind::I16,
          .result_value_id = prepare::PreparedValueId{152},
          .result_value_name = c4c::ValueNameId{153},
          .result_type = bir::TypeKind::I16,
          .result_register = wreg(2),
          .lhs = aarch64_codegen::make_register_operand(wreg(3)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I16,
                  .signed_value = 15,
                  .unsigned_value = 15,
              }),
          .post_zero_extend_result_bits = 16U,
          .supported_integer_operation = true,
      }));
  const auto plain_i32 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
          .source_binary_opcode = bir::BinaryOpcode::UDiv,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{154},
          .result_value_name = c4c::ValueNameId{155},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(4),
          .lhs = aarch64_codegen::make_register_operand(wreg(5)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 3,
                  .unsigned_value = 3,
              }),
          .supported_integer_operation = true,
      }));

  const auto result = print_common_instruction_nodes({udiv8, urem16, plain_i32});
  if (!result.ok) {
    return fail("expected narrow unsigned reductions to print with post zero-extension: " +
                result.diagnostic);
  }
  if (const int check = expect_assembly(result.assembly,
                                        "    lsr w0, w1, #2\n"
                                        "    ubfx w0, w0, #0, #8\n"
                                        "    and w2, w3, #15\n"
                                        "    ubfx w2, w2, #0, #16\n"
                                        "    lsr w4, w5, #3\n",
                                        "    lsr w0, w1, #2\n"
                                        "    ubfx w0, w0, #0, #8\n"
                                        "    and w2, w3, #15\n"
                                        "    ubfx w2, w2, #0, #16\n"
                                        "    lsr w4, w5, #3\n",
                                        "narrow unsigned reduction post-extension printer");
      check != 0) {
    return check;
  }

  const auto bad_extension = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
          .source_binary_opcode = bir::BinaryOpcode::UDiv,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{156},
          .result_value_name = c4c::ValueNameId{157},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(6),
          .lhs = aarch64_codegen::make_register_operand(wreg(7)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 1,
                  .unsigned_value = 1,
              }),
          .post_zero_extend_result_bits = 32U,
          .supported_integer_operation = true,
      }));
  const auto bad_result = aarch64_codegen::print_machine_instruction_line_payloads(bad_extension);
  if (bad_result.ok ||
      bad_result.diagnostic.find("unsupported post-zero-extension width") ==
          std::string::npos) {
    return fail("expected invalid unsigned reduction post-extension fact to fail closed");
  }

  return 0;
}

int selected_unsigned_reductions_materialize_lhs_sources() {
  const auto immediate_lhs = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::LogicalShiftRight,
          .source_binary_opcode = bir::BinaryOpcode::UDiv,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{170},
          .result_value_name = c4c::ValueNameId{171},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 32,
                  .unsigned_value = 32,
              }),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 3,
                  .unsigned_value = 3,
              }),
          .supported_integer_operation = true,
      }));
  const auto memory_lhs = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::And,
          .source_binary_opcode = bir::BinaryOpcode::URem,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{172},
          .result_value_name = c4c::ValueNameId{173},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(2),
          .lhs = aarch64_codegen::make_memory_operand(frame_slot(16)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::UnsignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 15,
                  .unsigned_value = 15,
              }),
          .supported_integer_operation = true,
      }));

  const auto result =
      print_common_instruction_nodes({immediate_lhs, memory_lhs});
  if (!result.ok) {
    return fail("expected unsigned reductions to materialize lhs sources: " +
                result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    mov w9, #32\n"
                         "    lsr w0, w9, #3\n"
                         "    ldr w9, [sp, #16]\n"
                         "    and w2, w9, #15\n",
                         "    mov w9, #32\n"
                         "    lsr w0, w9, #3\n"
                         "    ldr w9, [sp, #16]\n"
                         "    and w2, w9, #15\n",
                         "unsigned reduction lhs materialization printer");
}

int signed_i32_add_sub_results_print_explicit_sign_extension() {
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{158},
          .result_value_name = c4c::ValueNameId{159},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_register_operand(wreg(2)),
          .post_sign_extend_result_bits = 32U,
          .supported_integer_operation = true,
      }));
  const auto sub = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{160},
          .result_value_name = c4c::ValueNameId{161},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(3),
          .lhs = aarch64_codegen::make_register_operand(wreg(4)),
          .rhs = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 7,
                  .unsigned_value = 7,
              }),
          .post_sign_extend_result_bits = 32U,
          .supported_integer_operation = true,
      }));

  const auto result = print_common_instruction_nodes({add, sub});
  if (!result.ok) {
    return fail("expected signed I32 add/sub results to print with post sign-extension: " +
                result.diagnostic);
  }
  if (const int check = expect_assembly(result.assembly,
                                        "    add w0, w1, w2\n"
                                        "    sxtw x0, w0\n"
                                        "    sub w3, w4, #7\n"
                                        "    sxtw x3, w3\n",
                                        "    add w0, w1, w2\n"
                                        "    sxtw x0, w0\n"
                                        "    sub w3, w4, #7\n"
                                        "    sxtw x3, w3\n",
                                        "signed I32 ALU post-extension printer");
      check != 0) {
    return check;
  }

  const auto bad_extension = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I16,
          .result_value_id = prepare::PreparedValueId{162},
          .result_value_name = c4c::ValueNameId{163},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(5),
          .lhs = aarch64_codegen::make_register_operand(wreg(6)),
          .rhs = aarch64_codegen::make_register_operand(wreg(7)),
          .post_sign_extend_result_bits = 16U,
          .supported_integer_operation = true,
      }));
  const auto bad_result =
      aarch64_codegen::print_machine_instruction_line_payloads(bad_extension);
  if (bad_result.ok ||
      bad_result.diagnostic.find("unsupported post-sign-extension width") ==
          std::string::npos) {
    return fail("expected invalid signed ALU post-extension fact to fail closed");
  }

  return 0;
}

int selected_scalar_unary_integer_ops_print_from_structured_operands() {
  const auto neg32 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::Neg,
              .operand_type = bir::TypeKind::I32,
              .result_value_id = prepare::PreparedValueId{44},
              .result_value_name = c4c::ValueNameId{45},
              .result_type = bir::TypeKind::I32,
              .result_register = wreg(0),
              .operand = aarch64_codegen::make_register_operand(wreg(1)),
              .supported_integer_operation = true,
          }));
  const auto not64 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::BitNot,
              .operand_type = bir::TypeKind::I64,
              .result_value_id = prepare::PreparedValueId{46},
              .result_value_name = c4c::ValueNameId{47},
              .result_type = bir::TypeKind::I64,
              .result_register = xreg(2),
              .operand = aarch64_codegen::make_register_operand(xreg(3)),
              .supported_integer_operation = true,
          }));
  const auto clz32 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::CountLeadingZeros,
              .operand_type = bir::TypeKind::I32,
              .result_value_id = prepare::PreparedValueId{48},
              .result_value_name = c4c::ValueNameId{49},
              .result_type = bir::TypeKind::I32,
              .result_register = wreg(4),
              .operand = aarch64_codegen::make_register_operand(wreg(5)),
              .supported_integer_operation = true,
          }));
  const auto clz64 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::CountLeadingZeros,
              .operand_type = bir::TypeKind::I64,
              .result_value_id = prepare::PreparedValueId{50},
              .result_value_name = c4c::ValueNameId{51},
              .result_type = bir::TypeKind::I64,
              .result_register = xreg(6),
              .operand = aarch64_codegen::make_register_operand(xreg(7)),
              .supported_integer_operation = true,
          }));
  const auto ctz32 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::CountTrailingZeros,
              .operand_type = bir::TypeKind::I32,
              .result_value_id = prepare::PreparedValueId{52},
              .result_value_name = c4c::ValueNameId{53},
              .result_type = bir::TypeKind::I32,
              .result_register = wreg(8),
              .operand = aarch64_codegen::make_register_operand(wreg(9)),
              .supported_integer_operation = true,
          }));
  const auto ctz64 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::CountTrailingZeros,
              .operand_type = bir::TypeKind::I64,
              .result_value_id = prepare::PreparedValueId{54},
              .result_value_name = c4c::ValueNameId{55},
              .result_type = bir::TypeKind::I64,
              .result_register = xreg(10),
              .operand = aarch64_codegen::make_register_operand(xreg(11)),
              .supported_integer_operation = true,
          }));
  const auto bswap16 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::ByteSwap,
              .operand_type = bir::TypeKind::I16,
              .result_value_id = prepare::PreparedValueId{56},
              .result_value_name = c4c::ValueNameId{57},
              .result_type = bir::TypeKind::I16,
              .result_register = wreg(12),
              .operand = aarch64_codegen::make_register_operand(wreg(13)),
              .supported_integer_operation = true,
          }));
  const auto bswap32 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::ByteSwap,
              .operand_type = bir::TypeKind::I32,
              .result_value_id = prepare::PreparedValueId{58},
              .result_value_name = c4c::ValueNameId{59},
              .result_type = bir::TypeKind::I32,
              .result_register = wreg(14),
              .operand = aarch64_codegen::make_register_operand(wreg(15)),
              .supported_integer_operation = true,
          }));
  const auto bswap64 = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_unary_instruction_record(
          aarch64_codegen::ScalarUnaryRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .operation = aarch64_codegen::ScalarUnaryOperationKind::ByteSwap,
              .operand_type = bir::TypeKind::I64,
              .result_value_id = prepare::PreparedValueId{60},
              .result_value_name = c4c::ValueNameId{61},
              .result_type = bir::TypeKind::I64,
              .result_register = xreg(16),
              .operand = aarch64_codegen::make_register_operand(xreg(17)),
              .supported_integer_operation = true,
          }));

  const auto result = print_common_instruction_nodes(
      {neg32, not64, clz32, clz64, ctz32, ctz64, bswap16, bswap32, bswap64});
  if (!result.ok) {
    return fail("expected scalar unary integer operations to print: " + result.diagnostic);
  }
  const std::string expected =
      "    neg w0, w1\n"
      "    mvn x2, x3\n"
      "    clz w4, w5\n"
      "    clz x6, x7\n"
      "    rbit w8, w9\n"
      "    clz w8, w8\n"
      "    rbit x10, x11\n"
      "    clz x10, x10\n"
      "    rev w12, w13\n"
      "    lsr w12, w12, #16\n"
      "    rev w14, w15\n"
      "    rev x16, x17\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "scalar unary integer common-printer drift guard");
}

int selected_simple_integer_casts_print_from_structured_operands() {
  const auto sext = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignExtend,
          .source_cast_opcode = bir::CastOpcode::SExt,
          .source_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{52},
          .result_value_name = c4c::ValueNameId{53},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .source = aarch64_codegen::make_register_operand(wreg(1)),
          .supported_simple_integer_cast = true,
      }));
  const auto zext = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::ZeroExtend,
          .source_cast_opcode = bir::CastOpcode::ZExt,
          .source_type = bir::TypeKind::I1,
          .result_value_id = prepare::PreparedValueId{54},
          .result_value_name = c4c::ValueNameId{55},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(2),
          .source = aarch64_codegen::make_register_operand(wreg(3)),
          .supported_simple_integer_cast = true,
      }));
  const auto trunc = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::Truncate,
          .source_cast_opcode = bir::CastOpcode::Trunc,
          .source_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{56},
          .result_value_name = c4c::ValueNameId{57},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(4),
          .source = aarch64_codegen::make_register_operand(xreg(5)),
          .supported_simple_integer_cast = true,
      }));
  const auto promoted_truth_zext = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::ZeroExtend,
          .source_cast_opcode = bir::CastOpcode::ZExt,
          .source_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{58},
          .result_value_name = c4c::ValueNameId{59},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(6),
          .source = aarch64_codegen::make_register_operand(wreg(7)),
          .supported_simple_integer_cast = true,
      }));

  const auto result =
      print_common_instruction_nodes({sext, zext, trunc, promoted_truth_zext});
  if (!result.ok) {
    return fail("expected simple integer casts to print from structured operands: " +
                result.diagnostic);
  }
  const std::string expected =
      "    sxtw x0, w1\n"
      "    ubfx w2, w3, #0, #1\n"
      "    mov w4, w5\n"
      "    mov w6, w7\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "simple integer cast common-printer drift guard");
}

int selected_fp_arithmetic_prints_from_structured_operands() {
  const auto fadd = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::F32,
          .result_value_id = prepare::PreparedValueId{58},
          .result_value_name = c4c::ValueNameId{59},
          .result_type = bir::TypeKind::F32,
          .result_register = sreg(0),
          .lhs = aarch64_codegen::make_register_operand(sreg(1)),
          .rhs = aarch64_codegen::make_register_operand(sreg(2)),
          .supported_floating_operation = true,
      }));
  const auto fsub = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::F64,
          .result_value_id = prepare::PreparedValueId{60},
          .result_value_name = c4c::ValueNameId{61},
          .result_type = bir::TypeKind::F64,
          .result_register = dreg(3),
          .lhs = aarch64_codegen::make_register_operand(dreg(4)),
          .rhs = aarch64_codegen::make_register_operand(dreg(5)),
          .supported_floating_operation = true,
      }));
  const auto fmul = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Mul,
          .source_binary_opcode = bir::BinaryOpcode::Mul,
          .operand_type = bir::TypeKind::F32,
          .result_value_id = prepare::PreparedValueId{62},
          .result_value_name = c4c::ValueNameId{63},
          .result_type = bir::TypeKind::F32,
          .result_register = sreg(6),
          .lhs = aarch64_codegen::make_register_operand(sreg(7)),
          .rhs = aarch64_codegen::make_register_operand(sreg(8)),
          .supported_floating_operation = true,
      }));
  const auto fdiv = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Div,
          .source_binary_opcode = bir::BinaryOpcode::SDiv,
          .operand_type = bir::TypeKind::F64,
          .result_value_id = prepare::PreparedValueId{64},
          .result_value_name = c4c::ValueNameId{65},
          .result_type = bir::TypeKind::F64,
          .result_register = dreg(9),
          .lhs = aarch64_codegen::make_register_operand(dreg(10)),
          .rhs = aarch64_codegen::make_register_operand(dreg(11)),
          .supported_floating_operation = true,
      }));
  const auto result = print_common_instruction_nodes({fadd, fsub, fmul, fdiv});
  if (!result.ok) {
    return fail("expected selected FP arithmetic to print: " + result.diagnostic);
  }
  const std::string expected =
      "    fadd s0, s1, s2\n"
      "    fsub d3, d4, d5\n"
      "    fmul s6, s7, s8\n"
      "    fdiv d9, d10, d11\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "FP arithmetic common-printer drift guard");
}

int selected_scalar_fp_unary_fabs_intrinsics_print_from_structured_records() {
  const auto f32 = aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(
      scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(6), sreg(7)));
  const auto f64 = aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(
      scalar_fp_unary_fabs_record(bir::TypeKind::F64, dreg(8), dreg(9)));
  const auto result = print_common_instruction_nodes({f32, f64});
  if (!result.ok) {
    return fail("expected selected scalar fabs intrinsic records to print: " +
                result.diagnostic);
  }
  const std::string expected =
      "    fabs s6, s7\n"
      "    fabs d8, d9\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "scalar fabs intrinsic structured printer");
}

int selected_scalar_fp_unary_fabs_intrinsics_reject_incomplete_printer_facts() {
  auto missing_result = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(0), sreg(1));
  missing_result.result_register = std::nullopt;
  const auto missing_result_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(missing_result);
  const auto missing_result_print =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_result_instruction);
  if (missing_result_print.ok ||
      missing_result_print.diagnostic.find("missing operand or result register authority") ==
          std::string::npos) {
    return fail("expected fabs intrinsic without result register authority to fail closed");
  }

  auto wrong_bank = scalar_fp_unary_fabs_record(bir::TypeKind::F64, dreg(2), dreg(3));
  wrong_bank.result_register = xreg(2);
  wrong_bank.result_register->value_id = prepare::PreparedValueId{83};
  wrong_bank.result_register->value_name = c4c::ValueNameId{83};
  const auto wrong_bank_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(wrong_bank);
  const auto wrong_bank_print =
      aarch64_codegen::print_machine_instruction_line_payloads(wrong_bank_instruction);
  if (wrong_bank_print.ok ||
      wrong_bank_print.diagnostic.find("incomplete printable FPR register facts") ==
          std::string::npos) {
    return fail("expected fabs intrinsic with non-FPR result register to fail closed");
  }

  auto side_effecting = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(4), sreg(5));
  side_effecting.has_side_effects = true;
  side_effecting.requires_feature = true;
  auto side_effecting_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(side_effecting);
  side_effecting_instruction.selection.status =
      aarch64_codegen::MachineNodeSelectionStatus::Selected;
  side_effecting_instruction.selection.diagnostic = {};
  const auto side_effecting_print =
      aarch64_codegen::print_machine_instruction_line_payloads(side_effecting_instruction);
  if (side_effecting_print.ok ||
      side_effecting_print.diagnostic.find("side-effect-free feature-free fabs") ==
          std::string::npos) {
    return fail("expected feature or side-effect fabs record to fail closed in printer");
  }

  auto f128 = scalar_fp_unary_fabs_record(bir::TypeKind::F64, dreg(6), dreg(7));
  f128.operand_type = bir::TypeKind::F128;
  f128.result_type = bir::TypeKind::F128;
  const auto f128_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(f128);
  const auto f128_print =
      aarch64_codegen::print_machine_instruction_line_payloads(f128_instruction);
  if (f128_print.ok ||
      f128_print.diagnostic.find("type is outside the selected subset") ==
          std::string::npos) {
    return fail("expected F128 fabs-shaped intrinsic record to fail closed");
  }

  auto non_selected = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(8), sreg(9));
  non_selected.has_prepared_call_plan = false;
  const auto non_selected_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(non_selected);
  const auto non_selected_print =
      aarch64_codegen::print_machine_instruction_line_payloads(non_selected_instruction);
  if (non_selected_print.ok ||
      non_selected_print.diagnostic.find("requires prepared call-plan authority") ==
          std::string::npos) {
    return fail("expected non-selected fabs intrinsic record to fail closed");
  }

  return 0;
}

int selected_crc_vector_intrinsics_print_from_structured_records() {
  const auto crc = aarch64_codegen::make_crc32w_intrinsic_instruction(
      crc32w_record(wreg(2), wreg(0), wreg(1)));
  const auto load = aarch64_codegen::make_vector_load_intrinsic_instruction(
      vector_load_record(qreg(4), xreg(5)));
  const auto add = aarch64_codegen::make_vector_add_intrinsic_instruction(
      vector_add_record(qreg(6), qreg(7), qreg(8)));
  const auto result = print_common_instruction_nodes({crc, load, add});
  if (!result.ok) {
    return fail("expected selected CRC/vector intrinsic records to print: " +
                result.diagnostic);
  }
  const std::string expected =
      "    crc32w w2, w0, w1\n"
      "    ld1 {v4.16b}, [x5]\n"
      "    add v6.16b, v7.16b, v8.16b\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "CRC/vector intrinsic structured printer");
}

int selected_crc_vector_intrinsics_reject_incomplete_printer_facts() {
  auto crc_missing_result = crc32w_record(wreg(2), wreg(0), wreg(1));
  crc_missing_result.result_register = std::nullopt;
  const auto crc_missing_result_instruction =
      aarch64_codegen::make_crc32w_intrinsic_instruction(crc_missing_result);
  const auto crc_missing_result_print =
      aarch64_codegen::print_machine_instruction_line_payloads(
          crc_missing_result_instruction);
  if (crc_missing_result_print.ok ||
      crc_missing_result_print.diagnostic.find("missing operand or result register authority") ==
          std::string::npos ||
      !crc_missing_result_print.instruction_lines.empty()) {
    return fail("expected CRC32W record without result authority to fail closed");
  }

  auto crc_wrong_register_bank = crc32w_record(wreg(2), wreg(0), wreg(1));
  crc_wrong_register_bank.data = aarch64_codegen::make_register_operand(qreg(3));
  auto crc_wrong_register_bank_instruction =
      aarch64_codegen::make_crc32w_intrinsic_instruction(crc_wrong_register_bank);
  crc_wrong_register_bank_instruction.selection.status =
      aarch64_codegen::MachineNodeSelectionStatus::Selected;
  crc_wrong_register_bank_instruction.selection.diagnostic = {};
  const auto crc_wrong_register_bank_print =
      aarch64_codegen::print_machine_instruction_line_payloads(
          crc_wrong_register_bank_instruction);
  if (crc_wrong_register_bank_print.ok ||
      crc_wrong_register_bank_print.diagnostic.find("printable W-register facts") ==
          std::string::npos ||
      !crc_wrong_register_bank_print.instruction_lines.empty()) {
    return fail("expected CRC32W record with non-W data register to fail closed");
  }

  auto load_missing_memory = vector_load_record(qreg(4), xreg(5));
  load_missing_memory.memory.base_register = std::nullopt;
  const auto load_missing_memory_instruction =
      aarch64_codegen::make_vector_load_intrinsic_instruction(load_missing_memory);
  const auto load_missing_memory_print =
      aarch64_codegen::print_machine_instruction_line_payloads(
          load_missing_memory_instruction);
  if (load_missing_memory_print.ok ||
      load_missing_memory_print.diagnostic.find(
          "requires pointer memory and result register authority") ==
          std::string::npos ||
      !load_missing_memory_print.instruction_lines.empty()) {
    return fail("expected vector-load record without memory base register to fail closed");
  }

  auto load_nonzero_offset = vector_load_record(qreg(4), xreg(5));
  load_nonzero_offset.memory.byte_offset = 16;
  auto load_nonzero_offset_instruction =
      aarch64_codegen::make_vector_load_intrinsic_instruction(load_nonzero_offset);
  load_nonzero_offset_instruction.selection.status =
      aarch64_codegen::MachineNodeSelectionStatus::Selected;
  load_nonzero_offset_instruction.selection.diagnostic = {};
  const auto load_nonzero_offset_print =
      aarch64_codegen::print_machine_instruction_line_payloads(
          load_nonzero_offset_instruction);
  if (load_nonzero_offset_print.ok ||
      load_nonzero_offset_print.diagnostic.find(
          "requires pointer memory and result register authority") ==
          std::string::npos ||
      !load_nonzero_offset_print.instruction_lines.empty()) {
    return fail("expected vector-load record with offset addressing to fail closed");
  }

  auto add_wrong_shape = vector_add_record(qreg(6), qreg(7), qreg(8));
  add_wrong_shape.vector_lane_count = 8;
  const auto add_wrong_shape_instruction =
      aarch64_codegen::make_vector_add_intrinsic_instruction(add_wrong_shape);
  const auto add_wrong_shape_print =
      aarch64_codegen::print_machine_instruction_line_payloads(
          add_wrong_shape_instruction);
  if (add_wrong_shape_print.ok ||
      add_wrong_shape_print.diagnostic.find("missing v16i8 shape authority") ==
          std::string::npos ||
      !add_wrong_shape_print.instruction_lines.empty()) {
    return fail("expected vector-add record without v16i8 shape to fail closed");
  }

  auto add_wrong_register_bank = vector_add_record(qreg(6), qreg(7), qreg(8));
  add_wrong_register_bank.lhs = aarch64_codegen::make_register_operand(wreg(9));
  auto add_wrong_register_bank_instruction =
      aarch64_codegen::make_vector_add_intrinsic_instruction(add_wrong_register_bank);
  add_wrong_register_bank_instruction.selection.status =
      aarch64_codegen::MachineNodeSelectionStatus::Selected;
  add_wrong_register_bank_instruction.selection.diagnostic = {};
  const auto add_wrong_register_bank_print =
      aarch64_codegen::print_machine_instruction_line_payloads(
          add_wrong_register_bank_instruction);
  if (add_wrong_register_bank_print.ok ||
      add_wrong_register_bank_print.diagnostic.find("printable vector register facts") ==
          std::string::npos ||
      !add_wrong_register_bank_print.instruction_lines.empty()) {
    return fail("expected vector-add record with scalar lhs register to fail closed");
  }

  return 0;
}

int complete_unsupported_intrinsic_carriers_do_not_print_as_machine_records() {
  auto crc = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(0), sreg(1));
  crc.source_carrier = complete_crc32w_carrier();
  crc.family = bir::IntrinsicFamilyKind::Crc;
  crc.operation = bir::IntrinsicOperationKind::Crc32W;
  crc.operand_type = bir::TypeKind::I32;
  crc.result_type = bir::TypeKind::I32;
  crc.requires_feature = true;
  crc.source_callee_name = std::string{"llvm.aarch64.crc32w"};
  const auto crc_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(crc);
  const auto crc_print =
      aarch64_codegen::print_machine_instruction_line_payloads(crc_instruction);
  if (crc_print.ok ||
      crc_print.diagnostic.find("outside the selected scalar FP unary subset") ==
          std::string::npos ||
      !crc_print.instruction_lines.empty()) {
    return fail("expected complete CRC carrier not to print as an AArch64 machine record");
  }

  auto vector = scalar_fp_unary_fabs_record(bir::TypeKind::F32, qreg(0), qreg(1));
  vector.source_carrier = complete_vector_add_carrier();
  vector.family = bir::IntrinsicFamilyKind::VectorOperation;
  vector.operation = bir::IntrinsicOperationKind::VectorAdd;
  vector.operand_type = bir::TypeKind::I128;
  vector.result_type = bir::TypeKind::I128;
  vector.requires_feature = true;
  vector.source_callee_name = std::string{"llvm.aarch64.neon.add.v16i8"};
  const auto vector_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(vector);
  const auto vector_print =
      aarch64_codegen::print_machine_instruction_line_payloads(vector_instruction);
  if (vector_print.ok ||
      vector_print.diagnostic.find("outside the selected scalar FP unary subset") ==
          std::string::npos ||
      !vector_print.instruction_lines.empty()) {
    return fail("expected complete vector carrier not to print as an AArch64 machine record");
  }

  auto barrier = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(0), sreg(1));
  barrier.source_carrier = complete_barrier_dmb_carrier();
  barrier.family = bir::IntrinsicFamilyKind::Barrier;
  barrier.operation = bir::IntrinsicOperationKind::BarrierDmb;
  barrier.operand_type = bir::TypeKind::I32;
  barrier.result_type = bir::TypeKind::Void;
  barrier.source_callee_name = std::string{"llvm.aarch64.dmb"};
  barrier.has_side_effects = true;
  const auto barrier_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(barrier);
  const auto barrier_print =
      aarch64_codegen::print_machine_instruction_line_payloads(barrier_instruction);
  if (barrier_print.ok ||
      barrier_print.diagnostic.find("outside the selected scalar FP unary subset") ==
          std::string::npos ||
      !barrier_print.instruction_lines.empty()) {
    return fail("expected complete barrier DMB carrier not to print as an AArch64 machine record");
  }

  auto cache = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(0), sreg(1));
  static const prepare::PreparedIntrinsicCarrier cache_carrier{
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
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
      .source_callee_name = std::string{"llvm.aarch64.dc.cvau"},
      .has_prepared_call_plan = true,
  };
  cache.source_carrier = &cache_carrier;
  cache.family = bir::IntrinsicFamilyKind::CacheMaintenance;
  cache.operation = bir::IntrinsicOperationKind::CacheDcCvau;
  cache.operand_type = bir::TypeKind::Ptr;
  cache.result_type = bir::TypeKind::Void;
  cache.source_callee_name = std::string{"llvm.aarch64.dc.cvau"};
  cache.has_side_effects = true;
  const auto cache_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(cache);
  const auto cache_print =
      aarch64_codegen::print_machine_instruction_line_payloads(cache_instruction);
  if (cache_print.ok ||
      cache_print.diagnostic.find("outside the selected scalar FP unary subset") ==
          std::string::npos ||
      !cache_print.instruction_lines.empty()) {
    return fail("expected complete cache DC CVAU carrier not to print as an AArch64 machine record");
  }

  auto hint = scalar_fp_unary_fabs_record(bir::TypeKind::F32, sreg(0), sreg(1));
  static const prepare::PreparedIntrinsicCarrier hint_carrier{
      .carrier_kind = prepare::PreparedIntrinsicCarrierKind::Complete,
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
      .source_callee_name = std::string{"llvm.aarch64.hint"},
      .has_prepared_call_plan = true,
  };
  hint.source_carrier = &hint_carrier;
  hint.family = bir::IntrinsicFamilyKind::PauseHint;
  hint.operation = bir::IntrinsicOperationKind::HintYield;
  hint.operand_type = bir::TypeKind::I32;
  hint.result_type = bir::TypeKind::Void;
  hint.source_callee_name = std::string{"llvm.aarch64.hint"};
  hint.has_side_effects = true;
  const auto hint_instruction =
      aarch64_codegen::make_scalar_fp_unary_intrinsic_instruction(hint);
  const auto hint_print =
      aarch64_codegen::print_machine_instruction_line_payloads(hint_instruction);
  if (hint_print.ok ||
      hint_print.diagnostic.find("outside the selected scalar FP unary subset") ==
          std::string::npos ||
      !hint_print.instruction_lines.empty()) {
    return fail("expected complete hint yield carrier not to print as an AArch64 machine record");
  }

  return 0;
}

int selected_fp_conversions_print_from_structured_operands() {
  const auto sitofp = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignedIntToFloat,
          .source_cast_opcode = bir::CastOpcode::SIToFP,
          .source_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{66},
          .result_value_name = c4c::ValueNameId{67},
          .result_type = bir::TypeKind::F64,
          .result_register = dreg(0),
          .source = aarch64_codegen::make_register_operand(wreg(1)),
          .source_register_bank = prepare::PreparedRegisterBank::Gpr,
          .result_register_bank = prepare::PreparedRegisterBank::Fpr,
          .crosses_register_bank = true,
          .supported_float_integer_conversion = true,
      }));
  const auto uitofp = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::UnsignedIntToFloat,
          .source_cast_opcode = bir::CastOpcode::UIToFP,
          .source_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{68},
          .result_value_name = c4c::ValueNameId{69},
          .result_type = bir::TypeKind::F32,
          .result_register = sreg(2),
          .source = aarch64_codegen::make_register_operand(xreg(3)),
          .source_register_bank = prepare::PreparedRegisterBank::Gpr,
          .result_register_bank = prepare::PreparedRegisterBank::Fpr,
          .crosses_register_bank = true,
          .supported_float_integer_conversion = true,
      }));
  const auto fptosi = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::FloatToSignedInt,
          .source_cast_opcode = bir::CastOpcode::FPToSI,
          .source_type = bir::TypeKind::F32,
          .result_value_id = prepare::PreparedValueId{70},
          .result_value_name = c4c::ValueNameId{71},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(4),
          .source = aarch64_codegen::make_register_operand(sreg(5)),
          .source_register_bank = prepare::PreparedRegisterBank::Fpr,
          .result_register_bank = prepare::PreparedRegisterBank::Gpr,
          .crosses_register_bank = true,
          .supported_float_integer_conversion = true,
      }));
  const auto fptoui = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::FloatToUnsignedInt,
          .source_cast_opcode = bir::CastOpcode::FPToUI,
          .source_type = bir::TypeKind::F64,
          .result_value_id = prepare::PreparedValueId{72},
          .result_value_name = c4c::ValueNameId{73},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(6),
          .source = aarch64_codegen::make_register_operand(dreg(7)),
          .source_register_bank = prepare::PreparedRegisterBank::Fpr,
          .result_register_bank = prepare::PreparedRegisterBank::Gpr,
          .crosses_register_bank = true,
          .supported_float_integer_conversion = true,
      }));
  const auto fpext = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::FloatExtend,
          .source_cast_opcode = bir::CastOpcode::FPExt,
          .source_type = bir::TypeKind::F32,
          .result_value_id = prepare::PreparedValueId{74},
          .result_value_name = c4c::ValueNameId{75},
          .result_type = bir::TypeKind::F64,
          .result_register = dreg(8),
          .source = aarch64_codegen::make_register_operand(sreg(9)),
          .source_register_bank = prepare::PreparedRegisterBank::Fpr,
          .result_register_bank = prepare::PreparedRegisterBank::Fpr,
          .supported_float_width_conversion = true,
      }));
  const auto fptrunc = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::FloatTruncate,
          .source_cast_opcode = bir::CastOpcode::FPTrunc,
          .source_type = bir::TypeKind::F64,
          .result_value_id = prepare::PreparedValueId{76},
          .result_value_name = c4c::ValueNameId{77},
          .result_type = bir::TypeKind::F32,
          .result_register = sreg(10),
          .source = aarch64_codegen::make_register_operand(dreg(11)),
          .source_register_bank = prepare::PreparedRegisterBank::Fpr,
          .result_register_bank = prepare::PreparedRegisterBank::Fpr,
          .supported_float_width_conversion = true,
      }));
  const auto result = print_common_instruction_nodes(
      {sitofp, uitofp, fptosi, fptoui, fpext, fptrunc});
  if (!result.ok) {
    return fail("expected selected FP conversions to print: " + result.diagnostic);
  }
  const std::string expected =
      "    scvtf d0, w1\n"
      "    ucvtf s2, x3\n"
      "    fcvtzs x4, s5\n"
      "    fcvtzu w6, d7\n"
      "    fcvt d8, s9\n"
      "    fcvt s10, d11\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "FP conversion common-printer drift guard");
}

int selected_i128_records_print_from_structured_fields() {
  auto load_memory = frame_slot(32);
  load_memory.size_bytes = 16;
  load_memory.align_bytes = 16;
  const auto load_pair = i128_pair_operand(prepare::PreparedValueId{60},
                                           c4c::ValueNameId{60},
                                           6);
  const auto transport_load =
      aarch64_codegen::make_i128_transport_instruction(aarch64_codegen::I128TransportRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .transport_kind = aarch64_codegen::I128TransportKind::LoadFromMemory,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .value_id = prepare::PreparedValueId{60},
          .value_name = c4c::ValueNameId{60},
          .carrier_kind = prepare::PreparedI128CarrierKind::RegisterPair,
          .lane_width_bytes = 8,
          .total_size_bytes = 16,
          .total_align_bytes = 16,
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .register_class = prepare::PreparedRegisterClass::General,
          .low_lane = load_pair.low_lane,
          .high_lane = load_pair.high_lane,
          .memory = load_memory,
          .source_carrier = reinterpret_cast<const prepare::PreparedI128Carrier*>(0x1),
      });
  auto copy_record = aarch64_codegen::I128TransportRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .transport_kind = aarch64_codegen::I128TransportKind::CopyRegisterPair,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .value_id = prepare::PreparedValueId{73},
      .value_name = c4c::ValueNameId{73},
      .carrier_kind = prepare::PreparedI128CarrierKind::RegisterPair,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .low_lane =
          i128_pair_operand(prepare::PreparedValueId{73}, c4c::ValueNameId{73}, 26)
              .low_lane,
      .high_lane =
          i128_pair_operand(prepare::PreparedValueId{73}, c4c::ValueNameId{73}, 26)
              .high_lane,
      .source_value_id = prepare::PreparedValueId{74},
      .source_value_name = c4c::ValueNameId{74},
      .source_low_lane =
          i128_pair_operand(prepare::PreparedValueId{74}, c4c::ValueNameId{74}, 28)
              .low_lane,
      .source_high_lane =
          i128_pair_operand(prepare::PreparedValueId{74}, c4c::ValueNameId{74}, 28)
              .high_lane,
      .source_carrier = reinterpret_cast<const prepare::PreparedI128Carrier*>(0x1),
      .copy_source_carrier = reinterpret_cast<const prepare::PreparedI128Carrier*>(0x2),
  };
  const auto transport_copy =
      aarch64_codegen::make_i128_transport_instruction(copy_record);
  const auto pair_add = aarch64_codegen::make_i128_pair_operation_instruction(
      aarch64_codegen::I128PairOperationRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::I128PairOperationKind::Add,
          .lane_semantics = aarch64_codegen::I128PairLaneSemantics::CarryPropagating,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .result =
              i128_pair_operand(prepare::PreparedValueId{61}, c4c::ValueNameId{61}, 8),
          .lhs = i128_pair_operand(prepare::PreparedValueId{62}, c4c::ValueNameId{62}, 10),
          .rhs = i128_pair_operand(prepare::PreparedValueId{63}, c4c::ValueNameId{63}, 12),
      });
  const auto shift = aarch64_codegen::make_i128_shift_instruction(
      aarch64_codegen::I128ShiftRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .shift_kind = aarch64_codegen::I128ShiftKind::LogicalRight,
          .lane_semantics = aarch64_codegen::I128ShiftLaneSemantics::CrossLaneLogicalRight,
          .count_kind = aarch64_codegen::I128ShiftCountKind::Immediate,
          .source_binary_opcode = bir::BinaryOpcode::LShr,
          .result =
              i128_pair_operand(prepare::PreparedValueId{64}, c4c::ValueNameId{64}, 14),
          .source =
              i128_pair_operand(prepare::PreparedValueId{65}, c4c::ValueNameId{65}, 16),
          .shift_count =
              aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 12,
              }),
      });
  const auto compare = aarch64_codegen::make_i128_compare_instruction(
      aarch64_codegen::I128CompareRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .predicate = bir::BinaryOpcode::Eq,
          .signedness = aarch64_codegen::I128CompareSignedness::Equality,
          .high_word_semantics =
              aarch64_codegen::I128CompareHighWordSemantics::EqualityBothLanes,
          .result_value_id = prepare::PreparedValueId{66},
          .result_value_name = c4c::ValueNameId{66},
          .result_register = wreg(0),
          .lhs = i128_pair_operand(prepare::PreparedValueId{67}, c4c::ValueNameId{67}, 18),
          .rhs = i128_pair_operand(prepare::PreparedValueId{68}, c4c::ValueNameId{68}, 20),
      });
  const auto unsigned_compare = aarch64_codegen::make_i128_compare_instruction(
      aarch64_codegen::I128CompareRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .predicate = bir::BinaryOpcode::Ult,
          .signedness = aarch64_codegen::I128CompareSignedness::Unsigned,
          .high_word_semantics =
              aarch64_codegen::I128CompareHighWordSemantics::UnsignedHighWordFirst,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 9,
          .result_value_id = prepare::PreparedValueId{69},
          .result_value_name = c4c::ValueNameId{69},
          .result_register = wreg(1),
          .lhs = i128_pair_operand(prepare::PreparedValueId{70}, c4c::ValueNameId{70}, 22),
          .rhs = i128_pair_operand(prepare::PreparedValueId{71}, c4c::ValueNameId{71}, 24),
      });
  const auto result = print_common_instruction_nodes(
      {transport_load, transport_copy, pair_add, shift, compare, unsigned_compare});
  if (!result.ok) {
    return fail("expected selected i128 nodes to print from structured fields: " +
                result.diagnostic);
  }
  const std::string expected =
      "    ldp x6, x7, [sp, #32]\n"
      "    mov x26, x28\n"
      "    mov x27, x29\n"
      "    adds x8, x10, x12\n"
      "    adc x9, x11, x13\n"
      "    extr x14, x16, x17, #12\n"
      "    lsr x15, x17, #12\n"
      "    cmp x19, x21\n"
      "    ccmp x18, x20, #0, eq\n"
      "    cset w0, eq\n"
      "    cmp x23, x25\n"
      "    b.lo .L_i128cmp_2_3_9_true\n"
      "    b.hi .L_i128cmp_2_3_9_false\n"
      "    cmp x22, x24\n"
      "    b.lo .L_i128cmp_2_3_9_true\n"
      "    b .L_i128cmp_2_3_9_false\n"
      "    .L_i128cmp_2_3_9_true:\n"
      "    mov w1, #1\n"
      "    b .L_i128cmp_2_3_9_done\n"
      "    .L_i128cmp_2_3_9_false:\n"
      "    mov w1, #0\n"
      "    .L_i128cmp_2_3_9_done:\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "i128 structured common-printer drift guard");
}

int selected_i128_helper_boundaries_print_from_structured_fields() {
  auto signed_helper = printable_i128_helper(bir::BinaryOpcode::SDiv,
                                             prepare::PreparedI128RuntimeHelperKind::SignedDiv,
                                             "__divti3",
                                             prepare::PreparedValueId{70},
                                             c4c::ValueNameId{70},
                                             6,
                                             prepare::PreparedValueId{71},
                                             c4c::ValueNameId{71},
                                             8,
                                             prepare::PreparedValueId{72},
                                             c4c::ValueNameId{72},
                                             10);
  auto unsigned_helper = printable_i128_helper(
      bir::BinaryOpcode::URem,
      prepare::PreparedI128RuntimeHelperKind::UnsignedRem,
      "__umodti3",
      prepare::PreparedValueId{80},
      c4c::ValueNameId{80},
      12,
      prepare::PreparedValueId{81},
      c4c::ValueNameId{81},
      14,
      prepare::PreparedValueId{82},
      c4c::ValueNameId{82},
      16);
  const auto signed_record =
      aarch64_codegen::make_i128_runtime_helper_boundary_instruction(
          i128_helper_record(&signed_helper,
                             bir::BinaryOpcode::SDiv,
                             prepare::PreparedI128RuntimeHelperKind::SignedDiv,
                             "__divti3",
                             prepare::PreparedValueId{70},
                             c4c::ValueNameId{70},
                             6,
                             prepare::PreparedValueId{71},
                             c4c::ValueNameId{71},
                             8,
                             prepare::PreparedValueId{72},
                             c4c::ValueNameId{72},
                             10));
  const auto unsigned_record =
      aarch64_codegen::make_i128_runtime_helper_boundary_instruction(
          i128_helper_record(&unsigned_helper,
                             bir::BinaryOpcode::URem,
                             prepare::PreparedI128RuntimeHelperKind::UnsignedRem,
                             "__umodti3",
                             prepare::PreparedValueId{80},
                             c4c::ValueNameId{80},
                             12,
                             prepare::PreparedValueId{81},
                             c4c::ValueNameId{81},
                             14,
                             prepare::PreparedValueId{82},
                             c4c::ValueNameId{82},
                             16));
  const auto result = print_common_instruction_nodes({signed_record, unsigned_record});
  if (!result.ok) {
    return fail("expected i128 helper boundary records to print");
  }
  const std::string expected =
      "    mov x0, x8\n"
      "    mov x1, x9\n"
      "    mov x2, x10\n"
      "    mov x3, x11\n"
      "    bl __divti3\n"
      "    mov x6, x0\n"
      "    mov x7, x1\n"
      "    mov x0, x14\n"
      "    mov x1, x15\n"
      "    mov x2, x16\n"
      "    mov x3, x17\n"
      "    bl __umodti3\n"
      "    mov x12, x0\n"
      "    mov x13, x1\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "i128 helper boundary structured printer");
}

int selected_f128_transport_and_helper_nodes_print_from_structured_fields() {
  auto load_record = aarch64_codegen::F128TransportRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .transport_kind = aarch64_codegen::F128TransportKind::LoadFromMemory,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 1,
      .value_id = prepare::PreparedValueId{88},
      .value_name = c4c::ValueNameId{88},
      .value_type = bir::TypeKind::F128,
      .carrier_kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .register_bank = prepare::PreparedRegisterBank::Vreg,
      .register_class = prepare::PreparedRegisterClass::Vector,
      .contiguous_width = 1,
      .reg = qreg(4),
      .occupied_register_names = {"q4"},
      .memory = f128_frame_slot(32),
      .source_carrier = reinterpret_cast<const prepare::PreparedF128Carrier*>(0x1),
  };
  auto store_record = load_record;
  store_record.transport_kind = aarch64_codegen::F128TransportKind::StoreToMemory;
  store_record.instruction_index = 2;
  store_record.value_id = prepare::PreparedValueId{89};
  store_record.value_name = c4c::ValueNameId{89};
  store_record.reg = qreg(5);
  store_record.occupied_register_names = {"q5"};
  store_record.memory = f128_frame_slot(48);

  const auto load = aarch64_codegen::make_f128_transport_instruction(load_record);
  const auto store = aarch64_codegen::make_f128_transport_instruction(store_record);
  const auto helper = aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
      printable_f128_arithmetic_helper());
  const auto result = print_common_instruction_nodes({load, store, helper});
  if (!result.ok) {
    return fail("expected selected f128 nodes to print from structured fields: " +
                result.diagnostic);
  }
  const std::string expected =
      "    ldr q4, [sp, #32]\n"
      "    str q5, [sp, #48]\n"
      "    mov v0.16b, v6.16b\n"
      "    mov v1.16b, v8.16b\n"
      "    bl __addtf3\n"
      "    mov v4.16b, v0.16b\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "f128 transport/helper structured printer");
}

int selected_f128_memory_backed_transport_prints_through_reserved_scratch() {
  auto load_record = aarch64_codegen::F128TransportRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .transport_kind = aarch64_codegen::F128TransportKind::LoadFromMemory,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 3,
      .value_id = prepare::PreparedValueId{93},
      .value_name = c4c::ValueNameId{93},
      .value_type = bir::TypeKind::F128,
      .carrier_kind = prepare::PreparedF128CarrierKind::MemoryBacked,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .slot_id = prepare::PreparedFrameSlotId{7},
      .stack_offset_bytes = std::size_t{64},
      .memory = f128_frame_slot(32),
      .source_carrier = reinterpret_cast<const prepare::PreparedF128Carrier*>(0x1),
  };
  auto store_record = load_record;
  store_record.transport_kind = aarch64_codegen::F128TransportKind::StoreToMemory;
  store_record.instruction_index = 4;
  store_record.value_id = prepare::PreparedValueId{94};
  store_record.value_name = c4c::ValueNameId{94};
  store_record.memory = f128_frame_slot(96);

  const auto load = aarch64_codegen::make_f128_transport_instruction(load_record);
  const auto store = aarch64_codegen::make_f128_transport_instruction(store_record);
  const auto result = print_common_instruction_nodes({load, store});
  if (!result.ok) {
    return fail("expected selected f128 memory-backed transport to print through scratch: " +
                result.diagnostic);
  }
  const std::string expected =
      "    ldr q16, [sp, #32]\n"
      "    str q16, [sp, #64]\n"
      "    ldr q16, [sp, #64]\n"
      "    str q16, [sp, #96]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "f128 memory-backed transport scratch printer");
}

int selected_large_f128_memory_backed_transport_materializes_frame_slot_addresses() {
  auto load_record = aarch64_codegen::F128TransportRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .transport_kind = aarch64_codegen::F128TransportKind::LoadFromMemory,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 5,
      .value_id = prepare::PreparedValueId{95},
      .value_name = c4c::ValueNameId{95},
      .value_type = bir::TypeKind::F128,
      .carrier_kind = prepare::PreparedF128CarrierKind::MemoryBacked,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .slot_id = prepare::PreparedFrameSlotId{8},
      .stack_offset_bytes = std::size_t{80000},
      .memory = f128_frame_slot(70000),
      .source_carrier = reinterpret_cast<const prepare::PreparedF128Carrier*>(0x1),
  };
  auto store_record = load_record;
  store_record.transport_kind = aarch64_codegen::F128TransportKind::StoreToMemory;
  store_record.instruction_index = 6;
  store_record.value_id = prepare::PreparedValueId{96};
  store_record.value_name = c4c::ValueNameId{96};
  store_record.memory = f128_frame_slot(70016);

  const auto load = aarch64_codegen::make_f128_transport_instruction(load_record);
  const auto store = aarch64_codegen::make_f128_transport_instruction(store_record);
  const auto result = print_common_instruction_nodes({load, store});
  if (!result.ok) {
    return fail("expected large f128 memory-backed transport to materialize addresses: " +
                result.diagnostic);
  }
  const std::string expected =
      "    movz x9, #4464\n"
      "    movk x9, #1, lsl #16\n"
      "    add x9, sp, x9\n"
      "    ldr q16, [x9]\n"
      "    movz x9, #14464\n"
      "    movk x9, #1, lsl #16\n"
      "    add x9, sp, x9\n"
      "    str q16, [x9]\n"
      "    movz x9, #14464\n"
      "    movk x9, #1, lsl #16\n"
      "    add x9, sp, x9\n"
      "    ldr q16, [x9]\n"
      "    movz x9, #4480\n"
      "    movk x9, #1, lsl #16\n"
      "    add x9, sp, x9\n"
      "    str q16, [x9]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "large f128 memory-backed transport address materialization");
}

int selected_f128_symbol_memory_backed_transport_materializes_symbol_address() {
  auto symbol_memory = f128_frame_slot(0);
  symbol_memory.base_kind = aarch64_codegen::MemoryBaseKind::Symbol;
  symbol_memory.frame_slot_id = std::nullopt;
  symbol_memory.symbol_name = c4c::LinkNameId{39};
  symbol_memory.symbol_label = "hfa34";
  symbol_memory.byte_offset = 16;

  auto load_record = aarch64_codegen::F128TransportRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .transport_kind = aarch64_codegen::F128TransportKind::LoadFromMemory,
      .function_name = c4c::FunctionNameId{2},
      .block_label = c4c::BlockLabelId{3},
      .instruction_index = 7,
      .value_id = prepare::PreparedValueId{97},
      .value_name = c4c::ValueNameId{97},
      .value_type = bir::TypeKind::F128,
      .carrier_kind = prepare::PreparedF128CarrierKind::MemoryBacked,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .slot_id = prepare::PreparedFrameSlotId{9},
      .stack_offset_bytes = std::size_t{64},
      .memory = symbol_memory,
      .source_carrier = reinterpret_cast<const prepare::PreparedF128Carrier*>(0x1),
  };

  const auto load = aarch64_codegen::make_f128_transport_instruction(load_record);
  const auto result = print_common_instruction_nodes({load});
  if (!result.ok) {
    return fail("expected f128 symbol memory-backed transport to materialize address: " +
                result.diagnostic);
  }
  const std::string expected =
      "    adrp x9, hfa34+16\n"
      "    add x9, x9, :lo12:hfa34+16\n"
      "    ldr q16, [x9]\n"
      "    str q16, [sp, #64]\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "f128 symbol memory-backed transport address materialization");
}

int selected_f128_compare_and_cast_helpers_print_from_structured_records() {
  const auto compare = aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
      printable_f128_comparison_helper());
  const auto f64_to_f128 = aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
      printable_f64_to_f128_cast_helper());
  const auto f128_to_f32 = aarch64_codegen::make_f128_runtime_helper_boundary_instruction(
      printable_f128_to_f32_cast_helper());
  const auto result = print_common_instruction_nodes({compare, f64_to_f128, f128_to_f32});
  if (!result.ok) {
    return fail("expected f128 compare/cast helpers to print from structured records: " +
                result.diagnostic);
  }
  const std::string expected =
      "    mov v0.16b, v6.16b\n"
      "    mov v1.16b, v8.16b\n"
      "    bl __eqtf2\n"
      "    cmp w0, #0\n"
      "    cset w9, eq\n"
      "    fmov d0, d6\n"
      "    bl __extenddftf2\n"
      "    mov v4.16b, v0.16b\n"
      "    mov v0.16b, v6.16b\n"
      "    bl __trunctfsf2\n"
      "    fmov s4, s0\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "f128 compare/cast helper structured printer");
}

int selected_f128_records_reject_incomplete_structured_fields() {
  const auto memory_backed_without_slot =
      aarch64_codegen::make_f128_transport_instruction(
      aarch64_codegen::F128TransportRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .transport_kind = aarch64_codegen::F128TransportKind::LoadFromMemory,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 3,
          .value_id = prepare::PreparedValueId{93},
          .value_name = c4c::ValueNameId{93},
          .value_type = bir::TypeKind::F128,
          .carrier_kind = prepare::PreparedF128CarrierKind::MemoryBacked,
          .total_size_bytes = 16,
          .total_align_bytes = 16,
          .memory = f128_frame_slot(64),
          .source_carrier = reinterpret_cast<const prepare::PreparedF128Carrier*>(0x1),
      });
  const auto memory_backed_result =
      aarch64_codegen::print_machine_instruction_line_payloads(memory_backed_without_slot);
  if (memory_backed_result.ok ||
      memory_backed_result.diagnostic.find("frame-slot authority") ==
          std::string::npos) {
    return fail("expected incomplete f128 memory-backed transport to fail closed");
  }

  auto helper_record = printable_f128_arithmetic_helper();
  helper_record.lhs.carrier_register.reset();
  const auto helper =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(helper_record);
  const auto helper_result =
      aarch64_codegen::print_machine_instruction_line_payloads(helper);
  if (helper_result.ok ||
      helper_result.diagnostic.find("missing full-width q-register carrier or ABI facts") ==
          std::string::npos) {
    return fail("expected incomplete f128 helper boundary to fail closed before printing");
  }

  auto compare_record = printable_f128_comparison_helper();
  compare_record.scalar_result.marshaling_move.reset();
  const auto compare =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(compare_record);
  const auto compare_result =
      aarch64_codegen::print_machine_instruction_line_payloads(compare);
  if (compare_result.ok ||
      compare_result.diagnostic.find("scalar cmp-result marshal facts") ==
          std::string::npos) {
    return fail("expected incomplete f128 comparison helper boundary to fail closed");
  }

  auto f64_to_f128_record = printable_f64_to_f128_cast_helper();
  f64_to_f128_record.scalar_operand.marshaling_move.reset();
  const auto f64_to_f128 =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(f64_to_f128_record);
  const auto f64_to_f128_result =
      aarch64_codegen::print_machine_instruction_line_payloads(f64_to_f128);
  if (f64_to_f128_result.ok ||
      f64_to_f128_result.diagnostic.find("scalar marshal/unmarshal moves") ==
          std::string::npos) {
    return fail("expected incomplete f64->f128 cast helper boundary to fail closed");
  }

  auto f128_to_f32_record = printable_f128_to_f32_cast_helper();
  f128_to_f32_record.scalar_result.marshaling_move.reset();
  const auto f128_to_f32 =
      aarch64_codegen::make_f128_runtime_helper_boundary_instruction(f128_to_f32_record);
  const auto f128_to_f32_result =
      aarch64_codegen::print_machine_instruction_line_payloads(f128_to_f32);
  if (f128_to_f32_result.ok ||
      f128_to_f32_result.diagnostic.find("scalar marshal/unmarshal moves") ==
          std::string::npos) {
    return fail("expected incomplete f128->f32 cast helper boundary to fail closed");
  }

  return 0;
}

int selected_i128_records_reject_incomplete_structured_fields() {
  const auto helper_record = incomplete_i128_helper_record();
  const auto helper =
      aarch64_codegen::make_i128_runtime_helper_boundary_instruction(helper_record);
  const auto helper_result = aarch64_codegen::print_machine_instruction_line_payloads(helper);
  if (helper_result.ok ||
      helper_result.diagnostic.find("live-preservation policy") == std::string::npos) {
    return fail("expected i128 helper boundary without live preservation to fail closed");
  }
  auto missing_move_source =
      printable_i128_helper(bir::BinaryOpcode::SDiv,
                            prepare::PreparedI128RuntimeHelperKind::SignedDiv,
                            "__divti3",
                            prepare::PreparedValueId{70},
                            c4c::ValueNameId{70},
                            6,
                            prepare::PreparedValueId{71},
                            c4c::ValueNameId{71},
                            8,
                            prepare::PreparedValueId{72},
                            c4c::ValueNameId{72},
                            10);
  missing_move_source.lhs_low_argument_move.reset();
  const auto missing_move_helper =
      aarch64_codegen::make_i128_runtime_helper_boundary_instruction(
          i128_helper_record(&missing_move_source,
                             bir::BinaryOpcode::SDiv,
                             prepare::PreparedI128RuntimeHelperKind::SignedDiv,
                             "__divti3",
                             prepare::PreparedValueId{70},
                             c4c::ValueNameId{70},
                             6,
                             prepare::PreparedValueId{71},
                             c4c::ValueNameId{71},
                             8,
                             prepare::PreparedValueId{72},
                             c4c::ValueNameId{72},
                             10));
  const auto missing_move_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_move_helper);
  if (missing_move_result.ok ||
      missing_move_result.diagnostic.find("marshal/unmarshal moves") ==
          std::string::npos) {
    return fail("expected i128 helper boundary without marshal move facts to fail closed");
  }

  auto missing_abi_source = printable_i128_helper(
      bir::BinaryOpcode::URem,
      prepare::PreparedI128RuntimeHelperKind::UnsignedRem,
      "__umodti3",
      prepare::PreparedValueId{80},
      c4c::ValueNameId{80},
      12,
      prepare::PreparedValueId{81},
      c4c::ValueNameId{81},
      14,
      prepare::PreparedValueId{82},
      c4c::ValueNameId{82},
      16);
  missing_abi_source.rhs_high_argument_move->abi_register.register_bank =
      prepare::PreparedRegisterBank::None;
  const auto missing_abi_helper =
      aarch64_codegen::make_i128_runtime_helper_boundary_instruction(
          i128_helper_record(&missing_abi_source,
                             bir::BinaryOpcode::URem,
                             prepare::PreparedI128RuntimeHelperKind::UnsignedRem,
                             "__umodti3",
                             prepare::PreparedValueId{80},
                             c4c::ValueNameId{80},
                             12,
                             prepare::PreparedValueId{81},
                             c4c::ValueNameId{81},
                             14,
                             prepare::PreparedValueId{82},
                             c4c::ValueNameId{82},
                             16));
  const auto missing_abi_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_abi_helper);
  if (missing_abi_result.ok ||
      missing_abi_result.diagnostic.find("GPR ABI register bindings") ==
          std::string::npos) {
    return fail("expected i128 helper boundary without ABI move facts to fail closed");
  }

  auto pair_record = aarch64_codegen::I128PairOperationRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .operation = aarch64_codegen::I128PairOperationKind::Sub,
      .lane_semantics = aarch64_codegen::I128PairLaneSemantics::BorrowPropagating,
      .source_binary_opcode = bir::BinaryOpcode::Sub,
      .result = i128_pair_operand(prepare::PreparedValueId{80}, c4c::ValueNameId{80}, 2),
      .lhs = i128_pair_operand(prepare::PreparedValueId{81}, c4c::ValueNameId{81}, 4),
      .rhs = i128_pair_operand(prepare::PreparedValueId{82}, c4c::ValueNameId{82}, 6),
  };
  pair_record.rhs.high_lane.reg.reset();
  const auto pair = aarch64_codegen::make_i128_pair_operation_instruction(pair_record);
  const auto pair_result = aarch64_codegen::print_machine_instruction_line_payloads(pair);
  if (pair_result.ok ||
      pair_result.diagnostic.find("missing_required_facts") == std::string::npos ||
      pair_result.diagnostic.find("source register-pair carriers") == std::string::npos) {
    return fail("expected incomplete i128 pair operation to fail closed");
  }
  auto copy_record = aarch64_codegen::I128TransportRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .transport_kind = aarch64_codegen::I128TransportKind::CopyRegisterPair,
      .value_id = prepare::PreparedValueId{83},
      .value_name = c4c::ValueNameId{83},
      .carrier_kind = prepare::PreparedI128CarrierKind::RegisterPair,
      .low_lane =
          i128_pair_operand(prepare::PreparedValueId{83}, c4c::ValueNameId{83}, 8)
              .low_lane,
      .high_lane =
          i128_pair_operand(prepare::PreparedValueId{83}, c4c::ValueNameId{83}, 8)
              .high_lane,
      .source_value_id = prepare::PreparedValueId{84},
      .source_value_name = c4c::ValueNameId{84},
      .source_low_lane =
          i128_pair_operand(prepare::PreparedValueId{84}, c4c::ValueNameId{84}, 10)
              .low_lane,
      .source_high_lane =
          i128_pair_operand(prepare::PreparedValueId{84}, c4c::ValueNameId{84}, 10)
              .high_lane,
      .source_carrier = reinterpret_cast<const prepare::PreparedI128Carrier*>(0x1),
      .copy_source_carrier = reinterpret_cast<const prepare::PreparedI128Carrier*>(0x2),
  };
  copy_record.source_high_lane.reg.reset();
  const auto copy = aarch64_codegen::make_i128_transport_instruction(copy_record);
  const auto copy_result = aarch64_codegen::print_machine_instruction_line_payloads(copy);
  if (copy_result.ok ||
      copy_result.diagnostic.find("source low/high registers") == std::string::npos) {
    return fail("expected incomplete i128 copy transport to fail closed");
  }
  return 0;
}

int selected_immediate_return_node_prints_callable_epilogue() {
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 0,
              }),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = print_common_instruction_nodes({ret});
  if (!result.ok) {
    return fail("expected selected immediate return node to print AArch64 return text: " +
                result.diagnostic);
  }
  const auto move_mnemonic = aarch64_codegen::machine_instruction_auxiliary_printer_mnemonic(ret);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const std::string expected =
      "    " + std::string(move_mnemonic) + " w0, #0\n" +
      "    " + std::string(return_mnemonic) + "\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    mov w0, #0\n    ret\n",
                                        "return common-printer drift guard");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          move_mnemonic, "mov", "return helper auxiliary mnemonic");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(
          return_mnemonic, "ret", "return helper primary mnemonic");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_symbol_pointer_return_node_prints_address_materialization_before_ret() {
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_symbol_operand(
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{72},
                  .type = bir::TypeKind::Ptr,
              }),
          .value_type = bir::TypeKind::Ptr,
          .symbol_label = "selected_callee",
      });

  const auto result = print_common_instruction_nodes({ret});
  if (!result.ok) {
    return fail("expected selected symbol-pointer return node to print AArch64 return text: " +
                result.diagnostic);
  }
  const std::string expected =
      "    adrp x0, selected_callee\n"
      "    add x0, x0, :lo12:selected_callee\n"
      "    ret\n";
  return expect_assembly(result.assembly,
                         expected,
                         expected,
                         "symbol-pointer return common-printer drift guard");
}

int common_mir_printer_can_delegate_to_aarch64_target_spelling_adapter() {
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_immediate_operand(
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 7,
              }),
          .value_type = bir::TypeKind::I32,
      });

  mir::MachineFunction<aarch64_codegen::InstructionRecord> function;
  function.function_name = c4c::FunctionNameId{11};
  function.blocks.push_back(mir::MachineBlock<aarch64_codegen::InstructionRecord>{
      .block_label = c4c::BlockLabelId{13},
      .index = 0,
      .instructions =
          {
              mir::MachineInstruction<aarch64_codegen::InstructionRecord>{
                  .target = ret,
              },
          },
  });

  const auto target_printer = aarch64_codegen::MachineInstructionPrinter{};
  const auto result = mir::print_machine_function(function, target_printer);
  if (!result.ok) {
    return fail("expected common MIR printer to delegate AArch64 target spelling: " +
                result.diagnostic);
  }
  const auto move_mnemonic = aarch64_codegen::machine_instruction_auxiliary_printer_mnemonic(ret);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const auto line_payloads = aarch64_codegen::print_machine_instruction_line_payloads(ret);
  if (!line_payloads.ok || line_payloads.instruction_lines.size() != 2 ||
      line_payloads.instruction_lines[0] != "mov w0, #7" ||
      line_payloads.instruction_lines[1] != "ret") {
    return fail("expected AArch64 target spelling hook to return unindented line payloads");
  }
  const std::string expected =
      "    " + std::string(move_mnemonic) + " w0, #7\n" +
      "    " + std::string(return_mnemonic) + "\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov w0, #7\n    ret\n",
                         "common MIR printer AArch64 adapter drift guard");
}

int selected_scalar_add_with_immediate_operands_prints_structured_add() {
  const auto add = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{44},
          .result_value_name = c4c::ValueNameId{45},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 2,
          }),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 3,
          }),
          .supported_integer_operation = true,
      }));
  const auto ret = aarch64_codegen::make_return_instruction(
      aarch64_codegen::ReturnInstructionRecord{
          .value = aarch64_codegen::make_register_operand(wreg(0)),
          .value_type = bir::TypeKind::I32,
      });

  const auto result = print_common_instruction_nodes({add, ret});
  if (!result.ok) {
    return fail("expected scalar add with immediate operands to print structured add: " +
                result.diagnostic);
  }
  const auto add_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(add);
  const auto return_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(ret);
  const std::string expected =
      "    mov w0, #2\n"
      "    " +
      std::string(add_mnemonic) + " w0, w0, #3\n" +
      "    " + std::string(return_mnemonic) + "\n";
  if (const int check = expect_assembly(result.assembly,
                                        expected,
                                        "    mov w0, #2\n"
                                        "    add w0, w0, #3\n"
                                        "    ret\n",
                                        "scalar immediate add common-printer drift guard");
      check != 0) {
    return check;
  }
  return 0;
}

int selected_scalar_add_sub_materializes_nonencodable_immediates() {
  const auto add_out_of_range = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{46},
          .result_value_name = c4c::ValueNameId{47},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 4096,
          }),
          .supported_integer_operation = true,
      }));
  const auto out_of_range_result =
      aarch64_codegen::print_machine_instruction_line_payloads(add_out_of_range);
  if (!out_of_range_result.ok ||
      out_of_range_result.instruction_lines.size() != 2 ||
      out_of_range_result.instruction_lines[0] != "mov w9, #4096" ||
      out_of_range_result.instruction_lines[1] != "add w0, w1, w9") {
    return fail("expected scalar add with out-of-range immediate to materialize rhs: " +
                join_lines(out_of_range_result.instruction_lines));
  }

  const auto add_multi_chunk = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{148},
          .result_value_name = c4c::ValueNameId{149},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 503808,
          }),
          .supported_integer_operation = true,
      }));
  const auto multi_chunk_result =
      aarch64_codegen::print_machine_instruction_line_payloads(add_multi_chunk);
  if (!multi_chunk_result.ok ||
      multi_chunk_result.instruction_lines.size() != 3 ||
      multi_chunk_result.instruction_lines[0] != "movz w9, #45056" ||
      multi_chunk_result.instruction_lines[1] != "movk w9, #7, lsl #16" ||
      multi_chunk_result.instruction_lines[2] != "add w0, w1, w9") {
    return fail("expected scalar add with multi-chunk immediate to materialize rhs: " +
                join_lines(multi_chunk_result.instruction_lines));
  }

  const auto sub_negative = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{48},
          .result_value_name = c4c::ValueNameId{49},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_register_operand(wreg(1)),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = -1,
          }),
          .supported_integer_operation = true,
      }));
  const auto negative_result =
      aarch64_codegen::print_machine_instruction_line_payloads(sub_negative);
  if (!negative_result.ok ||
      negative_result.instruction_lines.size() != 2 ||
      negative_result.instruction_lines[0] != "mov w9, #-1" ||
      negative_result.instruction_lines[1] != "sub w0, w1, w9") {
    return fail("expected scalar sub with negative immediate to materialize rhs: " +
                join_lines(negative_result.instruction_lines));
  }

  return 0;
}

int selected_scalar_mul_div_rem_materializes_immediate_operands() {
  const auto div_pair = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Div,
          .source_binary_opcode = bir::BinaryOpcode::SDiv,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{50},
          .result_value_name = c4c::ValueNameId{51},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(13),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 6,
          }),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 2,
          }),
          .supported_integer_operation = true,
      }));
  const auto div_result = aarch64_codegen::print_machine_instruction_line_payloads(div_pair);
  if (!div_result.ok ||
      div_result.instruction_lines.size() != 3 ||
      div_result.instruction_lines[0] != "mov w9, #6" ||
      div_result.instruction_lines[1] != "mov w13, #2" ||
      div_result.instruction_lines[2] != "sdiv w13, w9, w13") {
    return fail("expected scalar div with immediate operands to materialize both sides: " +
                join_lines(div_result.instruction_lines));
  }

  const auto mul_immediate_lhs = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Mul,
          .source_binary_opcode = bir::BinaryOpcode::Mul,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{52},
          .result_value_name = c4c::ValueNameId{53},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(20),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 2,
          }),
          .rhs = aarch64_codegen::make_register_operand(wreg(13)),
          .supported_integer_operation = true,
      }));
  const auto mul_result =
      aarch64_codegen::print_machine_instruction_line_payloads(mul_immediate_lhs);
  if (!mul_result.ok ||
      mul_result.instruction_lines.size() != 2 ||
      mul_result.instruction_lines[0] != "mov w9, #2" ||
      mul_result.instruction_lines[1] != "mul w20, w9, w13") {
    return fail("expected scalar mul with immediate lhs to materialize lhs: " +
                join_lines(mul_result.instruction_lines));
  }

  const auto rem_pair = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Div,
          .source_binary_opcode = bir::BinaryOpcode::SRem,
          .operand_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{54},
          .result_value_name = c4c::ValueNameId{55},
          .result_type = bir::TypeKind::I32,
          .result_register = wreg(0),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 7,
          }),
          .rhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 3,
          }),
          .supported_integer_operation = true,
      }));
  const auto rem_result = aarch64_codegen::print_machine_instruction_line_payloads(rem_pair);
  if (!rem_result.ok ||
      rem_result.instruction_lines.size() != 4 ||
      rem_result.instruction_lines[0] != "mov w9, #7" ||
      rem_result.instruction_lines[1] != "mov w10, #3" ||
      rem_result.instruction_lines[2] != "sdiv w0, w9, w10" ||
      rem_result.instruction_lines[3] != "msub w0, w0, w10, w9") {
    return fail("expected scalar rem with immediate operands to materialize both sides: " +
                join_lines(rem_result.instruction_lines));
  }

  return 0;
}

int selected_simple_integer_casts_reject_missing_or_unsupported_facts() {
  const auto materialized_source = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignExtend,
          .source_cast_opcode = bir::CastOpcode::SExt,
          .source_type = bir::TypeKind::I32,
          .result_value_id = prepare::PreparedValueId{58},
          .result_value_name = c4c::ValueNameId{59},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .source = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I32,
              .signed_value = 1,
          }),
          .supported_simple_integer_cast = true,
      }));
  const auto materialized_source_result =
      aarch64_codegen::print_machine_instruction_line_payloads(materialized_source);
  if (!materialized_source_result.ok ||
      materialized_source_result.instruction_lines.size() != 2 ||
      materialized_source_result.instruction_lines[0] != "mov w0, #1" ||
      materialized_source_result.instruction_lines[1] != "sxtw x0, w0") {
    return fail("expected simple integer cast immediate source to materialize: " +
                join_lines(materialized_source_result.instruction_lines));
  }

  const auto unsupported_type = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_cast_instruction_record(aarch64_codegen::ScalarCastRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarCastOperationKind::SignExtend,
          .source_cast_opcode = bir::CastOpcode::SExt,
          .source_type = bir::TypeKind::F128,
          .result_value_id = prepare::PreparedValueId{60},
          .result_value_name = c4c::ValueNameId{61},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .source = aarch64_codegen::make_register_operand(xreg(1)),
          .supported_simple_integer_cast = true,
      }));
  const auto unsupported_type_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unsupported_type);
  if (unsupported_type_result.ok ||
      unsupported_type_result.diagnostic.find("supported integer source/result width") ==
          std::string::npos) {
    return fail("expected simple integer cast with unsupported type to fail closed");
  }

  return 0;
}

int selected_direct_call_prints_from_prepared_call_provenance() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"actual_function"},
      .preserved_values = {prepare::PreparedCallPreservedValue{
          .value_id = prepare::PreparedValueId{77},
          .value_name = c4c::ValueNameId{14},
          .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
          .callee_saved_save_index = std::size_t{0},
          .contiguous_width = 1,
          .register_name = std::string{"x19"},
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .occupied_register_names = {"x19"},
      }},
      .clobbered_registers = {prepare::PreparedClobberedRegister{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_name = "x13",
          .contiguous_width = 1,
          .occupied_register_names = {"x13"},
      }},
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{9},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "actual_function",
          .wrapper_kind = prepared_call.wrapper_kind,
          .preserved_values = prepared_call.preserved_values,
          .clobbered_registers = prepared_call.clobbered_registers,
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });

  if (call.preserves.size() != 1 ||
      call.preserves.front().reg != aarch64_abi::x_register(19) ||
      call.preserves.front().value_id != prepare::PreparedValueId{77}) {
    return fail("expected direct-call printer fixture to carry prepared preserved-value effect");
  }
  if (call.clobbers.size() != 1 ||
      call.clobbers.front().reg != aarch64_abi::x_register(13)) {
    return fail("expected direct-call printer fixture to carry prepared clobber effect");
  }
  const auto result = print_common_instruction_nodes({call});
  if (!result.ok) {
    return fail("expected direct call node to print from prepared provenance: " +
                result.diagnostic);
  }
  const auto call_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(call);
  const std::string expected = "    " + std::string(call_mnemonic) + " actual_function\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    bl actual_function\n",
                         "direct-call common-printer drift guard");
}

int selected_memory_return_call_prints_call_and_preserves_storage_effect() {
  const prepare::PreparedMemoryReturnPlan memory_return{
      .sret_arg_index = std::size_t{0},
      .storage_slot_name = c4c::SlotNameId{8},
      .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
      .slot_id = prepare::PreparedFrameSlotId{9},
      .stack_offset_bytes = std::size_t{32},
      .size_bytes = 16,
      .align_bytes = 8,
  };
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"make_large"},
      .memory_return = memory_return,
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{9},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "make_large",
          .wrapper_kind = prepared_call.wrapper_kind,
          .memory_return = prepared_call.memory_return,
          .memory_return_storage =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{3},
                  .instruction_index = 1,
                  .base_kind = aarch64_codegen::MemoryBaseKind::FrameSlot,
                  .frame_slot_id = prepare::PreparedFrameSlotId{9},
                  .byte_offset = 32,
                  .byte_offset_is_prepared_snapshot = true,
                  .size_bytes = 16,
                  .align_bytes = 8,
                  .can_use_base_plus_offset = true,
              },
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
      });

  if (call.defs.size() != 1 ||
      call.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Memory ||
      call.defs.front().frame_slot_id != prepare::PreparedFrameSlotId{9} ||
      call.side_effects.size() != 2 ||
      call.side_effects.back() != aarch64_codegen::MachineSideEffectKind::MemoryWrite) {
    return fail("expected memory-return call printer fixture to carry prepared storage effect");
  }
  const auto result = print_common_instruction_nodes({call});
  if (!result.ok) {
    return fail("expected memory-return call node to print the call itself: " +
                result.diagnostic);
  }
  const auto call_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(call);
  const std::string expected = "    " + std::string(call_mnemonic) + " make_large\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    bl make_large\n",
                         "memory-return call common-printer drift guard");
}

int selected_indirect_call_prints_from_prepared_register_callee() {
  const prepare::PreparedCallPlan prepared_call{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
      .is_indirect = true,
      .indirect_callee =
          prepare::PreparedIndirectCalleePlan{
              .value_name = c4c::ValueNameId{52},
              .value_id = prepare::PreparedValueId{51},
              .encoding = prepare::PreparedStorageEncodingKind::Register,
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = std::string{"x9"},
          },
  };
  const auto call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .indirect_callee =
              aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(9),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
                  .value_id = prepare::PreparedValueId{51},
                  .value_name = c4c::ValueNameId{52},
                  .prepared_class = prepare::PreparedRegisterClass::General,
                  .prepared_bank = prepare::PreparedRegisterBank::Gpr,
                  .expected_view = aarch64_abi::RegisterView::X,
                  .contiguous_width = 1,
                  .occupied_registers = {"x9"},
              }),
          .wrapper_kind = prepared_call.wrapper_kind,
          .prepared_indirect_callee = prepared_call.indirect_callee,
          .source_call = &prepared_call,
          .calling_convention = bir::CallingConv::C,
          .is_indirect = true,
      });

  const auto result = print_common_instruction_nodes({call});
  if (!result.ok) {
    return fail("expected indirect call node to print from prepared register callee: " +
                result.diagnostic);
  }
  const auto call_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(call);
  const std::string expected = "    " + std::string(call_mnemonic) + " x9\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    blr x9\n",
                         "indirect-call common-printer drift guard");
}

int selected_call_boundary_register_move_prints_prepared_mov() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{70},
                  .to_value_id = prepare::PreparedValueId{70},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_register = xreg(2),
          .destination_register = xreg(0),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected call-boundary register move to print: " + result.diagnostic);
  }
  const auto move_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(move);
  const std::string expected = "    " + std::string(move_mnemonic) + " x0, x2\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov x0, x2\n",
                         "call-boundary register-move drift guard");
}

int selected_call_boundary_scalar_fpr_argument_move_prints_prepared_fmov() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{70},
                  .to_value_id = prepare::PreparedValueId{70},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"s0"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"s0"},
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_register = sreg(13),
          .destination_register = sreg(0),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected scalar FPR call-boundary move to print: " + result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    fmov s0, s13\n",
                         "    fmov s0, s13\n",
                         "fixed HFA scalar FPR call-lane publication");
}

int selected_call_boundary_immediate_argument_prints_prepared_mov() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
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
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = prepare::PreparedMoveResolution{
              .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .destination_register_name = std::string{"x0"},
              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          },
          .source_immediate =
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 7,
                  .unsigned_value = 7,
              },
          .destination_register = wreg(0),
          .source_bundle = &call_boundary_bundle,
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected call-boundary immediate argument move to print: " +
                result.diagnostic);
  }
  const auto move_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(move);
  const std::string expected = "    " + std::string(move_mnemonic) + " w0, #7\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov w0, #7\n",
                         "call-boundary immediate argument drift guard");
}

int selected_call_boundary_large_immediate_argument_materializes_legal_constant() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
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
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = prepare::PreparedMoveResolution{
              .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .destination_register_name = std::string{"x0"},
              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          },
          .source_immediate =
              aarch64_codegen::ImmediateOperand{
                  .kind = aarch64_codegen::ImmediateKind::SignedInteger,
                  .type = bir::TypeKind::I32,
                  .signed_value = 0x12345,
                  .unsigned_value = 0x12345,
              },
          .destination_register = wreg(0),
          .source_bundle = &call_boundary_bundle,
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected large call-boundary immediate argument to print: " +
                result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    movz w0, #9029\n"
                         "    movk w0, #1, lsl #16\n",
                         "    movz w0, #9029\n"
                         "    movk w0, #1, lsl #16\n",
                         "large call-boundary immediate materialization");
}

int selected_call_boundary_frame_slot_source_materializes_large_offset() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{72},
                  .to_value_id = prepare::PreparedValueId{72},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
  };
  const auto in_range = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_memory = frame_slot(32),
          .destination_register = xreg(0),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });
  auto large_source = frame_slot(1644);
  large_source.size_bytes = 4;
  large_source.align_bytes = 4;
  const auto large = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_memory = large_source,
          .destination_register = xreg(13),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({in_range, large});
  if (!result.ok) {
    return fail("expected call-boundary frame-slot source moves to print: " +
                result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    ldr x0, [sp, #32]\n"
                         "    add x9, sp, #1644\n"
                         "    ldr x13, [x9]\n",
                         "    ldr x0, [sp, #32]\n"
                         "    add x9, sp, #1644\n"
                         "    ldr x13, [x9]\n",
                         "call-boundary frame-slot source materialization");
}

int selected_call_boundary_f128_frame_slot_argument_prints_q_load() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{72},
                  .to_value_id = prepare::PreparedValueId{72},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{3},
                  .destination_register_name = std::string{"q3"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_memory = f128_frame_slot(80),
          .destination_register = qreg(3),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected f128 frame-slot call argument to print q-register load: " +
                result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    ldr q3, [sp, #80]\n",
                         "    ldr q3, [sp, #80]\n",
                         "f128 frame-slot call argument publication");
}

int selected_after_call_result_register_move_prints_prepared_mov() {
  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::AfterCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{71},
                  .to_value_id = prepare::PreparedValueId{71},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"x0"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"x0"},
              },
          },
  };
  const auto move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_register = xreg(0),
          .destination_register = xreg(3),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });

  const auto result = print_common_instruction_nodes({move});
  if (!result.ok) {
    return fail("expected after-call result register move to print: " + result.diagnostic);
  }
  const auto move_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(move);
  const std::string expected = "    " + std::string(move_mnemonic) + " x3, x0\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    mov x3, x0\n",
                         "after-call result register-move drift guard");
}

int selected_simple_frame_setup_and_teardown_print_from_prepared_frame_facts() {
  const prepare::PreparedFramePlanFunction prepared_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
  };
  const auto setup = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });
  const auto teardown = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });

  const auto result = print_common_instruction_nodes({setup, teardown});
  if (!result.ok) {
    return fail("expected simple fixed frame nodes to print from prepared frame facts: " +
                result.diagnostic);
  }
  const auto setup_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(setup);
  const auto teardown_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(teardown);
  const std::string expected =
      "    " + std::string(setup_mnemonic) + " sp, sp, #32\n" +
      "    " + std::string(teardown_mnemonic) + " sp, sp, #32\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    sub sp, sp, #32\n"
                         "    add sp, sp, #32\n",
                         "simple frame common-printer drift guard");
}

int selected_large_frame_setup_and_teardown_materialize_adjustment() {
  const prepare::PreparedFramePlanFunction prepared_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 5776,
      .frame_alignment_bytes = 16,
  };
  const auto setup = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });
  const auto teardown = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = prepared_frame.function_name,
          .frame_size_bytes = prepared_frame.frame_size_bytes,
          .frame_alignment_bytes = prepared_frame.frame_alignment_bytes,
          .source_frame = &prepared_frame,
      });

  const auto result = print_common_instruction_nodes({setup, teardown});
  if (!result.ok) {
    return fail("expected large fixed frame adjustment nodes to print: " +
                result.diagnostic);
  }
  const auto setup_mnemonic = aarch64_codegen::machine_instruction_primary_printer_mnemonic(setup);
  const auto teardown_mnemonic =
      aarch64_codegen::machine_instruction_primary_printer_mnemonic(teardown);
  const std::string expected =
      "    movz x9, #5776\n"
      "    " + std::string(setup_mnemonic) + " sp, sp, x9\n"
      "    movz x9, #5776\n"
      "    " + std::string(teardown_mnemonic) + " sp, sp, x9\n";
  return expect_assembly(result.assembly,
                         expected,
                         "    movz x9, #5776\n"
                         "    sub sp, sp, x9\n"
                         "    movz x9, #5776\n"
                         "    add sp, sp, x9\n",
                         "large frame adjustment materialization drift guard");
}

int selected_non_leaf_frame_prints_link_register_save_restore() {
  const auto saved_x19 = prepared_saved_x19(16);
  const auto setup = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = c4c::FunctionNameId{2},
          .frame_size_bytes = 32,
          .frame_alignment_bytes = 16,
          .preserves_link_register = true,
          .link_register_save_offset_bytes = std::size_t{0},
          .saved_callee_registers = {saved_x19},
      });
  const auto teardown = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = c4c::FunctionNameId{2},
          .frame_size_bytes = 32,
          .frame_alignment_bytes = 16,
          .preserves_link_register = true,
          .link_register_save_offset_bytes = std::size_t{0},
          .saved_callee_registers = {saved_x19},
      });

  const auto result = print_common_instruction_nodes({setup, teardown});
  if (!result.ok) {
    return fail("expected non-leaf LR frame nodes to print: " + result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    sub sp, sp, #32\n"
                         "    str x30, [sp, #0]\n"
                         "    str x19, [sp, #16]\n"
                         "    ldr x19, [sp, #16]\n"
                         "    ldr x30, [sp, #0]\n"
                         "    add sp, sp, #32\n",
                         "    sub sp, sp, #32\n"
                         "    str x30, [sp, #0]\n"
                         "    str x19, [sp, #16]\n"
                         "    ldr x19, [sp, #16]\n"
                         "    ldr x30, [sp, #0]\n"
                         "    add sp, sp, #32\n",
                         "non-leaf LR frame common-printer drift guard");
}

int selected_large_frame_link_register_slot_materializes_address() {
  const auto setup = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = c4c::FunctionNameId{2},
          .frame_size_bytes = 5776,
          .frame_alignment_bytes = 16,
          .preserves_link_register = true,
          .link_register_save_offset_bytes = std::size_t{5760},
      });
  const auto teardown = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::EpilogueTeardown,
          .function_name = c4c::FunctionNameId{2},
          .frame_size_bytes = 5776,
          .frame_alignment_bytes = 16,
          .preserves_link_register = true,
          .link_register_save_offset_bytes = std::size_t{5760},
      });

  const auto result = print_common_instruction_nodes({setup, teardown});
  if (!result.ok) {
    return fail("expected large link-register frame slot to print: " +
                result.diagnostic);
  }
  return expect_assembly(result.assembly,
                         "    movz x9, #5776\n"
                         "    sub sp, sp, x9\n"
                         "    movz x9, #5760\n"
                         "    add x9, sp, x9\n"
                         "    str x30, [x9]\n"
                         "    movz x9, #5760\n"
                         "    add x9, sp, x9\n"
                         "    ldr x30, [x9]\n"
                         "    movz x9, #5776\n"
                         "    add sp, sp, x9\n",
                         "    movz x9, #5776\n"
                         "    sub sp, sp, x9\n"
                         "    movz x9, #5760\n"
                         "    add x9, sp, x9\n"
                         "    str x30, [x9]\n"
                         "    movz x9, #5760\n"
                         "    add x9, sp, x9\n"
                         "    ldr x30, [x9]\n"
                         "    movz x9, #5776\n"
                         "    add sp, sp, x9\n",
                         "large link-register frame slot drift guard");
}

int selected_direct_address_materialization_prints_page_low12_sequence() {
  prepare::PreparedAddressMaterialization source;
  const auto address = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::DirectPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::DirectGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 4,
          .result_value_id = prepare::PreparedValueId{40},
          .result_value_name = c4c::ValueNameId{41},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(9),
          .symbol_name = c4c::LinkNameId{42},
          .symbol_label = "g.direct",
          .byte_offset = 16,
          .source_materialization = &source,
      });
  const auto result = aarch64_codegen::print_machine_instruction_line_payloads(address);
  if (!result.ok || !result.diagnostic.empty()) {
    return fail("expected direct global address materialization to print ADRP/ADD");
  }
  if (result.instruction_lines.size() != 2) {
    return fail("expected direct global address materialization to print two lines");
  }
  if (const int check = expect_equal(
          result.instruction_lines[0], "adrp x9, g.direct+16", "direct address ADRP");
      check != 0) {
    return check;
  }
  return expect_equal(
      result.instruction_lines[1], "add x9, x9, :lo12:g.direct+16", "direct address low12 ADD");
}

int selected_string_address_materialization_prints_page_low12_sequence() {
  prepare::PreparedAddressMaterialization source;
  const auto address = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::StringConstant,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::StringConstant,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 5,
          .result_value_id = prepare::PreparedValueId{42},
          .result_value_name = c4c::ValueNameId{43},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(10),
          .text_name = c4c::TextId{44},
          .text_label = ".L.str0",
          .source_materialization = &source,
      });
  const auto result = aarch64_codegen::print_machine_instruction_line_payloads(address);
  if (!result.ok || !result.diagnostic.empty()) {
    return fail("expected string address materialization to print ADRP/ADD");
  }
  if (result.instruction_lines.size() != 2) {
    return fail("expected string address materialization to print two lines");
  }
  if (const int check =
          expect_equal(result.instruction_lines[0], "adrp x10, .L.str0", "string address ADRP");
      check != 0) {
    return check;
  }
  return expect_equal(
      result.instruction_lines[1], "add x10, x10, :lo12:.L.str0", "string address low12 ADD");
}

int remaining_address_materialization_printer_paths_use_structured_facts() {
  prepare::PreparedAddressMaterialization source;
  const auto tls = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::TlsRelative,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::TlsGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 6,
          .result_value_id = prepare::PreparedValueId{44},
          .result_value_name = c4c::ValueNameId{45},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(11),
          .symbol_name = c4c::LinkNameId{46},
          .symbol_label = "g.tls",
          .address_space = bir::AddressSpace::Tls,
          .is_thread_local = true,
          .has_tls_address_space = true,
          .tls_model = prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative,
          .tls_thread_pointer_register =
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0,
          .tls_high_relocation = prepare::PreparedTlsRelocationKind::Aarch64TprelHi12,
          .tls_low_relocation = prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc,
          .source_materialization = &source,
      });
  const auto tls_result = aarch64_codegen::print_machine_instruction_line_payloads(tls);
  if (!tls_result.ok || !tls_result.diagnostic.empty()) {
    return fail("expected TLS address materialization to print local-exec sequence");
  }
  if (tls_result.instruction_lines.size() != 3) {
    return fail("expected TLS address materialization to print three lines");
  }
  if (const int check =
          expect_equal(tls_result.instruction_lines[0], "mrs x11, tpidr_el0", "TLS MRS");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(tls_result.instruction_lines[1],
                                     "add x11, x11, :tprel_hi12:g.tls",
                                     "TLS high relocation ADD");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(tls_result.instruction_lines[2],
                                     "add x11, x11, :tprel_lo12_nc:g.tls",
                                     "TLS low relocation ADD");
      check != 0) {
    return check;
  }

  const auto got = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::GotPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 7,
          .result_value_id = prepare::PreparedValueId{46},
          .result_value_name = c4c::ValueNameId{47},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(12),
          .symbol_name = c4c::LinkNameId{48},
          .symbol_label = "g.got",
          .address_materialization_policy =
              bir::GlobalAddressMaterializationPolicy::GotRequired,
          .source_materialization = &source,
      });
  const auto got_result = aarch64_codegen::print_machine_instruction_line_payloads(got);
  if (!got_result.ok || !got_result.diagnostic.empty()) {
    return fail("expected GOT address materialization to print GOT load sequence");
  }
  if (got_result.instruction_lines.size() != 2) {
    return fail("expected GOT address materialization to print two lines");
  }
  if (const int check =
          expect_equal(got_result.instruction_lines[0], "adrp x12, :got:g.got", "GOT ADRP");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(got_result.instruction_lines[1],
                                     "ldr x12, [x12, :got_lo12:g.got]",
                                     "GOT low12 LDR");
      check != 0) {
    return check;
  }

  const auto label = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::LabelPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::Label,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 8,
          .result_value_id = prepare::PreparedValueId{48},
          .result_value_name = c4c::ValueNameId{49},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(13),
          .target_label = c4c::BlockLabelId{50},
          .target_label_name = "target",
          .source_materialization = &source,
      });
  const auto label_result = aarch64_codegen::print_machine_instruction_line_payloads(label);
  if (!label_result.ok || !label_result.diagnostic.empty()) {
    return fail("expected label address materialization to print ADRP/ADD");
  }
  if (label_result.instruction_lines.size() != 2) {
    return fail("expected label address materialization to print two lines");
  }
  if (const int check =
          expect_equal(label_result.instruction_lines[0], "adrp x13, target", "label ADRP");
      check != 0) {
    return check;
  }
  if (const int check = expect_equal(label_result.instruction_lines[1],
                                     "add x13, x13, :lo12:target",
                                     "label low12 ADD");
      check != 0) {
    return check;
  }

  const auto missing_got_policy = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::GotPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 9,
          .result_value_id = prepare::PreparedValueId{50},
          .result_value_name = c4c::ValueNameId{51},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(14),
          .symbol_name = c4c::LinkNameId{52},
          .symbol_label = "g.got",
          .source_materialization = &source,
      });
  const auto missing_got_policy_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_got_policy);
  if (missing_got_policy_result.ok ||
      missing_got_policy_result.diagnostic.find("GOT-required policy") == std::string::npos) {
    return fail("expected GOT address materialization without policy to fail closed");
  }

  const auto missing_label_text = aarch64_codegen::make_address_materialization_instruction(
      aarch64_codegen::AddressMaterializationRecord{
          .kind = aarch64_codegen::AddressMaterializationKind::LabelPageLow12,
          .prepared_kind = prepare::PreparedAddressMaterializationKind::Label,
          .function_name = c4c::FunctionNameId{2},
          .block_label = c4c::BlockLabelId{3},
          .instruction_index = 10,
          .result_value_id = prepare::PreparedValueId{52},
          .result_value_name = c4c::ValueNameId{53},
          .result_home_kind = prepare::PreparedValueHomeKind::Register,
          .result_register = xreg(15),
          .target_label = c4c::BlockLabelId{54},
          .source_materialization = &source,
      });
  const auto missing_label_text_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_label_text);
  if (missing_label_text_result.ok ||
      missing_label_text_result.diagnostic.find("target label text") == std::string::npos) {
    return fail("expected label address materialization without label text to fail closed");
  }
  return 0;
}

int selected_inline_asm_template_prints_from_structured_operands() {
  const auto inline_asm =
      selected_inline_asm_instruction(selected_inline_asm_record());
  const auto result = print_common_instruction_nodes({inline_asm});
  if (!result.ok) {
    return fail("expected selected inline-asm template to print: " +
                result.diagnostic);
  }
  if (const int check = expect_equal(result.assembly,
                                     "    add w3, w3, w5\n"
                                     "    mov x3, #7\n",
                                     "selected inline-asm substitution");
      check != 0) {
    return check;
  }

  auto alias_record = selected_inline_asm_record("add %w0, %x1, %w2");
  alias_record.inline_asm_operands[1].home->register_name = std::string{"x3"};
  alias_record.inline_asm_operands[1].selected_operand =
      aarch64_codegen::make_register_operand(xreg(3));
  const auto alias_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(alias_record)));
  if (!alias_result.ok || alias_result.instruction_lines.size() != 1 ||
      alias_result.instruction_lines.front() != "add w3, x3, w5") {
    return fail("expected selected inline-asm tied home authority to allow w3/x3 aliases");
  }

  const auto literal_percent = selected_inline_asm_instruction(
      selected_inline_asm_record("mov %0, %2 // %%literal"));
  const auto literal_result =
      aarch64_codegen::print_machine_instruction_line_payloads(literal_percent);
  if (!literal_result.ok) {
    return fail("expected selected inline-asm literal percent to print: " +
                literal_result.diagnostic);
  }
  if (const int check = expect_equal(literal_result.instruction_lines.front(),
                                     "mov w3, w5 // %literal",
                                     "selected inline-asm literal percent");
      check != 0) {
    return check;
  }

  auto named_record =
      selected_inline_asm_record("add %w[dst], %w[seed], %w[rhs]\n"
                                 "mov %x[dst], #%[imm]");
  named_record.inline_asm_has_named_operand_references = true;
  const auto named_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(named_record)));
  if (!named_result.ok) {
    return fail("expected selected inline-asm named operands to print: " +
                named_result.diagnostic);
  }
  if (named_result.instruction_lines.size() != 2 ||
      named_result.instruction_lines[0] != "add w3, w3, w5" ||
      named_result.instruction_lines[1] != "mov x3, #7") {
    return fail("expected selected inline-asm named substitution");
  }

  auto memory_record = selected_inline_asm_record("ldr %w0, %2");
  const aarch64_abi::RegisterReference x5{
      .bank = aarch64_abi::RegisterBank::GeneralPurpose,
      .view = aarch64_abi::RegisterView::X,
      .index = 5,
  };
  const aarch64_codegen::RegisterOperand memory_base{
      .reg = x5,
      .role = aarch64_codegen::RegisterOperandRole::ValueHome,
      .value_id = prepare::PreparedValueId{52},
      .value_name = c4c::ValueNameId{52},
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .occupied_register_references = {x5},
      .occupied_registers = {"x5"},
  };
  const auto memory_operand = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
          .base_register = memory_base,
          .pointer_value_name = c4c::ValueNameId{52},
          .pointer_value_id = prepare::PreparedValueId{52},
          .byte_offset = 8,
          .size_bytes = 4,
          .align_bytes = 4,
          .can_use_base_plus_offset = true,
      });
  memory_record.operands[2] = memory_operand;
  memory_record.inline_asm_operands[2].kind = bir::InlineAsmOperandKind::MemoryInput;
  memory_record.inline_asm_operands[2].constraint = "m";
  memory_record.inline_asm_operands[2].home->register_name = std::string{"x5"};
  memory_record.inline_asm_operands[2].selected_operand = memory_operand;
  const auto memory_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(memory_record)));
  if (!memory_result.ok || memory_result.instruction_lines.size() != 1 ||
      memory_result.instruction_lines.front() != "ldr w3, [x5, #8]") {
    return fail("expected selected inline-asm memory operand to print from structured address");
  }

  auto address_record = selected_inline_asm_record("adr %x0, %2");
  const auto address_operand = aarch64_codegen::make_memory_operand(
      aarch64_codegen::MemoryOperand{
          .surface = aarch64_codegen::RecordSurfaceKind::MachineInstructionNode,
          .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
          .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
          .base_register = memory_base,
          .pointer_value_name = c4c::ValueNameId{52},
          .pointer_value_id = prepare::PreparedValueId{52},
          .byte_offset = 16,
          .size_bytes = 8,
          .align_bytes = 8,
          .can_use_base_plus_offset = true,
      });
  address_record.operands[2] = address_operand;
  address_record.inline_asm_operands[2].kind = bir::InlineAsmOperandKind::AddressInput;
  address_record.inline_asm_operands[2].constraint = "p";
  address_record.inline_asm_operands[2].home->register_name = std::string{"x5"};
  address_record.inline_asm_operands[2].selected_operand = address_operand;
  const auto address_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(address_record)));
  if (!address_result.ok || address_result.instruction_lines.size() != 1 ||
      address_result.instruction_lines.front() != "adr x3, [x5, #16]") {
    return fail("expected selected inline-asm address operand to print from structured address");
  }

  auto clobber_list = selected_inline_asm_record("add %w0, %w1, %w2");
  clobber_list.inline_asm_clobbers = {"x7", "memory", "cc"};
  const auto clobber_list_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(clobber_list)));
  if (!clobber_list_result.ok ||
      clobber_list_result.instruction_lines.size() != 1 ||
      clobber_list_result.instruction_lines.front() != "add w3, w3, w5") {
    return fail("expected selected inline-asm clobber list to print from record facts");
  }
  return 0;
}

int selected_inline_asm_template_rejects_incomplete_or_unsupported_records() {
  auto unknown_operand = selected_inline_asm_instruction(
      selected_inline_asm_record("add %w0, %w1, %w9"));
  const auto unknown_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unknown_operand);
  if (unknown_result.ok ||
      unknown_result.diagnostic.find("unknown operand") == std::string::npos) {
    return fail("expected inline-asm unknown operand to fail closed");
  }

  auto unsupported_modifier = selected_inline_asm_instruction(
      selected_inline_asm_record("add %q0, %w1, %w2"));
  const auto modifier_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unsupported_modifier);
  if (modifier_result.ok ||
      modifier_result.diagnostic.find("unsupported template modifier") ==
          std::string::npos) {
    return fail("expected inline-asm unsupported modifier to fail closed");
  }

  auto missing_selected = selected_inline_asm_record("add %w0, %w1, %w2");
  missing_selected.inline_asm_operands[2].selected_operand = std::nullopt;
  const auto missing_selected_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(missing_selected)));
  if (missing_selected_result.ok ||
      missing_selected_result.diagnostic.find("missing selected operand") ==
          std::string::npos) {
    return fail("expected inline-asm missing selected operand to fail closed");
  }

  auto missing_tie = selected_inline_asm_record("add %w0, %w1, %w2");
  missing_tie.inline_asm_operands[1].tied_output_index = std::nullopt;
  const auto missing_tie_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(missing_tie)));
  if (missing_tie_result.ok ||
      missing_tie_result.diagnostic.find("missing tied output index") ==
          std::string::npos) {
    return fail("expected inline-asm missing tie to fail closed");
  }

  auto missing_tied_output_selected =
      selected_inline_asm_record("mov %w1, %w2");
  missing_tied_output_selected.inline_asm_operands[0].selected_operand =
      std::nullopt;
  const auto missing_tied_output_selected_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(missing_tied_output_selected)));
  if (missing_tied_output_selected_result.ok ||
      missing_tied_output_selected_result.diagnostic.find(
          "missing selected tied output operand") == std::string::npos) {
    return fail("expected inline-asm missing tied output selected operand to fail closed");
  }

  auto missing_tie_home = selected_inline_asm_record("add %w0, %w1, %w2");
  missing_tie_home.inline_asm_operands[1].home = std::nullopt;
  const auto missing_tie_home_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(missing_tie_home)));
  if (missing_tie_home_result.ok ||
      missing_tie_home_result.diagnostic.find("missing prepared tied home") ==
          std::string::npos) {
    return fail("expected inline-asm missing tied home to fail closed");
  }

  auto allocator_dependent_tie =
      selected_inline_asm_record("add %w0, %w1, %w2");
  allocator_dependent_tie.inline_asm_operands[1].home->register_name =
      std::nullopt;
  const auto allocator_dependent_tie_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(allocator_dependent_tie)));
  if (allocator_dependent_tie_result.ok ||
      allocator_dependent_tie_result.diagnostic.find(
          "concrete prepared register homes") == std::string::npos) {
    return fail("expected inline-asm allocator-dependent tied home to fail closed");
  }

  auto mismatched_tie_home =
      selected_inline_asm_record("add %w0, %w1, %w2");
  mismatched_tie_home.inline_asm_operands[1].home->register_name =
      std::string{"w4"};
  mismatched_tie_home.inline_asm_operands[1].home->target_register_identity =
      gpr_identity(4);
  const auto mismatched_tie_home_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(mismatched_tie_home)));
  if (mismatched_tie_home_result.ok ||
      mismatched_tie_home_result.diagnostic.find("prepared home disagrees") ==
          std::string::npos) {
    return fail("expected inline-asm mismatched tied home to fail closed");
  }

  auto target_invalid_tie_home =
      selected_inline_asm_record("add %w0, %w1, %w2");
  target_invalid_tie_home.inline_asm_operands[1].home->register_name =
      std::string{"sp"};
  target_invalid_tie_home.inline_asm_operands[1]
      .home->target_register_identity = std::nullopt;
  const auto target_invalid_tie_home_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(target_invalid_tie_home)));
  if (target_invalid_tie_home_result.ok ||
      target_invalid_tie_home_result.diagnostic.find(
          "missing prepared coallocation authority") == std::string::npos) {
    return fail("expected inline-asm target-invalid tied home to fail closed");
  }

  auto class_invalid_tie_home =
      selected_inline_asm_record("add %w0, %w1, %w2");
  class_invalid_tie_home.inline_asm_operands[1].home->register_name =
      std::string{"s3"};
  class_invalid_tie_home.inline_asm_operands[1]
      .home->target_register_identity =
      prepare::PreparedTargetRegisterIdentity{
          .target_arch = c4c::TargetArch::Aarch64,
          .bank = prepare::PreparedRegisterBank::Fpr,
          .register_class = prepare::PreparedRegisterClass::Float,
          .physical_index = 3,
      };
  const auto class_invalid_tie_home_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(class_invalid_tie_home)));
  if (class_invalid_tie_home_result.ok ||
      class_invalid_tie_home_result.diagnostic.find(
          "incompatible prepared register class") == std::string::npos) {
    return fail("expected inline-asm class-invalid tied home to fail closed");
  }

  auto selected_tie_disagrees =
      selected_inline_asm_record("add %w0, %w1, %w2");
  selected_tie_disagrees.inline_asm_operands[1].selected_operand =
      aarch64_codegen::make_register_operand(wreg(4));
  const auto selected_tie_disagrees_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(selected_tie_disagrees)));
  if (selected_tie_disagrees_result.ok ||
      selected_tie_disagrees_result.diagnostic.find(
          "selected register disagrees with prepared home") == std::string::npos) {
    return fail("expected inline-asm tied selected register mismatch to fail closed");
  }

  auto selected_output_disagrees =
      selected_inline_asm_record("add %w0, %w1, %w2");
  selected_output_disagrees.inline_asm_operands[0].selected_operand =
      aarch64_codegen::make_register_operand(wreg(4));
  const auto selected_output_disagrees_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(selected_output_disagrees)));
  if (selected_output_disagrees_result.ok ||
      selected_output_disagrees_result.diagnostic.find(
          "selected register disagrees with prepared home") == std::string::npos) {
    return fail("expected inline-asm output selected register mismatch to fail closed");
  }

  auto unsupported_constraint = selected_inline_asm_record("add %w0, %w1, %w2");
  unsupported_constraint.inline_asm_operands[2].constraint = "m";
  const auto unsupported_constraint_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(unsupported_constraint)));
  if (unsupported_constraint_result.ok ||
      unsupported_constraint_result.diagnostic.find("unsupported constraint") ==
          std::string::npos) {
    return fail("expected inline-asm unsupported constraint to fail closed");
  }

  auto memory_input = selected_inline_asm_record("ldr %w0, %1");
  memory_input.inline_asm_operands[2].kind = bir::InlineAsmOperandKind::MemoryInput;
  memory_input.inline_asm_operands[2].constraint = "m";
  memory_input.inline_asm_operands[2].selected_operand = std::nullopt;
  const auto memory_input_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(memory_input)));
  if (memory_input_result.ok ||
      memory_input_result.diagnostic.find("structured memory address authority") ==
          std::string::npos) {
    return fail("expected inline-asm memory input to fail closed without selected authority");
  }

  auto address_input = selected_inline_asm_record("adr %x0, %1");
  address_input.inline_asm_operands[2].kind = bir::InlineAsmOperandKind::AddressInput;
  address_input.inline_asm_operands[2].constraint = "p";
  address_input.inline_asm_operands[2].selected_operand = std::nullopt;
  const auto address_input_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(address_input)));
  if (address_input_result.ok ||
      address_input_result.diagnostic.find("structured address authority") ==
          std::string::npos) {
    return fail("expected inline-asm address input to fail closed without selected authority");
  }

  auto unknown_named = selected_inline_asm_record("add %w0, %w1, %[missing]");
  unknown_named.inline_asm_has_named_operand_references = true;
  const auto unknown_named_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(unknown_named)));
  if (unknown_named_result.ok ||
      unknown_named_result.diagnostic.find("unknown named operand") ==
          std::string::npos) {
    return fail("expected inline-asm unknown named operand to fail closed");
  }

  auto duplicate_named = selected_inline_asm_record("add %w0, %w1, %[rhs]");
  duplicate_named.inline_asm_has_named_operand_references = true;
  duplicate_named.inline_asm_operands[3].name = std::string{"rhs"};
  const auto duplicate_named_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(duplicate_named)));
  if (duplicate_named_result.ok ||
      duplicate_named_result.diagnostic.find("duplicate named operand") ==
          std::string::npos) {
    return fail("expected inline-asm duplicate named operand to fail closed");
  }

  auto malformed_named = selected_inline_asm_record("add %w0, %w1, %[rhs");
  malformed_named.inline_asm_has_named_operand_references = true;
  const auto malformed_named_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(malformed_named)));
  if (malformed_named_result.ok ||
      malformed_named_result.diagnostic.find("malformed named operand") ==
          std::string::npos) {
    return fail("expected inline-asm malformed named operand to fail closed");
  }

  auto missing_name = selected_inline_asm_record("add %w0, %w1, %[]");
  missing_name.inline_asm_has_named_operand_references = true;
  const auto missing_name_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(missing_name)));
  if (missing_name_result.ok ||
      missing_name_result.diagnostic.find("missing named operand name") ==
          std::string::npos) {
    return fail("expected inline-asm missing named operand name to fail closed");
  }

  auto unsupported_named = selected_inline_asm_record("add %w0, %w1, %[rhs]");
  unsupported_named.inline_asm_has_named_operand_references = true;
  unsupported_named.inline_asm_operands[2].constraint = "m";
  const auto unsupported_named_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(unsupported_named)));
  if (unsupported_named_result.ok ||
      unsupported_named_result.diagnostic.find("unsupported constraint") ==
          std::string::npos) {
    return fail("expected inline-asm unsupported named operand to fail closed");
  }

  auto clobber = selected_inline_asm_record("add %w0, %w1, %w2");
  clobber.inline_asm_operands[2].kind = bir::InlineAsmOperandKind::Clobber;
  clobber.inline_asm_operands[2].constraint = "~{x1}";
  const auto clobber_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          selected_inline_asm_instruction(std::move(clobber)));
  if (clobber_result.ok ||
      clobber_result.diagnostic.find("structured clobber authority") ==
          std::string::npos) {
    return fail("expected inline-asm clobber operand to fail closed");
  }

  return 0;
}

int unsupported_surfaces_statuses_and_missing_operands_fail_closed() {
  const auto assembler = aarch64_codegen::make_assembler_instruction(
      aarch64_codegen::AssemblerInstructionRecord{});
  const auto assembler_result =
      aarch64_codegen::print_machine_instruction_line_payloads(assembler);
  if (assembler_result.ok ||
      assembler_result.diagnostic.find("surface machine_instruction_node") ==
          std::string::npos) {
    return fail("expected external assembler input to fail closed before printing");
  }

  const auto unsupported = aarch64_codegen::make_unsupported_machine_instruction(
      aarch64_codegen::InstructionFamily::Memory,
      aarch64_codegen::MachineNodeSelectionStatus::DeferredUnsupported,
      "fixture unsupported");
  const auto unsupported_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unsupported);
  if (unsupported_result.ok ||
      unsupported_result.diagnostic.find("deferred_unsupported: fixture unsupported") ==
          std::string::npos) {
    return fail("expected non-selected machine node to report selection diagnostic");
  }

  const auto scalar = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Add,
          .source_binary_opcode = bir::BinaryOpcode::Add,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{30},
          .result_value_name = c4c::ValueNameId{31},
          .result_type = bir::TypeKind::I64,
          .lhs = aarch64_codegen::make_register_operand(xreg(2)),
          .rhs = aarch64_codegen::make_register_operand(xreg(3)),
          .supported_integer_operation = true,
      }));
  const auto scalar_result =
      aarch64_codegen::print_machine_instruction_line_payloads(scalar);
  if (scalar_result.ok ||
      scalar_result.diagnostic.find("missing a structured destination register operand") ==
          std::string::npos) {
    return fail("expected selected scalar without destination register to fail closed");
  }

  const auto scalar_with_materialized_sub_lhs = aarch64_codegen::make_scalar_instruction(
      aarch64_codegen::make_scalar_alu_instruction_record(aarch64_codegen::ScalarAluRecord{
          .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
          .operation = aarch64_codegen::ScalarAluOperationKind::Sub,
          .source_binary_opcode = bir::BinaryOpcode::Sub,
          .operand_type = bir::TypeKind::I64,
          .result_value_id = prepare::PreparedValueId{32},
          .result_value_name = c4c::ValueNameId{33},
          .result_type = bir::TypeKind::I64,
          .result_register = xreg(0),
          .lhs = aarch64_codegen::make_immediate_operand(aarch64_codegen::ImmediateOperand{
              .kind = aarch64_codegen::ImmediateKind::SignedInteger,
              .type = bir::TypeKind::I64,
              .signed_value = 1,
          }),
          .rhs = aarch64_codegen::make_register_operand(xreg(2)),
          .supported_integer_operation = true,
      }));
  const auto scalar_with_materialized_sub_lhs_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          scalar_with_materialized_sub_lhs);
  if (!scalar_with_materialized_sub_lhs_result.ok ||
      scalar_with_materialized_sub_lhs_result.instruction_lines.size() != 2 ||
      scalar_with_materialized_sub_lhs_result.instruction_lines[0] != "mov x9, #1" ||
      scalar_with_materialized_sub_lhs_result.instruction_lines[1] != "sub x0, x9, x2") {
    return fail("expected selected scalar sub with immediate lhs to materialize lhs: " +
                join_lines(scalar_with_materialized_sub_lhs_result.instruction_lines));
  }

  auto load_address = frame_slot(48);
  load_address.result_value_id = prepare::PreparedValueId{34};
  load_address.result_value_name = c4c::ValueNameId{35};
  const auto load_missing_destination = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Load,
          .address = load_address,
          .result_value_id = prepare::PreparedValueId{34},
          .result_value_name = c4c::ValueNameId{35},
          .value_type = bir::TypeKind::I32,
      });
  const auto load_missing_destination_result =
      aarch64_codegen::print_machine_instruction_line_payloads(load_missing_destination);
  if (load_missing_destination_result.ok ||
      load_missing_destination_result.diagnostic.find(
          "load node is missing a structured destination register operand") ==
          std::string::npos) {
    return fail("expected selected load without destination register to fail closed");
  }

  const auto pointer_store_missing_base = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address =
              aarch64_codegen::MemoryOperand{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .support = aarch64_codegen::MemoryOperandSupportKind::Prepared,
                  .function_name = c4c::FunctionNameId{2},
                  .block_label = c4c::BlockLabelId{3},
                  .stored_value_id = prepare::PreparedValueId{36},
                  .stored_value_name = c4c::ValueNameId{37},
                  .base_kind = aarch64_codegen::MemoryBaseKind::PointerValue,
                  .byte_offset = 8,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .can_use_base_plus_offset = true,
              },
          .value = aarch64_codegen::make_register_operand(wreg(4)),
          .value_type = bir::TypeKind::I32,
      });
  const auto pointer_store_missing_base_result =
      aarch64_codegen::print_machine_instruction_line_payloads(pointer_store_missing_base);
  if (pointer_store_missing_base_result.ok ||
      pointer_store_missing_base_result.diagnostic.find(
          "memory address is not printable") == std::string::npos) {
    return fail("expected pointer-value store without base register to fail closed");
  }

  auto unprepared_store_address = frame_slot(56);
  unprepared_store_address.support =
      aarch64_codegen::MemoryOperandSupportKind::DeferredUnsupported;
  unprepared_store_address.stored_value_id = prepare::PreparedValueId{38};
  unprepared_store_address.stored_value_name = c4c::ValueNameId{39};
  const auto unprepared_store = aarch64_codegen::make_memory_instruction(
      aarch64_codegen::MemoryInstructionRecord{
          .memory_kind = aarch64_codegen::MemoryInstructionKind::Store,
          .address = unprepared_store_address,
          .value = aarch64_codegen::make_register_operand(wreg(5)),
          .value_type = bir::TypeKind::I32,
      });
  const auto unprepared_store_result =
      aarch64_codegen::print_machine_instruction_line_payloads(unprepared_store);
  if (unprepared_store_result.ok ||
      unprepared_store_result.diagnostic.find("memory operand is outside the selected subset") ==
          std::string::npos) {
    return fail("expected unprepared memory base to fail closed before printing");
  }

  const auto call_missing_provenance = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{10},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "actual_function",
          .calling_convention = bir::CallingConv::C,
      });
  const auto call_missing_provenance_result =
      aarch64_codegen::print_machine_instruction_line_payloads(call_missing_provenance);
  if (call_missing_provenance_result.ok ||
      call_missing_provenance_result.diagnostic.find("missing prepared callee provenance") ==
          std::string::npos) {
    return fail("expected selected direct call without prepared provenance to fail closed");
  }

  const auto indirect_call_missing_provenance = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .indirect_callee =
              aarch64_codegen::make_register_operand(aarch64_codegen::RegisterOperand{
                  .reg = aarch64_abi::x_register(9),
                  .role = aarch64_codegen::RegisterOperandRole::CallAbi,
              }),
          .calling_convention = bir::CallingConv::C,
          .is_indirect = true,
      });
  const auto indirect_call_missing_provenance_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          indirect_call_missing_provenance);
  if (indirect_call_missing_provenance_result.ok ||
      indirect_call_missing_provenance_result.diagnostic.find(
          "missing prepared callee provenance") == std::string::npos) {
    return fail("expected selected indirect call without prepared provenance to fail closed");
  }

  const prepare::PreparedCallPlan prepared_va_start_call{
      .block_index = 0,
      .instruction_index = 2,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_start.p0"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_start_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .block_index = 0,
      .instruction_index = 2,
      .destination_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{14},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{4},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x2"},
          },
      .destination_va_list_address =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{14},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{4},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{7},
              .offset_bytes = std::size_t{240},
              .size_bytes = std::size_t{32},
              .align_bytes = std::size_t{8},
          },
  };
  const prepare::PreparedVariadicEntryPlanFunction variadic_entry{
      .function_name = c4c::FunctionNameId{2},
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
                          .kind = prepare::PreparedVariadicVaListFieldKind::GpOffset,
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
              .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaStart},
              .scratch_register_count = std::size_t{1},
              .scratch_stack_bytes = std::size_t{0},
          },
      .helper_operand_homes = {va_start_homes},
  };
  const auto va_start_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{11},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_start.p0",
          .wrapper_kind = prepared_va_start_call.wrapper_kind,
          .source_call = &prepared_va_start_call,
          .source_variadic_entry = &variadic_entry,
          .source_variadic_helper_operand_homes = &variadic_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
          .calling_convention = bir::CallingConv::C,
      });
  const auto va_start_result =
      aarch64_codegen::print_machine_instruction_line_payloads(va_start_call);
  if (!va_start_result.ok || va_start_result.instruction_lines.size() != 21 ||
      va_start_result.instruction_lines[0] != "add x2, sp, #240" ||
      va_start_result.instruction_lines[1] != "str x1, [sp, #16]" ||
      va_start_result.instruction_lines[7] != "str x7, [sp, #64]" ||
      va_start_result.instruction_lines[8] != "str q0, [sp, #80]" ||
      va_start_result.instruction_lines[15] != "str q7, [sp, #192]" ||
      va_start_result.instruction_lines[16] != "add x9, sp, #208" ||
      va_start_result.instruction_lines[17] != "str x9, [x2, #8]" ||
      va_start_result.instruction_lines[18] != "movz w9, #65480" ||
      va_start_result.instruction_lines[19] != "movk w9, #65535, lsl #16" ||
      va_start_result.instruction_lines[20] != "str w9, [x2]") {
    return fail("expected variadic entry helper call to lower va_start to legal assembly");
  }

  const prepare::PreparedCallPlan prepared_va_arg_call{
      .block_index = 0,
      .instruction_index = 3,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_arg.i32"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_arg_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
      .block_index = 0,
      .instruction_index = 3,
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{15},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{5},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x3"},
          },
      .scalar_result =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{16},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{6},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"w0"},
          },
      .scalar_access_plan =
          prepare::PreparedVariadicScalarVaArgAccessPlan{
              .source_class =
                  prepare::PreparedVariadicScalarVaArgSourceClass::OverflowArgArea,
              .value_type = bir::TypeKind::I32,
              .value_size_bytes = 4,
              .value_align_bytes = 4,
              .result_home =
                  prepare::PreparedValueHome{
                      .value_id = prepare::PreparedValueId{16},
                      .function_name = c4c::FunctionNameId{2},
                      .value_name = c4c::ValueNameId{6},
                      .kind = prepare::PreparedValueHomeKind::Register,
                      .register_name = std::string{"w0"},
                  },
              .source_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .source_field_offset_bytes = std::size_t{8},
              .source_slot_size_bytes = std::size_t{8},
              .progression_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .progression_field_offset_bytes = std::size_t{8},
              .progression_stride_bytes = std::size_t{8},
              .overflow_source_field =
                  prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
              .overflow_source_field_offset_bytes = std::size_t{8},
              .overflow_stride_bytes = std::size_t{8},
          },
  };
  auto scalar_va_arg_entry = variadic_entry;
  scalar_va_arg_entry.helper_resources.required_helpers =
      {prepare::PreparedVariadicEntryHelperKind::VaArg};
  scalar_va_arg_entry.helper_resources.scratch_register_count = std::size_t{2};
  scalar_va_arg_entry.helper_operand_homes = {va_arg_homes};
  const auto va_arg_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{12},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.i32",
          .wrapper_kind = prepared_va_arg_call.wrapper_kind,
          .source_call = &prepared_va_arg_call,
          .source_variadic_entry = &scalar_va_arg_entry,
          .source_variadic_helper_operand_homes =
              &scalar_va_arg_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto va_arg_result =
      aarch64_codegen::print_machine_instruction_line_payloads(va_arg_call);
  if (!va_arg_result.ok || va_arg_result.instruction_lines.size() != 3 ||
      va_arg_result.instruction_lines[0].find(
          "va.arg.scalar source=overflow_arg_area va_list=value#15:register:x3 result=value#16:register:w0") ==
          std::string::npos ||
      va_arg_result.instruction_lines[1].find(
          "field=overflow_arg_area field_offset=8 slot_size=8") ==
          std::string::npos ||
      va_arg_result.instruction_lines[2].find(
          "progress field=overflow_arg_area field_offset=8 stride=8") ==
          std::string::npos ||
      va_arg_result.instruction_lines[2].find(
          "overflow_slot#6 overflow_stack+208") == std::string::npos) {
    return fail("expected scalar va_arg helper call to print prepared access-plan records");
  }

  auto scalar_fp_va_arg_entry = scalar_va_arg_entry;
  auto fp_homes = scalar_fp_va_arg_entry.helper_operand_homes.front();
  fp_homes.scalar_result =
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{17},
          .function_name = c4c::FunctionNameId{2},
          .value_name = c4c::ValueNameId{7},
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"d0"},
      };
  fp_homes.scalar_access_plan =
      prepare::PreparedVariadicScalarVaArgAccessPlan{
          .source_class =
              prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea,
          .value_type = bir::TypeKind::F64,
          .value_size_bytes = 8,
          .value_align_bytes = 8,
          .result_home = fp_homes.scalar_result,
          .source_field =
              prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea,
          .source_field_offset_bytes = std::size_t{24},
          .source_slot_size_bytes = std::size_t{16},
          .progression_field =
              prepare::PreparedVariadicVaListFieldKind::FpOffset,
          .progression_field_offset_bytes = std::size_t{4},
          .progression_stride_bytes = std::size_t{16},
          .overflow_source_field =
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
          .overflow_source_field_offset_bytes = std::size_t{8},
          .overflow_stride_bytes = std::size_t{8},
      };
  scalar_fp_va_arg_entry.helper_operand_homes = {fp_homes};
  const auto fp_va_arg_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{13},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.f64",
          .wrapper_kind = prepared_va_arg_call.wrapper_kind,
          .source_call = &prepared_va_arg_call,
          .source_variadic_entry = &scalar_fp_va_arg_entry,
          .source_variadic_helper_operand_homes =
              &scalar_fp_va_arg_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
          .calling_convention = bir::CallingConv::C,
      });
  const auto fp_va_arg_result =
      aarch64_codegen::print_machine_instruction_line_payloads(fp_va_arg_call);
  if (!fp_va_arg_result.ok || fp_va_arg_result.instruction_lines.size() != 3 ||
      fp_va_arg_result.instruction_lines[0].find(
          "va.arg.scalar source=fp_register_save_area va_list=value#15:register:x3 result=value#17:register:d0") ==
          std::string::npos ||
      fp_va_arg_result.instruction_lines[1].find(
          "field=fp_register_save_area field_offset=24 slot_size=16") ==
          std::string::npos ||
      fp_va_arg_result.instruction_lines[1].find(
          "fp_base=64 gp_slot=8 fp_slot=16") == std::string::npos ||
      fp_va_arg_result.instruction_lines[2].find(
          "progress field=fp_offset field_offset=4 stride=16") ==
          std::string::npos) {
    return fail("expected scalar fp va_arg helper call to print prepared access-plan records");
  }

  const prepare::PreparedCallPlan prepared_aggregate_va_arg_call{
      .block_index = 0,
      .instruction_index = 4,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_arg.aggregate"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes aggregate_va_arg_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
      .block_index = 0,
      .instruction_index = 4,
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{18},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{8},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x4"},
          },
      .aggregate_destination_payload =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{19},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{9},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{9},
              .offset_bytes = std::size_t{32},
          },
      .aggregate_access_plan =
          prepare::PreparedVariadicAggregateVaArgAccessPlan{
              .source_class =
                  prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
              .payload_size_bytes = 24,
              .payload_align_bytes = 8,
              .destination_payload_home =
                  prepare::PreparedValueHome{
                      .value_id = prepare::PreparedValueId{19},
                      .function_name = c4c::FunctionNameId{2},
                      .value_name = c4c::ValueNameId{9},
                      .kind = prepare::PreparedValueHomeKind::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{9},
                      .offset_bytes = std::size_t{32},
                  },
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
          },
  };
  auto aggregate_va_arg_entry = variadic_entry;
  aggregate_va_arg_entry.helper_resources.required_helpers =
      {prepare::PreparedVariadicEntryHelperKind::VaArgAggregate};
  aggregate_va_arg_entry.helper_resources.scratch_register_count = std::size_t{2};
  aggregate_va_arg_entry.helper_operand_homes = {aggregate_va_arg_homes};
  const auto aggregate_va_arg_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{14},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_arg.aggregate",
          .wrapper_kind = prepared_aggregate_va_arg_call.wrapper_kind,
          .source_call = &prepared_aggregate_va_arg_call,
          .source_variadic_entry = &aggregate_va_arg_entry,
          .source_variadic_helper_operand_homes =
              &aggregate_va_arg_entry.helper_operand_homes.front(),
          .variadic_entry_helper =
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
          .calling_convention = bir::CallingConv::C,
      });
  const auto aggregate_va_arg_result =
      aarch64_codegen::print_machine_instruction_line_payloads(
          aggregate_va_arg_call);
  if (!aggregate_va_arg_result.ok ||
      aggregate_va_arg_result.instruction_lines.size() != 14 ||
      aggregate_va_arg_result.instruction_lines[0] != "ldr x9, [x4, #8]" ||
      aggregate_va_arg_result.instruction_lines[1] != "ldr x9, [x9]" ||
      aggregate_va_arg_result.instruction_lines[2] != "str x9, [sp, #32]" ||
      aggregate_va_arg_result.instruction_lines[3] != "ldr x9, [x4, #8]" ||
      aggregate_va_arg_result.instruction_lines[4] != "add x9, x9, #8" ||
      aggregate_va_arg_result.instruction_lines[5] != "ldr x9, [x9]" ||
      aggregate_va_arg_result.instruction_lines[6] != "str x9, [sp, #40]" ||
      aggregate_va_arg_result.instruction_lines[7] != "ldr x9, [x4, #8]" ||
      aggregate_va_arg_result.instruction_lines[8] != "add x9, x9, #16" ||
      aggregate_va_arg_result.instruction_lines[9] != "ldr x9, [x9]" ||
      aggregate_va_arg_result.instruction_lines[10] != "str x9, [sp, #48]" ||
      aggregate_va_arg_result.instruction_lines[11] != "ldr x9, [x4, #8]" ||
      aggregate_va_arg_result.instruction_lines[12] != "add x9, x9, #24" ||
      aggregate_va_arg_result.instruction_lines[13] != "str x9, [x4, #8]") {
    return fail(
        "expected aggregate va_arg helper call to lower payload copy to executable assembly");
  }
  for (const auto& line : aggregate_va_arg_result.instruction_lines) {
    if (line.find("va.arg.aggregate") != std::string::npos) {
      return fail("expected aggregate va_arg helper lowering to omit raw helper text");
    }
  }

  const prepare::PreparedCallPlan prepared_va_copy_call{
      .block_index = 0,
      .instruction_index = 5,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"llvm.va_copy.p0.p0"},
  };
  const prepare::PreparedVariadicEntryHelperOperandHomes va_copy_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
      .block_index = 0,
      .instruction_index = 5,
      .destination_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{20},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{10},
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{10},
              .offset_bytes = std::size_t{64},
          },
      .source_va_list =
          prepare::PreparedValueHome{
              .value_id = prepare::PreparedValueId{21},
              .function_name = c4c::FunctionNameId{2},
              .value_name = c4c::ValueNameId{11},
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x5"},
          },
  };
  auto va_copy_entry = variadic_entry;
  va_copy_entry.helper_resources.required_helpers =
      {prepare::PreparedVariadicEntryHelperKind::VaCopy};
  va_copy_entry.helper_resources.scratch_register_count = std::size_t{1};
  va_copy_entry.helper_operand_homes = {va_copy_homes};
  const auto va_copy_call = aarch64_codegen::make_call_instruction(
      aarch64_codegen::CallInstructionRecord{
          .direct_callee =
              aarch64_codegen::SymbolOperand{
                  .link_name = c4c::LinkNameId{15},
                  .type = bir::TypeKind::Ptr,
                  .is_extern = true,
              },
          .direct_callee_label = "llvm.va_copy.p0.p0",
          .wrapper_kind = prepared_va_copy_call.wrapper_kind,
          .source_call = &prepared_va_copy_call,
          .source_variadic_entry = &va_copy_entry,
          .source_variadic_helper_operand_homes =
              &va_copy_entry.helper_operand_homes.front(),
          .variadic_entry_helper = prepare::PreparedVariadicEntryHelperKind::VaCopy,
          .calling_convention = bir::CallingConv::C,
      });
  const auto va_copy_result =
      aarch64_codegen::print_machine_instruction_line_payloads(va_copy_call);
  if (!va_copy_result.ok ||
      va_copy_result.instruction_lines.size() != 3 ||
      va_copy_result.instruction_lines[0].find(
          "va.copy dest=value#20:stack_slot:slot#10:offset+64 source=value#21:register:x5") ==
          std::string::npos ||
      va_copy_result.instruction_lines[0].find(
          "va_list_size=32 va_list_align=8 scratch_registers=1") ==
          std::string::npos ||
      va_copy_result.instruction_lines[1] !=
          "va.copy.field kind=gp_offset source_offset=0 destination_offset=0 size=4" ||
      va_copy_result.instruction_lines[2] !=
          "va.copy.field kind=overflow_arg_area source_offset=8 destination_offset=8 size=8") {
    return fail("expected va_copy helper call to print prepared layout field copies");
  }

  const auto frame_missing_provenance = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = c4c::FunctionNameId{2},
          .frame_size_bytes = 32,
          .frame_alignment_bytes = 16,
      });
  const auto frame_missing_provenance_result =
      aarch64_codegen::print_machine_instruction_line_payloads(frame_missing_provenance);
  if (frame_missing_provenance_result.ok ||
      frame_missing_provenance_result.diagnostic.find("prepared frame facts") ==
          std::string::npos) {
    return fail("expected selected frame without prepared provenance to fail closed");
  }

  const prepare::PreparedFramePlanFunction saved_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .saved_callee_registers = {prepared_saved_x19(16)},
  };
  const auto frame_with_saved_register = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = saved_frame.function_name,
          .frame_size_bytes = saved_frame.frame_size_bytes,
          .frame_alignment_bytes = saved_frame.frame_alignment_bytes,
          .saved_callee_registers = saved_frame.saved_callee_registers,
          .source_frame = &saved_frame,
      });
  const auto saved_result =
      aarch64_codegen::print_machine_instruction_line_payloads(frame_with_saved_register);
  if (!saved_result.ok) {
    return fail("expected frame with complete callee-save facts to print");
  }

  const prepare::PreparedFramePlanFunction dynamic_frame{
      .function_name = c4c::FunctionNameId{2},
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .has_dynamic_stack = true,
  };
  const auto dynamic_stack_frame = aarch64_codegen::make_frame_instruction(
      aarch64_codegen::FrameInstructionRecord{
          .frame_kind = aarch64_codegen::FrameInstructionKind::PrologueSetup,
          .function_name = dynamic_frame.function_name,
          .frame_size_bytes = dynamic_frame.frame_size_bytes,
          .frame_alignment_bytes = dynamic_frame.frame_alignment_bytes,
          .has_dynamic_stack = dynamic_frame.has_dynamic_stack,
          .source_frame = &dynamic_frame,
      });
  const auto dynamic_result =
      aarch64_codegen::print_machine_instruction_line_payloads(dynamic_stack_frame);
  if (dynamic_result.ok ||
      dynamic_result.diagnostic.find("dynamic-stack frame node is outside the printable subset") ==
          std::string::npos) {
    return fail("expected dynamic-stack frame to fail closed");
  }

  const prepare::PreparedMoveBundle call_boundary_bundle{
      .function_name = c4c::FunctionNameId{2},
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 0,
      .instruction_index = 4,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = prepare::PreparedValueId{70},
                  .to_value_id = prepare::PreparedValueId{71},
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
              },
          },
      .abi_bindings =
          {
              prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"x0"},
              },
          },
  };
  const auto call_boundary_move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = call_boundary_bundle.function_name,
          .phase = call_boundary_bundle.phase,
          .block_index = call_boundary_bundle.block_index,
          .instruction_index = call_boundary_bundle.instruction_index,
          .move = call_boundary_bundle.moves.front(),
          .source_bundle = &call_boundary_bundle,
          .source_move = &call_boundary_bundle.moves.front(),
      });
  const auto move_result =
      aarch64_codegen::print_machine_instruction_line_payloads(call_boundary_move);
  if (move_result.ok ||
      move_result.diagnostic.find(
          "call-boundary move node requires prepared register source and destination") ==
          std::string::npos) {
    return fail("expected call-boundary move record to fail closed in printer");
  }

  const auto call_boundary_binding =
      aarch64_codegen::make_call_boundary_abi_binding_instruction(
          aarch64_codegen::CallBoundaryAbiBindingInstructionRecord{
              .function_name = call_boundary_bundle.function_name,
              .phase = call_boundary_bundle.phase,
              .block_index = call_boundary_bundle.block_index,
              .instruction_index = call_boundary_bundle.instruction_index,
              .binding = call_boundary_bundle.abi_bindings.front(),
              .source_bundle = &call_boundary_bundle,
              .source_binding = &call_boundary_bundle.abi_bindings.front(),
          });
  const auto binding_result =
      aarch64_codegen::print_machine_instruction_line_payloads(call_boundary_binding);
  if (binding_result.ok ||
      binding_result.diagnostic.find(
          "call-boundary ABI binding node requires later AArch64 move lowering") ==
          std::string::npos) {
    return fail("expected call-boundary ABI binding record to fail closed in printer");
  }

  const auto missing_move = aarch64_codegen::make_call_boundary_move_instruction(
      aarch64_codegen::CallBoundaryMoveInstructionRecord{
          .function_name = c4c::FunctionNameId{2},
          .move = call_boundary_bundle.moves.front(),
      });
  const auto missing_move_result =
      aarch64_codegen::print_machine_instruction_line_payloads(missing_move);
  if (missing_move_result.ok ||
      missing_move_result.diagnostic.find("missing prepared move provenance") ==
          std::string::npos) {
    return fail("expected call-boundary move without provenance to fail closed");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = selected_spill_reload_nodes_print_gnu_aarch64_text(); result != 0) {
    return result;
  }
  if (const int result = selected_branch_and_store_nodes_print_without_semantic_roundtrip();
      result != 0) {
    return result;
  }
  if (const int result =
          fused_compare_branch_prints_immediate_left_operands_in_aarch64_order();
      result != 0) {
    return result;
  }
  if (const int result =
          fused_compare_branch_folds_both_immediate_operands_to_direct_branch();
      result != 0) {
    return result;
  }
  if (const int result =
          fused_compare_branch_materializes_nonencodable_immediate_operand();
      result != 0) {
    return result;
  }
  if (const int result = selected_branch_target_requires_matching_block_label_definition();
      result != 0) {
    return result;
  }
  if (const int result = selected_structured_memory_subset_prints_loads_and_stores();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_large_frame_slot_memory_offsets_materialize_address_scratch();
      result != 0) {
    return result;
  }
  if (const int result = selected_byte_stack_source_store_materializes_through_scratch();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_symbol_stack_source_store_materializes_through_value_scratch();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_symbol_halfword_immediate_store_materializes_through_value_scratch();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_stack_call_argument_store_prints_from_prepared_stack_facts();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_aggregate_stack_copy_prints_from_prepared_memory_operands();
      result != 0) {
    return result;
  }
  if (const int result =
          call_boundary_stack_printer_shapes_fail_closed_when_not_selected();
      result != 0) {
    return result;
  }
  if (const int result = selected_atomic_load_store_and_fence_print_from_structured_records();
      result != 0) {
    return result;
  }
  if (const int result = selected_atomic_rmw_loop_prints_from_structured_records();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_atomic_compare_exchange_loops_print_from_structured_records();
      result != 0) {
    return result;
  }
  if (const int result = selected_atomic_records_reject_missing_printer_facts();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_add_sub_and_register_return_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_fpr_register_return_publishes_abi_register();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_stack_publication_materializes_large_offset();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_alu_frame_slot_operands_materialize_before_printing();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_unsigned_power_of_two_reductions_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = narrow_unsigned_reductions_print_explicit_zero_extension();
      result != 0) {
    return result;
  }
  if (const int result = selected_unsigned_reductions_materialize_lhs_sources();
      result != 0) {
    return result;
  }
  if (const int result = signed_i32_add_sub_results_print_explicit_sign_extension();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_unary_integer_ops_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_simple_integer_casts_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_fp_arithmetic_prints_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_fp_unary_fabs_intrinsics_print_from_structured_records();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_fp_unary_fabs_intrinsics_reject_incomplete_printer_facts();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_crc_vector_intrinsics_print_from_structured_records();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_crc_vector_intrinsics_reject_incomplete_printer_facts();
      result != 0) {
    return result;
  }
  if (const int result =
          complete_unsupported_intrinsic_carriers_do_not_print_as_machine_records();
      result != 0) {
    return result;
  }
  if (const int result = selected_fp_conversions_print_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_i128_records_print_from_structured_fields(); result != 0) {
    return result;
  }
  if (const int result = selected_i128_helper_boundaries_print_from_structured_fields();
      result != 0) {
    return result;
  }
  if (const int result = selected_f128_transport_and_helper_nodes_print_from_structured_fields();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_f128_memory_backed_transport_prints_through_reserved_scratch();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_large_f128_memory_backed_transport_materializes_frame_slot_addresses();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_f128_symbol_memory_backed_transport_materializes_symbol_address();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_f128_compare_and_cast_helpers_print_from_structured_records();
      result != 0) {
    return result;
  }
  if (const int result = selected_f128_records_reject_incomplete_structured_fields();
      result != 0) {
    return result;
  }
  if (const int result = selected_i128_records_reject_incomplete_structured_fields();
      result != 0) {
    return result;
  }
  if (const int result = selected_immediate_return_node_prints_callable_epilogue();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_symbol_pointer_return_node_prints_address_materialization_before_ret();
      result != 0) {
    return result;
  }
  if (const int result = common_mir_printer_can_delegate_to_aarch64_target_spelling_adapter();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_add_with_immediate_operands_prints_structured_add();
      result != 0) {
    return result;
  }
  if (const int result = selected_scalar_add_sub_materializes_nonencodable_immediates();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_scalar_mul_div_rem_materializes_immediate_operands();
      result != 0) {
    return result;
  }
  if (const int result = selected_simple_integer_casts_reject_missing_or_unsupported_facts();
      result != 0) {
    return result;
  }
  if (const int result = selected_direct_call_prints_from_prepared_call_provenance();
      result != 0) {
    return result;
  }
  if (const int result = selected_memory_return_call_prints_call_and_preserves_storage_effect();
      result != 0) {
    return result;
  }
  if (const int result = selected_indirect_call_prints_from_prepared_register_callee();
      result != 0) {
    return result;
  }
  if (const int result = selected_call_boundary_register_move_prints_prepared_mov();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_call_boundary_scalar_fpr_argument_move_prints_prepared_fmov();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_call_boundary_immediate_argument_prints_prepared_mov();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_call_boundary_large_immediate_argument_materializes_legal_constant();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_call_boundary_frame_slot_source_materializes_large_offset();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_call_boundary_f128_frame_slot_argument_prints_q_load();
      result != 0) {
    return result;
  }
  if (const int result = selected_after_call_result_register_move_prints_prepared_mov();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_simple_frame_setup_and_teardown_print_from_prepared_frame_facts();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_large_frame_setup_and_teardown_materialize_adjustment();
      result != 0) {
    return result;
  }
  if (const int result = selected_non_leaf_frame_prints_link_register_save_restore();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_large_frame_link_register_slot_materializes_address();
      result != 0) {
    return result;
  }
  if (const int result = selected_direct_address_materialization_prints_page_low12_sequence();
      result != 0) {
    return result;
  }
  if (const int result = selected_string_address_materialization_prints_page_low12_sequence();
      result != 0) {
    return result;
  }
  if (const int result = remaining_address_materialization_printer_paths_use_structured_facts();
      result != 0) {
    return result;
  }
  if (const int result = selected_inline_asm_template_prints_from_structured_operands();
      result != 0) {
    return result;
  }
  if (const int result =
          selected_inline_asm_template_rejects_incomplete_or_unsupported_records();
      result != 0) {
    return result;
  }
  if (const int result = unsupported_surfaces_statuses_and_missing_operands_fail_closed();
      result != 0) {
    return result;
  }
  return 0;
}
