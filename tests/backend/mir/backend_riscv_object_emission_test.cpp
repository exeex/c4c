#include "src/backend/mir/object/model.hpp"
#include "src/backend/mir/riscv/codegen/object_emission.hpp"
#include "src/backend/mir/riscv/codegen/rv64_line_assembler.hpp"
#include "src/backend/prealloc/control_flow.hpp"
#include "src/backend/prealloc/module.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

namespace bir = c4c::backend::bir;
namespace object = c4c::backend::mir::object;
namespace prepare = c4c::backend::prepare;
namespace rv64 = c4c::backend::riscv::codegen;

constexpr std::uint32_t SHT_RELA = 4;
constexpr std::uint32_t SHT_SYMTAB = 2;
constexpr std::uint32_t R_RISCV_CALL_PLT = 19;
constexpr std::uint32_t R_RISCV_PCREL_HI20 = 23;
constexpr std::uint32_t R_RISCV_PCREL_LO12_I = 24;
constexpr std::uint16_t SHN_UNDEF = 0;

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  std::uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    value |= static_cast<std::uint64_t>(bytes[offset + shift / 8]) << shift;
  }
  return value;
}

std::string read_c_string(const std::vector<std::uint8_t>& bytes,
                          std::size_t offset) {
  std::string value;
  while (offset < bytes.size() && bytes[offset] != 0) {
    value.push_back(static_cast<char>(bytes[offset]));
    ++offset;
  }
  return value;
}

std::string shell_quote(const std::filesystem::path& path) {
  std::string quoted = "'";
  for (const char ch : path.string()) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(ch);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

int fail(const std::string& message) {
  std::cerr << message << "\n";
  return 1;
}

std::optional<object::ObjectModule> make_minimal_call_module() {
  return rv64::build_rv64_text_object_module({
      rv64::RiscvObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {
              rv64::make_rv64_direct_call_fragment("callee"),
              rv64::make_rv64_return_zero_fragment(),
          },
      },
  });
}

std::optional<object::ObjectModule> make_minimal_pcrel_module() {
  return rv64::build_rv64_text_object_module({
      rv64::RiscvObjectFunction{
          .name = "load_addr",
          .global = true,
          .fragments = {
              rv64::make_rv64_pcrel_address_fragment("target",
                                                     ".Lpcrel_hi_load_addr_0"),
              rv64::make_rv64_return_zero_fragment(),
          },
      },
  });
}

bir::Function make_prepared_return_zero_function(std::string name) {
  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  return bir::Function{
      .name = std::move(name),
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  };
}

prepare::PreparedBirModule make_prepared_direct_call_module() {
  prepare::PreparedBirModule prepared;
  const auto caller_name = prepared.names.function_names.intern("caller");
  const auto callee_name = prepared.names.function_names.intern("callee");

  bir::CallInst call;
  call.return_type = bir::TypeKind::Void;
  bir::Block caller_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  caller_entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "caller",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(caller_entry)},
  });
  prepared.module.functions.push_back(make_prepared_return_zero_function("callee"));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = caller_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = callee_name,
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = caller_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"callee"},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_rematerialized_return_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto t0_name = prepared.names.value_names.intern("%t0");
  const auto t1_name = prepared.names.value_names.intern("%t1");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::immediate_i32(2),
                  .rhs = bir::Value::immediate_i32(3),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Sub,
                  .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .rhs = bir::Value::immediate_i32(1),
              },
          },
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%t1");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = t0_name,
                  .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
                  .immediate_i32 = 5,
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = t1_name,
                  .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
                  .immediate_i32 = 4,
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_same_module_call_module() {
  prepare::PreparedBirModule prepared;
  const auto callee_name = prepared.names.function_names.intern("add_three");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto param_name = prepared.names.value_names.intern("%p.x");
  const auto callee_result_name = prepared.names.value_names.intern("%t0");
  const auto main_result_name = prepared.names.value_names.intern("%main.t0");

  bir::Block callee_entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%p.x"),
                  .rhs = bir::Value::immediate_i32(3),
              },
          },
      .terminator = bir::Terminator{},
  };
  callee_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%t0");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%main.t0");
  call.callee = "add_three";
  call.args = {bir::Value::immediate_i32(2)};
  call.arg_types = {bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  bir::Block main_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%main.t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "add_three",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .params = {bir::Param{
          .type = bir::TypeKind::I32,
          .name = "%p.x",
          .size_bytes = 4,
          .align_bytes = 4,
      }},
      .blocks = {std::move(callee_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(main_entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = callee_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = main_name,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = callee_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = callee_name,
                  .value_name = param_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = callee_name,
                  .value_name = callee_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = main_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = main_name,
                  .value_name = main_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"add_three"},
          .arguments = {prepare::PreparedCallArgumentPlan{
              .instruction_index = 0,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
              .source_literal = bir::Value::immediate_i32(2),
              .destination_register_name = std::string{"a0"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          }},
          .result = prepare::PreparedCallResultPlan{
              .instruction_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_value_id = 3,
              .source_register_name = std::string{"a0"},
              .source_contiguous_width = 1,
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_name = std::string{"t0"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_two_arg_scalar_call_module() {
  prepare::PreparedBirModule prepared;
  const auto callee_name = prepared.names.function_names.intern("add_pair");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto param_x_name = prepared.names.value_names.intern("%p.x");
  const auto param_y_name = prepared.names.value_names.intern("%p.y");
  const auto callee_result_name = prepared.names.value_names.intern("%t0");
  const auto main_result_name = prepared.names.value_names.intern("%main.t0");

  bir::Block callee_entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%p.x"),
                  .rhs = bir::Value::named(bir::TypeKind::I32, "%p.y"),
              },
          },
      .terminator = bir::Terminator{},
  };
  callee_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%t0");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%main.t0");
  call.callee = "add_pair";
  call.args = {bir::Value::immediate_i32(5), bir::Value::immediate_i32(7)};
  call.arg_types = {bir::TypeKind::I32, bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  bir::Block main_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%main.t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "add_pair",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .params =
          {
              bir::Param{
                  .type = bir::TypeKind::I32,
                  .name = "%p.x",
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
              bir::Param{
                  .type = bir::TypeKind::I32,
                  .name = "%p.y",
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
          },
      .blocks = {std::move(callee_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(main_entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = callee_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = main_name,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = callee_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = callee_name,
                  .value_name = param_x_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = callee_name,
                  .value_name = param_y_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a1"},
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = callee_name,
                  .value_name = callee_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = main_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 4,
                  .function_name = main_name,
                  .value_name = main_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"add_pair"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Immediate,
                      .source_literal = bir::Value::immediate_i32(5),
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Immediate,
                      .source_literal = bir::Value::immediate_i32(7),
                      .destination_register_name = std::string{"a1"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  },
              },
          .result = prepare::PreparedCallResultPlan{
              .instruction_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_value_id = 4,
              .source_register_name = std::string{"a0"},
              .source_contiguous_width = 1,
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_name = std::string{"t0"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_local_frame_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.x");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.x",
                  .slot_id = slot_name,
                  .value = bir::Value::immediate_i32(5),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .slot_name = "%lv.x",
                  .slot_id = slot_name,
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .local_slots = {bir::LocalSlot{
          .name = "%lv.x",
          .slot_id = slot_name,
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 4,
      .frame_alignment_bytes = 4,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{0},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .result_value_name = result_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{0},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
  });
  return prepared;
}

bir::InlineAsmOperandMetadata inline_asm_register_operand(
    bir::InlineAsmOperandKind kind,
    std::size_t constraint_index,
    std::string constraint,
    std::optional<std::size_t> arg_index,
    std::optional<std::size_t> output_index = std::nullopt,
    std::optional<std::size_t> tied_output_index = std::nullopt) {
  return bir::InlineAsmOperandMetadata{
      .kind = kind,
      .constraint_index = constraint_index,
      .constraint = std::move(constraint),
      .arg_index = arg_index,
      .output_index = output_index,
      .tied_output_index = tied_output_index,
  };
}

prepare::PreparedValueHome rv64_gpr_home(prepare::PreparedValueId value_id,
                                         c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         std::string register_name,
                                         std::size_t physical_index) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::move(register_name),
      .target_register_identity = prepare::PreparedTargetRegisterIdentity{
          .target_arch = c4c::TargetArch::Riscv64,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_class = prepare::PreparedRegisterClass::General,
          .physical_index = physical_index,
      },
  };
}

prepare::PreparedValueHome rv64_vector_home(prepare::PreparedValueId value_id,
                                            c4c::FunctionNameId function_name,
                                            c4c::ValueNameId value_name,
                                            std::string register_name,
                                            std::size_t physical_index) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::move(register_name),
      .target_register_identity = prepare::PreparedTargetRegisterIdentity{
          .target_arch = c4c::TargetArch::Riscv64,
          .bank = prepare::PreparedRegisterBank::Vreg,
          .register_class = prepare::PreparedRegisterClass::Vector,
          .physical_index = physical_index,
      },
  };
}

prepare::PreparedInlineAsmCarrier make_prepared_insn_d_carrier(
    std::string asm_text = ".insn.d %4, %5, %0, %1, %2, %3, %6") {
  prepare::PreparedInlineAsmCarrier carrier{
      .carrier_kind = prepare::PreparedInlineAsmCarrierKind::Complete,
      .asm_text = std::move(asm_text),
      .constraints = "=VRM2,VRM2,VRM2,VRM2,i,i,i",
      .side_effects = true,
      .operands =
          {
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterOutput,
                  .constraint_index = 0,
                  .constraint = "=VRM2",
                  .output_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 2,
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 1,
                  .constraint = "VRM2",
                  .arg_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 2,
                  .home = rv64_vector_home(2, {}, {}, "v4", 4),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 2,
                  .constraint = "VRM2",
                  .arg_index = std::size_t{1},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 2,
                  .home = rv64_vector_home(3, {}, {}, "v6", 6),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 3,
                  .constraint = "VRM2",
                  .arg_index = std::size_t{2},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 2,
                  .home = rv64_vector_home(4, {}, {}, "v8", 8),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::IntegerImmediateInput,
                  .constraint_index = 4,
                  .constraint = "i",
                  .arg_index = std::size_t{3},
                  .immediate_value = std::int64_t{0x0a},
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::IntegerImmediateInput,
                  .constraint_index = 5,
                  .constraint = "i",
                  .arg_index = std::size_t{4},
                  .immediate_value = std::int64_t{0x0b},
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::IntegerImmediateInput,
                  .constraint_index = 6,
                  .constraint = "i",
                  .arg_index = std::size_t{5},
                  .immediate_value = std::int64_t{0x03},
              },
          },
      .result_home = rv64_vector_home(1, {}, {}, "v20", 20),
  };
  return carrier;
}

std::string helper_style_rv64_insn_d_template_text() {
  return std::string(".insn.d ") + "%4, %5, " + "%0, %1, " + "%2, %3, %6";
}

prepare::PreparedBirModule make_prepared_inline_asm_insn_r_module(
    std::string asm_text = ".insn r 0x33, 0, 0, %0, %1, %2",
    bool complete_carrier = true,
    bool tied_first_input = false,
    bool structured_metadata = false) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile.arch = c4c::TargetArch::Riscv64;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto result_name = prepared.names.value_names.intern("%sum");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%sum");
  call.callee = "llvm.inline_asm";
  call.args = {bir::Value::named(bir::TypeKind::I32, "%lhs"),
               bir::Value::named(bir::TypeKind::I32, "%rhs")};
  call.arg_types = {bir::TypeKind::I32, bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  call.inline_asm = bir::InlineAsmMetadata{
      .asm_text = std::move(asm_text),
      .constraints = tied_first_input ? "=r,0,r" : "=r,r,r",
      .side_effects = true,
      .operands =
          {
              inline_asm_register_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                          0,
                                          "=r",
                                          std::nullopt,
                                          std::size_t{0}),
              inline_asm_register_operand(tied_first_input
                                              ? bir::InlineAsmOperandKind::TiedInput
                                              : bir::InlineAsmOperandKind::RegisterInput,
                                          1,
                                          tied_first_input ? "0" : "r",
                                          std::size_t{0},
                                          std::nullopt,
                                          tied_first_input ? std::optional<std::size_t>{0}
                                                           : std::nullopt),
              inline_asm_register_operand(bir::InlineAsmOperandKind::RegisterInput,
                                          2,
                                          "r",
                                          std::size_t{1}),
          },
  };
  if (structured_metadata) {
    call.inline_asm->insn_r = bir::InlineAsmInsnRMetadata{
        .opcode = 0x33,
        .funct3 = 0,
        .funct7 = 0,
        .operand_indices = {0, 1, 2},
    };
  }

  bir::Block entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, result_name, "t0", 5),
              rv64_gpr_home(2,
                            function_name,
                            lhs_name,
                            tied_first_input ? "t0" : "t1",
                            tied_first_input ? 5 : 6),
              rv64_gpr_home(3, function_name, rhs_name, "t2", 7),
          },
  });
  if (complete_carrier) {
    prepared.inline_asm_carriers.functions.push_back(
        prepare::PreparedInlineAsmCarrierFunction{
            .function_name = function_name,
            .carriers =
                {
                    prepare::PreparedInlineAsmCarrier{
                        .function_name = function_name,
                        .carrier_kind = prepare::PreparedInlineAsmCarrierKind::Complete,
                        .block_index = 0,
                        .inst_index = 0,
                        .asm_text = ".insn r 0x33, 0, 0, %0, %1, %2",
                        .constraints = tied_first_input ? "=r,0,r" : "=r,r,r",
                        .side_effects = true,
                        .operands =
                            {
                                prepare::PreparedInlineAsmOperand{
                                    .kind = bir::InlineAsmOperandKind::RegisterOutput,
                                    .constraint_index = 0,
                                    .constraint = "=r",
                                    .output_index = std::size_t{0},
                                },
                                prepare::PreparedInlineAsmOperand{
                                    .kind = tied_first_input
                                                ? bir::InlineAsmOperandKind::TiedInput
                                                : bir::InlineAsmOperandKind::RegisterInput,
                                    .constraint_index = 1,
                                    .constraint = tied_first_input ? "0" : "r",
                                    .arg_index = std::size_t{0},
                                    .tied_output_index =
                                        tied_first_input
                                            ? std::optional<std::size_t>{0}
                                            : std::nullopt,
                                    .value = bir::Value::named(bir::TypeKind::I32, "%lhs"),
                                    .value_name = lhs_name,
                                    .home = rv64_gpr_home(2,
                                                          function_name,
                                                          lhs_name,
                                                          tied_first_input ? "t0" : "t1",
                                                          tied_first_input ? 5 : 6),
                                    .tied_home_authority =
                                        tied_first_input
                                            ? std::optional<
                                                  prepare::PreparedInlineAsmTiedHomeAuthority>{
                                                  prepare::PreparedInlineAsmTiedHomeAuthority{
                                                      .tied_output_index = 0,
                                                      .shared_register =
                                                          prepare::
                                                              PreparedTargetRegisterIdentity{
                                                                  .target_arch =
                                                                      c4c::TargetArch::
                                                                          Riscv64,
                                                                  .bank =
                                                                      prepare::
                                                                          PreparedRegisterBank::
                                                                              Gpr,
                                                                  .register_class =
                                                                      prepare::
                                                                          PreparedRegisterClass::
                                                                              General,
                                                                  .physical_index = 5,
                                                              },
                                                  }}
                                            : std::nullopt,
                                },
                                prepare::PreparedInlineAsmOperand{
                                    .kind = bir::InlineAsmOperandKind::RegisterInput,
                                    .constraint_index = 2,
                                    .constraint = "r",
                                    .arg_index = std::size_t{1},
                                    .value = bir::Value::named(bir::TypeKind::I32, "%rhs"),
                                    .value_name = rhs_name,
                                    .home = rv64_gpr_home(3,
                                                          function_name,
                                                          rhs_name,
                                                          "t2",
                                                          7),
                                },
                            },
                        .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
                        .result_value_name = result_name,
                        .result_home = rv64_gpr_home(1,
                                                     function_name,
                                                     result_name,
                                                     "t0",
                                                     5),
                    },
                },
        });
  }
  return prepared;
}

prepare::PreparedBirModule make_prepared_inline_asm_insn_r_readwrite_module(
    std::string asm_text = ".insn r 0x33, 0, 0, %0, %0, %1",
    bool structured_metadata = true) {
  auto prepared = make_prepared_inline_asm_insn_r_module(std::move(asm_text));
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions[0].blocks[0].insts[0]);
  call.args = {bir::Value::named(bir::TypeKind::I32, "%lhs"),
               bir::Value::named(bir::TypeKind::I32, "%rhs")};
  call.arg_types = {bir::TypeKind::I32, bir::TypeKind::I32};
  call.inline_asm->constraints = "+r,r";
  call.inline_asm->operands =
      {
          inline_asm_register_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                      0,
                                      "+r",
                                      std::size_t{0},
                                      std::size_t{0}),
          inline_asm_register_operand(bir::InlineAsmOperandKind::RegisterInput,
                                      1,
                                      "r",
                                      std::size_t{1}),
      };
  if (structured_metadata) {
    call.inline_asm->insn_r = bir::InlineAsmInsnRMetadata{
        .opcode = 0x33,
        .funct3 = 0,
        .funct7 = 0,
        .operand_indices = {0, 0, 1},
    };
  } else {
    call.inline_asm->insn_r = std::nullopt;
  }

  auto& carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  carrier.asm_text = call.inline_asm->asm_text;
  carrier.constraints = "+r,r";
  carrier.operands =
      {
          prepare::PreparedInlineAsmOperand{
              .kind = bir::InlineAsmOperandKind::RegisterOutput,
              .constraint_index = 0,
              .constraint = "+r",
              .arg_index = std::size_t{0},
              .output_index = std::size_t{0},
              .value = bir::Value::named(bir::TypeKind::I32, "%lhs"),
              .value_name = prepared.names.value_names.find("%lhs"),
              .home = rv64_gpr_home(2,
                                    prepared.names.function_names.find("main"),
                                    prepared.names.value_names.find("%lhs"),
                                    "t0",
                                    5),
          },
          prepare::PreparedInlineAsmOperand{
              .kind = bir::InlineAsmOperandKind::RegisterInput,
              .constraint_index = 1,
              .constraint = "r",
              .arg_index = std::size_t{1},
              .value = bir::Value::named(bir::TypeKind::I32, "%rhs"),
              .value_name = prepared.names.value_names.find("%rhs"),
              .home = rv64_gpr_home(3,
                                    prepared.names.function_names.find("main"),
                                    prepared.names.value_names.find("%rhs"),
                                    "t2",
                                    7),
          },
      };
  return prepared;
}

prepare::PreparedBirModule make_prepared_inline_asm_insn_d_module(
    prepare::PreparedInlineAsmCarrier carrier = make_prepared_insn_d_carrier()) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile.arch = c4c::TargetArch::Riscv64;
  const auto function_name = prepared.names.function_names.intern("main");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%vd");
  call.callee = "llvm.inline_asm";
  call.args = {
      bir::Value::named(bir::TypeKind::I32, "%a"),
      bir::Value::named(bir::TypeKind::I32, "%b"),
      bir::Value::named(bir::TypeKind::I32, "%c"),
      bir::Value::immediate_i32(static_cast<std::int32_t>(carrier.operands[4]
                                                              .immediate_value
                                                              .value_or(0))),
      bir::Value::immediate_i32(static_cast<std::int32_t>(carrier.operands[5]
                                                              .immediate_value
                                                              .value_or(0))),
      bir::Value::immediate_i32(static_cast<std::int32_t>(carrier.operands[6]
                                                              .immediate_value
                                                              .value_or(0))),
  };
  call.arg_types = {bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  call.inline_asm = bir::InlineAsmMetadata{
      .asm_text = carrier.asm_text,
      .constraints = carrier.constraints,
      .side_effects = true,
  };

  bir::Block entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
  });

  carrier.function_name = function_name;
  carrier.block_index = 0;
  carrier.inst_index = 0;
  prepared.inline_asm_carriers.functions.push_back(
      prepare::PreparedInlineAsmCarrierFunction{
          .function_name = function_name,
          .carriers = {std::move(carrier)},
      });
  return prepared;
}

prepare::PreparedBirModule make_prepared_mixed_inline_asm_insn_module() {
  auto prepared = make_prepared_inline_asm_insn_r_module();
  auto carrier = make_prepared_insn_d_carrier();

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%vd");
  call.callee = "llvm.inline_asm";
  call.args = {
      bir::Value::named(bir::TypeKind::I32, "%a"),
      bir::Value::named(bir::TypeKind::I32, "%b"),
      bir::Value::named(bir::TypeKind::I32, "%c"),
      bir::Value::immediate_i32(static_cast<std::int32_t>(carrier.operands[4]
                                                              .immediate_value
                                                              .value_or(0))),
      bir::Value::immediate_i32(static_cast<std::int32_t>(carrier.operands[5]
                                                              .immediate_value
                                                              .value_or(0))),
      bir::Value::immediate_i32(static_cast<std::int32_t>(carrier.operands[6]
                                                              .immediate_value
                                                              .value_or(0))),
  };
  call.arg_types = {bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32,
                    bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  call.inline_asm = bir::InlineAsmMetadata{
      .asm_text = carrier.asm_text,
      .constraints = carrier.constraints,
      .side_effects = true,
  };

  auto& function = prepared.module.functions.front();
  function.blocks.front().insts.push_back(call);

  const auto function_name = prepared.control_flow.functions.front().function_name;
  carrier.function_name = function_name;
  carrier.block_index = 0;
  carrier.inst_index = 1;
  prepared.inline_asm_carriers.functions.front().carriers.push_back(
      std::move(carrier));
  return prepared;
}

prepare::PreparedBirModule make_prepared_local_register_arg_call_module() {
  prepare::PreparedBirModule prepared;
  const auto callee_name = prepared.names.function_names.intern("add_pair");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto x_slot_name = prepared.names.slot_names.intern("%lv.x");
  const auto y_slot_name = prepared.names.slot_names.intern("%lv.y");
  const auto param_x_name = prepared.names.value_names.intern("%p.x");
  const auto param_y_name = prepared.names.value_names.intern("%p.y");
  const auto callee_result_name = prepared.names.value_names.intern("%t0");
  const auto main_x_name = prepared.names.value_names.intern("%main.x");
  const auto main_y_name = prepared.names.value_names.intern("%main.y");
  const auto main_result_name = prepared.names.value_names.intern("%main.result");

  bir::Block callee_entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%p.x"),
                  .rhs = bir::Value::named(bir::TypeKind::I32, "%p.y"),
              },
          },
      .terminator = bir::Terminator{},
  };
  callee_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%t0");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%main.result");
  call.callee = "add_pair";
  call.args = {bir::Value::named(bir::TypeKind::I32, "%main.x"),
               bir::Value::named(bir::TypeKind::I32, "%main.y")};
  call.arg_types = {bir::TypeKind::I32, bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  bir::Block main_entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.x",
                  .slot_id = x_slot_name,
                  .value = bir::Value::immediate_i32(5),
                  .align_bytes = 4,
              },
              bir::StoreLocalInst{
                  .slot_name = "%lv.y",
                  .slot_id = y_slot_name,
                  .value = bir::Value::immediate_i32(7),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%main.x"),
                  .slot_name = "%lv.x",
                  .slot_id = x_slot_name,
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%main.y"),
                  .slot_name = "%lv.y",
                  .slot_id = y_slot_name,
                  .align_bytes = 4,
              },
              call,
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%main.result");

  prepared.module.functions.push_back(bir::Function{
      .name = "add_pair",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .params =
          {
              bir::Param{
                  .type = bir::TypeKind::I32,
                  .name = "%p.x",
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
              bir::Param{
                  .type = bir::TypeKind::I32,
                  .name = "%p.y",
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
          },
      .blocks = {std::move(callee_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .local_slots =
          {
              bir::LocalSlot{
                  .name = "%lv.x",
                  .slot_id = x_slot_name,
                  .type = bir::TypeKind::I32,
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
              bir::LocalSlot{
                  .name = "%lv.y",
                  .slot_id = y_slot_name,
                  .type = bir::TypeKind::I32,
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
          },
      .blocks = {std::move(main_entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = callee_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = main_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = callee_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = callee_name,
                  .value_name = param_x_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = callee_name,
                  .value_name = param_y_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a1"},
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = callee_name,
                  .value_name = callee_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = main_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 4,
                  .function_name = main_name,
                  .value_name = main_x_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 5,
                  .function_name = main_name,
                  .value_name = main_y_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
              },
              prepare::PreparedValueHome{
                  .value_id = 6,
                  .function_name = main_name,
                  .value_name = main_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s2"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 4,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"add_pair"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 4,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{4},
                      .source_register_name = std::string{"t0"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 4,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{5},
                      .source_register_name = std::string{"s1"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a1"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                  },
              },
          .result = prepare::PreparedCallResultPlan{
              .instruction_index = 4,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_value_id = 6,
              .source_register_name = std::string{"a0"},
              .source_contiguous_width = 1,
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_name = std::string{"s2"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          },
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = main_name,
      .frame_size_bytes = 8,
      .frame_alignment_bytes = 4,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{0},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{1},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 2,
                  .result_value_name = main_x_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{0},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 3,
                  .result_value_name = main_y_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{1},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
  });
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .offset_bytes = 0,
          .size_bytes = 4,
          .align_bytes = 4,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{1},
          .offset_bytes = 4,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
  return prepared;
}

int records_minimal_text_and_call_relocation() {
  const auto module = make_minimal_call_module();
  if (!module.has_value()) {
    return fail("expected RV64 object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() != 16 || text->size_bytes != 16 ||
      !text->executable || text->writable) {
    return fail("expected executable .text section with four RV64 words");
  }
  if (text->bytes[0] != 0x97 || text->bytes[1] != 0x00 ||
      text->bytes[2] != 0x00 || text->bytes[3] != 0x00) {
    return fail("expected direct call fragment to start with auipc ra, 0");
  }
  if (text->bytes[4] != 0xe7 || text->bytes[5] != 0x80 ||
      text->bytes[6] != 0x00 || text->bytes[7] != 0x00) {
    return fail("expected direct call fragment to include jalr ra, 0(ra)");
  }

  const auto* caller = object::find_symbol(*module, "caller");
  const auto* callee = object::find_symbol(*module, "callee");
  if (caller == nullptr || caller->binding != object::SymbolBinding::Global ||
      caller->kind != object::SymbolKind::Function ||
      caller->section != std::optional<object::SectionId>{text->id} ||
      caller->value != 0 || caller->size_bytes != 16) {
    return fail("expected defined global caller function symbol");
  }
  if (callee == nullptr || callee->binding != object::SymbolBinding::Global ||
      callee->kind != object::SymbolKind::Function ||
      !object::is_undefined_symbol(*callee)) {
    return fail("expected undefined global callee function symbol");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != 19 ||
      module->relocations[0].symbol != callee->id ||
      module->relocations[0].addend != 0) {
    return fail("expected R_RISCV_CALL_PLT relocation at the call pair");
  }
  return 0;
}

int records_same_module_direct_call_symbol() {
  const auto module = rv64::build_rv64_text_object_module({
      rv64::RiscvObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {
              rv64::make_rv64_direct_call_fragment("callee"),
              rv64::make_rv64_return_zero_fragment(),
          },
      },
      rv64::RiscvObjectFunction{
          .name = "callee",
          .global = true,
          .fragments = {
              rv64::make_rv64_return_zero_fragment(),
          },
      },
  });
  if (!module.has_value()) {
    return fail("expected same-module RV64 object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* callee = object::find_symbol(*module, "callee");
  if (text == nullptr || callee == nullptr ||
      callee->section != std::optional<object::SectionId>{text->id} ||
      callee->value != 16 || callee->size_bytes != 8 ||
      object::is_undefined_symbol(*callee)) {
    return fail("expected same-module callee to resolve as a defined function");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].symbol != callee->id ||
      module->relocations[0].type != 19) {
    return fail("expected same-module direct call to target the defined callee symbol");
  }
  return 0;
}

int records_pcrel_hi_lo_pairing_with_auipc_site_label() {
  const auto module = make_minimal_pcrel_module();
  if (!module.has_value()) {
    return fail("expected RV64 pcrel object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() != 16 || text->size_bytes != 16) {
    return fail("expected pcrel fixture to produce two address words plus return");
  }
  if (text->bytes[0] != 0x97 || text->bytes[1] != 0x02 ||
      text->bytes[2] != 0x00 || text->bytes[3] != 0x00) {
    return fail("expected pcrel fixture to start with auipc t0, 0");
  }
  if (text->bytes[4] != 0x93 || text->bytes[5] != 0x82 ||
      text->bytes[6] != 0x02 || text->bytes[7] != 0x00) {
    return fail("expected pcrel fixture to include addi t0, t0, 0");
  }

  const auto* target = object::find_symbol(*module, "target");
  const auto* auipc_label = object::find_symbol(*module, ".Lpcrel_hi_load_addr_0");
  const auto* function = object::find_symbol(*module, "load_addr");
  if (target == nullptr || target->binding != object::SymbolBinding::Global ||
      target->kind != object::SymbolKind::Function ||
      !object::is_undefined_symbol(*target)) {
    return fail("expected pcrel high relocation target to be an undefined symbol");
  }
  if (auipc_label == nullptr ||
      auipc_label->binding != object::SymbolBinding::Local ||
      auipc_label->kind != object::SymbolKind::NoType ||
      auipc_label->section != std::optional<object::SectionId>{text->id} ||
      auipc_label->value != 0 || auipc_label->size_bytes != 0) {
    return fail("expected pcrel low relocation target to be a local AUIPC label");
  }
  if (function == nullptr ||
      function->section != std::optional<object::SectionId>{text->id} ||
      function->value != 0 || function->size_bytes != 16) {
    return fail("expected pcrel fixture function symbol to cover the fragment");
  }
  if (module->labels.size() != 1 ||
      module->labels[0].name != ".Lpcrel_hi_load_addr_0" ||
      module->labels[0].section != text->id || module->labels[0].offset != 0) {
    return fail("expected pcrel fixture to record the AUIPC-site label");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != target->id ||
      module->relocations[0].addend != 0 ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id ||
      module->relocations[1].addend != 0) {
    return fail("expected paired R_RISCV_PCREL_HI20/LO12_I relocations");
  }
  return 0;
}

int builds_prepared_text_object_module_without_call_text() {
  const auto prepared = make_prepared_direct_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* caller = object::find_symbol(*module, "caller");
  const auto* callee = object::find_symbol(*module, "callee");
  if (text == nullptr || caller == nullptr || callee == nullptr) {
    return fail("expected prepared object module to publish .text and function symbols");
  }
  if (text->bytes.size() != 40 || text->size_bytes != 40) {
    return fail("expected prepared caller and callee text fragments");
  }
  if (caller->section != std::optional<object::SectionId>{text->id} ||
      caller->value != 0 || caller->size_bytes != 32 ||
      callee->section != std::optional<object::SectionId>{text->id} ||
      callee->value != 32 || callee->size_bytes != 8) {
    return fail("expected prepared function symbols to use shared object helpers");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 8 ||
      module->relocations[0].type != 19 ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected prepared direct call to lower through R_RISCV_CALL_PLT");
  }
  return 0;
}

int builds_prepared_rematerialized_nonzero_return_object() {
  const auto prepared = make_prepared_rematerialized_return_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared immediate-return RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared immediate-return object to publish text/main");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 8 ||
      main_symbol->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared immediate-return object to contain one RV64 return fragment");
  }
  if (text->bytes[0] != 0x13 || text->bytes[1] != 0x05 ||
      text->bytes[2] != 0x40 || text->bytes[3] != 0x00 ||
      text->bytes[4] != 0x67 || text->bytes[5] != 0x80 ||
      text->bytes[6] != 0x00 || text->bytes[7] != 0x00) {
    return fail("expected addi a0, zero, 4 followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared immediate-return object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_same_module_call_object() {
  const auto prepared = make_prepared_scalar_same_module_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* callee = object::find_symbol(*module, "add_three");
  const auto* main = object::find_symbol(*module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected prepared scalar call object to publish text/functions");
  }
  if (text->bytes.size() != 52 || text->size_bytes != 52 ||
      callee->value != 0 || callee->size_bytes != 12 ||
      main->value != 12 || main->size_bytes != 40) {
    return fail("expected prepared scalar call object text layout");
  }
  if (text->bytes[0] != 0x93 || text->bytes[1] != 0x02 ||
      text->bytes[2] != 0x35 || text->bytes[3] != 0x00 ||
      text->bytes[4] != 0x13 || text->bytes[5] != 0x85 ||
      text->bytes[6] != 0x02 || text->bytes[7] != 0x00) {
    return fail("expected add_three to add immediate param and move return to a0");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 24 ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected scalar same-module call relocation at call pair");
  }
  return 0;
}

int builds_prepared_two_arg_scalar_call_object() {
  const auto prepared = make_prepared_two_arg_scalar_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared two-arg scalar call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* callee = object::find_symbol(*module, "add_pair");
  const auto* main = object::find_symbol(*module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected prepared two-arg object to publish text/functions");
  }
  if (text->bytes.size() != 56 || text->size_bytes != 56 ||
      callee->value != 0 || callee->size_bytes != 12 ||
      main->value != 12 || main->size_bytes != 44) {
    return fail("expected prepared two-arg object text layout");
  }
  if (text->bytes[0] != 0xb3 || text->bytes[1] != 0x02 ||
      text->bytes[2] != 0xb5 || text->bytes[3] != 0x00) {
    return fail("expected add_pair to use RV64 register-register add");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 28 ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected two-arg same-module call relocation at call pair");
  }
  return 0;
}

int builds_prepared_scalar_local_frame_object() {
  const auto prepared = make_prepared_scalar_local_frame_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar local RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar local object to publish text/main");
  }
  if (text->bytes.size() != 28 || text->size_bytes != 28 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 28) {
    return fail("expected prepared scalar local object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00500313 ||
      read_u32(text->bytes, 8) != 0x00612023 ||
      read_u32(text->bytes, 12) != 0x00012283 ||
      read_u32(text->bytes, 16) != 0x00028513 ||
      read_u32(text->bytes, 20) != 0x01010113 ||
      read_u32(text->bytes, 24) != 0x00008067) {
    return fail("expected stack-frame sw/lw scalar local object sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar local object to need no relocations");
  }
  return 0;
}

int builds_prepared_local_register_arg_call_object() {
  const auto prepared = make_prepared_local_register_arg_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared local/register-arg call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* callee = object::find_symbol(*module, "add_pair");
  const auto* main = object::find_symbol(*module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected local/register-arg call object to publish text/functions");
  }
  if (text->bytes.size() != 80 || text->size_bytes != 80 ||
      callee->value != 0 || callee->size_bytes != 12 ||
      main->value != 12 || main->size_bytes != 68) {
    return fail("expected local/register-arg call object text layout");
  }
  const std::size_t main_offset = main->value;
  if (read_u32(text->bytes, main_offset + 0) != 0xfe010113 ||
      read_u32(text->bytes, main_offset + 4) != 0x00113c23 ||
      read_u32(text->bytes, main_offset + 24) != 0x00012283 ||
      read_u32(text->bytes, main_offset + 28) != 0x00412483 ||
      read_u32(text->bytes, main_offset + 32) != 0x00028513 ||
      read_u32(text->bytes, main_offset + 36) != 0x00048593 ||
      read_u32(text->bytes, main_offset + 56) != 0x01813083 ||
      read_u32(text->bytes, main_offset + 60) != 0x02010113) {
    return fail("expected combined call/local frame and register argument moves");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != main_offset + 40 ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected local/register-arg same-module call relocation");
  }
  return 0;
}

int builds_prepared_inline_asm_insn_r_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 inline-asm .insn r object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected inline-asm .insn r object to publish text/main");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 12) {
    return fail("expected inline-asm .insn r object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x007302b3 ||
      read_u32(text->bytes, 4) != 0x00028513 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected .insn r add t0, t1, t2 followed by return move");
  }
  if (!module->relocations.empty()) {
    return fail("expected inline-asm .insn r object to need no relocations");
  }
  return 0;
}

int builds_structured_prepared_inline_asm_insn_r_object_without_text_reparse() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x80, 7, 127, raw, tokens, ignored",
      true,
      false,
      true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected structured RV64 inline-asm .insn r object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() != 12) {
    return fail("expected structured inline-asm .insn r object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x007302b3 ||
      read_u32(text->bytes, 4) != 0x00028513 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected structured .insn r metadata to encode add t0, t1, t2");
  }
  return 0;
}

int builds_prepared_inline_asm_insn_r_tied_input_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, %1, %2",
      true,
      true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 tied inline-asm .insn r object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() != 12) {
    return fail("expected tied inline-asm .insn r object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x007282b3 ||
      read_u32(text->bytes, 4) != 0x00028513 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected tied .insn r add t0, t0, t2 followed by return move");
  }
  return 0;
}

int builds_structured_prepared_inline_asm_insn_r_readwrite_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_readwrite_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected structured RV64 read-write .insn r object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() != 12) {
    return fail("expected structured read-write .insn r object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x007282b3 ||
      read_u32(text->bytes, 4) != 0x00028513 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected structured read-write .insn r add t0, t0, t2");
  }
  return 0;
}

int substitutes_prepared_rv64_vector_inline_asm_base_registers() {
  prepare::PreparedInlineAsmCarrier carrier{
      .carrier_kind = prepare::PreparedInlineAsmCarrierKind::Complete,
      .asm_text = "vcombo %0, %1, %2, %3, %4",
      .constraints = "VR,VRM1,VRM2,VRM4,VRM8",
      .side_effects = true,
      .operands =
          {
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 0,
                  .constraint = "VR",
                  .arg_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 1,
                  .home = rv64_vector_home(1, {}, {}, "v3", 3),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 1,
                  .constraint = "VRM1",
                  .arg_index = std::size_t{1},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 1,
                  .home = rv64_vector_home(2, {}, {}, "v5", 5),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 2,
                  .constraint = "VRM2",
                  .arg_index = std::size_t{2},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 2,
                  .home = rv64_vector_home(3, {}, {}, "v6", 6),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 3,
                  .constraint = "VRM4",
                  .arg_index = std::size_t{3},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 4,
                  .home = rv64_vector_home(4, {}, {}, "v8", 8),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 4,
                  .constraint = "VRM8",
                  .arg_index = std::size_t{4},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 8,
                  .home = rv64_vector_home(5, {}, {}, "v16", 16),
              },
          },
  };

  const auto substituted = rv64::substitute_prepared_riscv_inline_asm_operands(carrier);
  if (!substituted.has_value() || *substituted != "vcombo v3, v5, v6, v8, v16") {
    return fail("expected RV64 VR/VRM1/VRM2/VRM4/VRM8 substitution to print selected base vector registers");
  }
  return 0;
}

int substitutes_prepared_rv64_mixed_scalar_vector_inline_asm_registers() {
  prepare::PreparedInlineAsmCarrier carrier{
      .carrier_kind = prepare::PreparedInlineAsmCarrierKind::Complete,
      .asm_text = "mix %0, %1, %2",
      .constraints = "=r,VRM2,r",
      .side_effects = true,
      .operands =
          {
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterOutput,
                  .constraint_index = 0,
                  .constraint = "=r",
                  .output_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::General,
                  .register_group_width = 1,
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 1,
                  .constraint = "VRM2",
                  .arg_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 2,
                  .home = rv64_vector_home(2, {}, {}, "v12", 12),
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterInput,
                  .constraint_index = 2,
                  .constraint = "r",
                  .arg_index = std::size_t{1},
                  .register_class = bir::InlineAsmRegisterClass::General,
                  .register_group_width = 1,
                  .home = rv64_gpr_home(3, {}, {}, "t1", 6),
              },
          },
      .result_home = rv64_gpr_home(1, {}, {}, "t0", 5),
  };

  const auto substituted = rv64::substitute_prepared_riscv_inline_asm_operands(carrier);
  if (!substituted.has_value() || *substituted != "mix t0, v12, t1") {
    return fail("expected mixed RV64 scalar/vector inline asm substitution to preserve operand homes");
  }
  return 0;
}

int substitutes_prepared_rv64_tied_vector_inline_asm_base_register() {
  const auto shared_home = rv64_vector_home(1, {}, {}, "v24", 24);
  prepare::PreparedInlineAsmCarrier carrier{
      .carrier_kind = prepare::PreparedInlineAsmCarrierKind::Complete,
      .asm_text = "vtie %0, %1",
      .constraints = "=VRM8,0",
      .side_effects = true,
      .operands =
          {
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::RegisterOutput,
                  .constraint_index = 0,
                  .constraint = "=VRM8",
                  .output_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 8,
              },
              prepare::PreparedInlineAsmOperand{
                  .kind = bir::InlineAsmOperandKind::TiedInput,
                  .constraint_index = 1,
                  .constraint = "0",
                  .arg_index = std::size_t{0},
                  .tied_output_index = std::size_t{0},
                  .register_class = bir::InlineAsmRegisterClass::Vector,
                  .register_group_width = 8,
                  .home = shared_home,
              },
          },
      .result_home = shared_home,
  };

  const auto substituted = rv64::substitute_prepared_riscv_inline_asm_operands(carrier);
  if (!substituted.has_value() || *substituted != "vtie v24, v24") {
    return fail("expected tied RV64 VRM8 substitution to print the shared base vector register");
  }
  return 0;
}

int parses_rv64_line_core_canonical_subset() {
  const auto insn = rv64::parse_rv64_asm_line(
      ".insn.d 10, 11, v6, v0, v2, v4, 3");
  const auto* insn_d = insn.has_value()
                           ? std::get_if<rv64::Rv64InsnDLine>(&*insn)
                           : nullptr;
  if (insn_d == nullptr || insn_d->major != 10 || insn_d->operation != 11 ||
      insn_d->destination.bank != rv64::Rv64AsmRegisterBank::Vector ||
      insn_d->destination.physical_index != 6 ||
      insn_d->lhs.physical_index != 0 || insn_d->rhs.physical_index != 2 ||
      insn_d->accumulator.physical_index != 4 || insn_d->dtype != 3) {
    return fail("expected RV64 line parser to accept canonical .insn.d fields");
  }

  const auto li = rv64::parse_rv64_asm_line("li a0, 0");
  const auto* li_line = li.has_value() ? std::get_if<rv64::Rv64LiLine>(&*li)
                                       : nullptr;
  if (li_line == nullptr ||
      li_line->destination.bank != rv64::Rv64AsmRegisterBank::Gpr ||
      li_line->destination.physical_index != 10 || li_line->immediate != 0) {
    return fail("expected RV64 line parser to accept canonical li");
  }

  const auto ret = rv64::parse_rv64_asm_line("ret");
  if (!ret.has_value() || !std::holds_alternative<rv64::Rv64RetLine>(*ret)) {
    return fail("expected RV64 line parser to accept ret");
  }
  return 0;
}

int rejects_rv64_line_core_malformed_subset() {
  if (rv64::parse_rv64_asm_line(
          ".insn.d 10, 11, x6, v0, v2, v4, 3")
          .has_value()) {
    return fail("expected RV64 line parser to reject non-vector .insn.d register");
  }
  if (rv64::parse_rv64_asm_line(
          ".insn.d 10, 11, v6, v0, v2, v4")
          .has_value()) {
    return fail("expected RV64 line parser to reject missing .insn.d field");
  }
  if (rv64::parse_rv64_asm_line(
          ".insn.d 128, 11, v6, v0, v2, v4, 3")
          .has_value()) {
    return fail("expected RV64 line parser to reject out-of-range .insn.d namespace");
  }
  if (rv64::parse_rv64_asm_line("li v0, 0").has_value()) {
    return fail("expected RV64 line parser to reject vector destination for li");
  }
  if (rv64::parse_rv64_asm_line("li a0, 2048").has_value()) {
    return fail("expected RV64 line parser to reject out-of-range li immediate");
  }
  if (rv64::parse_rv64_asm_line("ret a0").has_value()) {
    return fail("expected RV64 line parser to reject malformed ret");
  }
  return 0;
}

int encodes_rv64_line_core_canonical_subset() {
  std::vector<std::uint8_t> bytes;
  for (const std::string_view line : {
           ".insn.d 10, 11, v6, v0, v2, v4, 3",
           "li a0, 0",
           "ret",
       }) {
    const auto parsed = rv64::parse_rv64_asm_line(line);
    if (!parsed.has_value()) {
      return fail("expected canonical RV64 line to parse before encode");
    }
    const auto encoded = rv64::encode_rv64_asm_line(*parsed);
    if (!encoded.has_value()) {
      return fail("expected canonical RV64 line to encode");
    }
    bytes.insert(bytes.end(), encoded->begin(), encoded->end());
  }
  if (bytes.size() != 16 || read_u64(bytes, 0) != 0x0000030b0820030aull ||
      read_u32(bytes, 8) != 0x00000513 ||
      read_u32(bytes, 12) != 0x00008067) {
    return fail("expected RV64 line encoder to preserve canonical object bytes");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_without_complete_carrier() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, %1, %2",
      false);
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to require complete carrier");
  }
  return 0;
}

int rejects_structured_prepared_inline_asm_insn_r_bad_operand_metadata_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, %1, %2",
      true,
      false,
      true);
  auto& call =
      std::get<bir::CallInst>(prepared.module.functions[0].blocks[0].insts[0]);
  call.inline_asm->insn_r->operand_indices = {0, 1, 99};
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected structured .insn r object path to reject bad operand metadata");
  }
  return 0;
}

int rejects_prepared_inline_asm_non_insn_r_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module("addi $0, $1, 0");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm object path to reject unsupported asm template");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_extra_field_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, %1, %2, %0");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject extra fields");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_missing_field_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, %1");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject missing fields");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_out_of_range_numeric_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x80, 0, 0, %0, %1, %2");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject invalid numeric fields");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_bad_operand_token_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, a0, %2");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject raw register tokens");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_named_operand_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %[dst], %1, %2");
  prepared.inline_asm_carriers.functions[0].carriers[0].has_named_operand_references =
      true;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject named operands");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_template_modifier_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %c0, %1, %2");
  prepared.inline_asm_carriers.functions[0].carriers[0].has_template_modifiers =
      true;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject template modifiers");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_clobber_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module();
  prepared.inline_asm_carriers.functions[0].carriers[0].clobbers.push_back("memory");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject clobbers");
  }
  return 0;
}

int rejects_structured_prepared_inline_asm_insn_r_closed_surface_object() {
  auto named = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %[dst], %1, %2",
      true,
      false,
      true);
  named.inline_asm_carriers.functions[0].carriers[0].has_named_operand_references =
      true;
  if (rv64::build_rv64_prepared_text_object_module(named).has_value()) {
    return fail("expected structured .insn r object path to reject named operands");
  }

  auto modifier = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %c0, %1, %2",
      true,
      false,
      true);
  modifier.inline_asm_carriers.functions[0].carriers[0].has_template_modifiers =
      true;
  if (rv64::build_rv64_prepared_text_object_module(modifier).has_value()) {
    return fail("expected structured .insn r object path to reject template modifiers");
  }

  auto clobber = make_prepared_inline_asm_insn_r_module(
      ".insn r 0x33, 0, 0, %0, %1, %2",
      true,
      false,
      true);
  clobber.inline_asm_carriers.functions[0].carriers[0].clobbers.push_back(
      "memory");
  if (rv64::build_rv64_prepared_text_object_module(clobber).has_value()) {
    return fail("expected structured .insn r object path to reject clobbers");
  }

  return 0;
}

int rejects_prepared_inline_asm_insn_r_unsupported_operand_kind_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module();
  prepared.inline_asm_carriers.functions[0].carriers[0].operands[1].kind =
      bir::InlineAsmOperandKind::IntegerImmediateInput;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject non-register operands");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_unsupported_constraint_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module();
  prepared.inline_asm_carriers.functions[0].carriers[0].operands[1].constraint = "v";
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject unsupported constraints");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_r_vector_home_object() {
  auto prepared = make_prepared_inline_asm_insn_r_module();
  auto& home =
      *prepared.inline_asm_carriers.functions[0].carriers[0].operands[1].home;
  home.register_name = "v1";
  home.target_register_identity->bank = prepare::PreparedRegisterBank::Vreg;
  home.target_register_identity->register_class = prepare::PreparedRegisterClass::Vector;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn r object path to reject vector homes");
  }
  return 0;
}

int classifies_prepared_inline_asm_insn_d_positional_shape() {
  const auto carrier = make_prepared_insn_d_carrier();
  const auto shape = rv64::classify_prepared_rv64_insn_d_inline_asm(carrier);
  if (!shape.has_value()) {
    return fail("expected positional RV64 EV .insn.d shape to classify");
  }
  if (shape->major != 0x0a || shape->operation != 0x0b || shape->dtype != 0x03) {
    return fail("expected .insn.d classifier to read prepared i immediates");
  }
  if (shape->destination.bank != rv64::RiscvInsnDInlineAsmRegisterBank::Vector ||
      shape->destination.physical_index != 20 ||
      shape->destination.group_width != 2 ||
      shape->lhs.physical_index != 4 || shape->rhs.physical_index != 6 ||
      shape->accumulator.physical_index != 8) {
    return fail("expected .insn.d classifier to publish vector base register identities");
  }
  return 0;
}

int encodes_prepared_inline_asm_insn_d_positional_shape() {
  const auto carrier = make_prepared_insn_d_carrier();
  const auto shape = rv64::classify_prepared_rv64_insn_d_inline_asm(carrier);
  if (!shape.has_value()) {
    return fail("expected positional RV64 EV .insn.d shape to classify before encode");
  }
  const auto encoded = rv64::encode_rv64_ev_insn_d_inline_asm(*shape);
  if (!encoded.has_value()) {
    return fail("expected positional RV64 EV .insn.d shape to encode");
  }
  if (*encoded != 0x0000030b10620a0aull) {
    return fail("expected EV .insn.d fields to land in documented 64-bit bits");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_out_of_range_fields() {
  auto carrier = make_prepared_insn_d_carrier();
  carrier.operands[4].immediate_value = std::int64_t{0x80};
  auto shape = rv64::classify_prepared_rv64_insn_d_inline_asm(carrier);
  if (!shape.has_value() ||
      rv64::encode_rv64_ev_insn_d_inline_asm(*shape).has_value()) {
    return fail("expected EV .insn.d encoder to reject 7-bit namespace overflow");
  }

  carrier = make_prepared_insn_d_carrier();
  carrier.operands[5].immediate_value = std::int64_t{0x100};
  shape = rv64::classify_prepared_rv64_insn_d_inline_asm(carrier);
  if (!shape.has_value() ||
      rv64::encode_rv64_ev_insn_d_inline_asm(*shape).has_value()) {
    return fail("expected EV .insn.d encoder to reject 8-bit operation overflow");
  }

  carrier = make_prepared_insn_d_carrier();
  carrier.operands[6].immediate_value = std::int64_t{0x10000};
  shape = rv64::classify_prepared_rv64_insn_d_inline_asm(carrier);
  if (!shape.has_value() ||
      rv64::encode_rv64_ev_insn_d_inline_asm(*shape).has_value()) {
    return fail("expected EV .insn.d encoder to reject 16-bit dtype overflow");
  }
  return 0;
}

int builds_prepared_inline_asm_insn_d_object() {
  const auto prepared = make_prepared_inline_asm_insn_d_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 inline-asm .insn.d object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected inline-asm .insn.d object to publish text/main");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 12) {
    return fail("expected inline-asm .insn.d object text layout");
  }
  if (read_u64(text->bytes, 0) != 0x0000030b10620a0aull ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected EV .insn.d bytes followed by return");
  }
  if (!module->relocations.empty()) {
    return fail("expected inline-asm .insn.d object to need no relocations");
  }
  return 0;
}

int builds_prepared_inline_asm_insn_d_adjacent_template_object() {
  auto carrier = make_prepared_insn_d_carrier(
      ".insn.d "
      "%4, %5, "
      "%0, %1, "
      "%2, %3, %6");
  const auto prepared = make_prepared_inline_asm_insn_d_module(std::move(carrier));
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module) {
    return fail("expected adjacent-fragment RV64 inline-asm .insn.d object module to build");
  }
  const auto* text = find_section(*module, ".text");
  const auto* main = find_symbol(*module, "main");
  if (!text || !main) {
    return fail("expected adjacent-fragment inline-asm .insn.d object to publish text/main");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      main->value != 0 || main->size_bytes != 12) {
    return fail("expected adjacent-fragment inline-asm .insn.d object text layout");
  }
  if (read_u64(text->bytes, 0) != 0x0000030b10620a0aull ||
      read_u32(text->bytes, 8) != 0x00008067u) {
    return fail("expected adjacent-fragment EV .insn.d bytes followed by return");
  }
  if (!module->relocations.empty()) {
    return fail("expected adjacent-fragment inline-asm .insn.d object to need no relocations");
  }
  return 0;
}

int builds_prepared_inline_asm_insn_d_helper_template_object() {
  auto carrier =
      make_prepared_insn_d_carrier(helper_style_rv64_insn_d_template_text());
  const auto prepared = make_prepared_inline_asm_insn_d_module(std::move(carrier));
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module) {
    return fail("expected helper-built RV64 inline-asm .insn.d object module to build");
  }
  const auto* text = find_section(*module, ".text");
  const auto* main = find_symbol(*module, "main");
  if (!text || !main) {
    return fail("expected helper-built inline-asm .insn.d object to publish text/main");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      main->value != 0 || main->size_bytes != 12) {
    return fail("expected helper-built inline-asm .insn.d object text layout");
  }
  if (read_u64(text->bytes, 0) != 0x0000030b10620a0aull ||
      read_u32(text->bytes, 8) != 0x00008067u) {
    return fail("expected helper-built EV .insn.d bytes followed by return");
  }
  if (!module->relocations.empty()) {
    return fail("expected helper-built inline-asm .insn.d object to need no relocations");
  }
  return 0;
}

int builds_prepared_mixed_inline_asm_insn_object() {
  const auto prepared = make_prepared_mixed_inline_asm_insn_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected mixed prepared RV64 inline-asm object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected mixed inline-asm object to publish text/main");
  }
  if (text->bytes.size() != 20 || text->size_bytes != 20 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 20) {
    return fail("expected mixed inline-asm object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x007302b3 ||
      read_u64(text->bytes, 4) != 0x0000030b10620a0aull ||
      read_u32(text->bytes, 12) != 0x00028513 ||
      read_u32(text->bytes, 16) != 0x00008067) {
    return fail("expected .insn r, EV .insn.d, and return bytes in order");
  }
  if (!module->relocations.empty()) {
    return fail("expected mixed inline-asm object to need no relocations");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_out_of_range_object() {
  auto carrier = make_prepared_insn_d_carrier();
  carrier.operands[6].immediate_value = std::int64_t{0x10000};
  const auto prepared = make_prepared_inline_asm_insn_d_module(std::move(carrier));
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm .insn.d object path to reject invalid fields");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_missing_field_shape() {
  const auto carrier = make_prepared_insn_d_carrier(
      ".insn.d %4, %5, %0, %1, %2, %3");
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to reject missing positional field");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_extra_field_shape() {
  const auto carrier = make_prepared_insn_d_carrier(
      ".insn.d %4, %5, %0, %1, %2, %3, %6, %1");
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to reject extra positional field");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_literal_immediate_shape() {
  const auto carrier = make_prepared_insn_d_carrier(
      ".insn.d 0x0a, %5, %0, %1, %2, %3, %6");
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to require immediate placeholders");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_missing_immediate_value_shape() {
  auto carrier = make_prepared_insn_d_carrier();
  carrier.operands[4].immediate_value = std::nullopt;
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to require prepared compile-time immediates");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_register_in_immediate_slot_shape() {
  const auto carrier = make_prepared_insn_d_carrier(
      ".insn.d %1, %5, %0, %1, %2, %3, %6");
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to reject register operand in immediate field");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_unsupported_register_operand_shape() {
  auto carrier = make_prepared_insn_d_carrier();
  carrier.operands[1].kind = bir::InlineAsmOperandKind::MemoryInput;
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to reject unsupported register operand kind");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_named_operand_shape() {
  auto carrier = make_prepared_insn_d_carrier(
      ".insn.d %[major], %5, %0, %1, %2, %3, %6");
  carrier.has_named_operand_references = true;
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to reject named operands");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_template_modifier_shape() {
  auto carrier = make_prepared_insn_d_carrier(
      ".insn.d %c4, %5, %0, %1, %2, %3, %6");
  carrier.has_template_modifiers = true;
  if (rv64::classify_prepared_rv64_insn_d_inline_asm(carrier).has_value()) {
    return fail("expected .insn.d classifier to reject template modifiers");
  }
  return 0;
}

int rejects_prepared_inline_asm_insn_d_object() {
  const auto prepared = make_prepared_inline_asm_insn_r_module(
      ".insn.d 0x2b, 0, 0, %0, %1, %2");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected inline-asm object path to reject EV .insn.d");
  }
  return 0;
}

int rejects_prepared_data_without_asm_fallback() {
  auto prepared = make_prepared_direct_call_module();
  prepared.module.globals.push_back(bir::Global{
      .name = "g",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(1),
  });
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected prepared RV64 object path to reject unsupported globals");
  }
  return 0;
}

int serializes_rv64_relocatable_elf_contract() {
  const auto module = make_minimal_call_module();
  if (!module.has_value()) {
    return fail("expected RV64 object module construction to succeed");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to produce an image");
  }
  const auto& bytes = image->bytes;
  if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' ||
      bytes[2] != 'L' || bytes[3] != 'F') {
    return fail("expected ELF image magic");
  }
  if (bytes[4] != 2 || bytes[5] != 1 || read_u16(bytes, 16) != 1 ||
      read_u16(bytes, 18) != 243 || read_u32(bytes, 48) != 0x5) {
    return fail("expected ELF64 little-endian relocatable RV64 header");
  }

  const std::size_t shoff = read_u64(bytes, 40);
  const std::size_t shentsize = read_u16(bytes, 58);
  const std::size_t shnum = read_u16(bytes, 60);
  const std::size_t shstrndx = read_u16(bytes, 62);
  if (shoff == 0 || shentsize != 64 || shnum < 5 || shstrndx >= shnum) {
    return fail("expected valid section header table");
  }
  const std::size_t shstr_header = shoff + shstrndx * shentsize;
  const std::size_t shstr_offset = read_u64(bytes, shstr_header + 24);

  std::size_t rela_text_header = 0;
  std::size_t symtab_header = 0;
  std::size_t text_header = 0;
  for (std::size_t index = 1; index < shnum; ++index) {
    const std::size_t header = shoff + index * shentsize;
    const std::string name =
        read_c_string(bytes, shstr_offset + read_u32(bytes, header));
    if (name == ".text") {
      text_header = header;
    } else if (name == ".rela.text") {
      rela_text_header = header;
    } else if (name == ".symtab") {
      symtab_header = header;
    }
  }
  if (text_header == 0 || rela_text_header == 0 || symtab_header == 0) {
    return fail("expected .text, .rela.text, and .symtab sections");
  }
  if (read_u64(bytes, text_header + 32) != 16 ||
      read_u32(bytes, rela_text_header + 4) != SHT_RELA ||
      read_u32(bytes, symtab_header + 4) != SHT_SYMTAB) {
    return fail("expected RV64 text and relocation section shapes");
  }

  const std::size_t rela_offset = read_u64(bytes, rela_text_header + 24);
  const std::size_t rela_size = read_u64(bytes, rela_text_header + 32);
  const std::size_t rela_entsize = read_u64(bytes, rela_text_header + 56);
  if (rela_size != 24 || rela_entsize != 24 || read_u64(bytes, rela_offset) != 0) {
    return fail("expected one call-pair relocation at text offset zero");
  }
  const std::uint64_t r_info = read_u64(bytes, rela_offset + 8);
  if ((r_info & 0xffffffffull) != 19) {
    return fail("expected serialized R_RISCV_CALL_PLT relocation type");
  }

  const std::size_t symtab_offset = read_u64(bytes, symtab_header + 24);
  const std::size_t symbol_index = r_info >> 32;
  const std::size_t symbol_offset = symtab_offset + symbol_index * 24;
  const std::size_t strtab_header =
      shoff + read_u32(bytes, symtab_header + 40) * shentsize;
  const std::size_t strtab_offset = read_u64(bytes, strtab_header + 24);
  const std::string symbol_name =
      read_c_string(bytes, strtab_offset + read_u32(bytes, symbol_offset));
  if (symbol_name != "callee" || read_u16(bytes, symbol_offset + 6) != 0) {
    return fail("expected relocation to reference undefined callee symbol");
  }
  return 0;
}

int serializes_pcrel_hi_lo_relocations_with_auipc_label_symbol() {
  const auto module = make_minimal_pcrel_module();
  if (!module.has_value()) {
    return fail("expected RV64 pcrel object module construction to succeed");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 pcrel ELF writer to produce an image");
  }
  const auto& bytes = image->bytes;
  const std::size_t shoff = read_u64(bytes, 40);
  const std::size_t shentsize = read_u16(bytes, 58);
  const std::size_t shnum = read_u16(bytes, 60);
  const std::size_t shstrndx = read_u16(bytes, 62);
  if (bytes.size() < 64 || shoff == 0 || shentsize != 64 || shstrndx >= shnum) {
    return fail("expected pcrel ELF section headers");
  }

  const std::size_t shstr_header = shoff + shstrndx * shentsize;
  const std::size_t shstr_offset = read_u64(bytes, shstr_header + 24);
  std::size_t text_header = 0;
  std::size_t rela_text_header = 0;
  std::size_t symtab_header = 0;
  for (std::size_t index = 1; index < shnum; ++index) {
    const std::size_t header = shoff + index * shentsize;
    const std::string name =
        read_c_string(bytes, shstr_offset + read_u32(bytes, header));
    if (name == ".text") {
      text_header = header;
    } else if (name == ".rela.text") {
      rela_text_header = header;
    } else if (name == ".symtab") {
      symtab_header = header;
    }
  }
  if (text_header == 0 || rela_text_header == 0 || symtab_header == 0) {
    return fail("expected pcrel ELF to include .text, .rela.text, and .symtab");
  }
  if (read_u64(bytes, text_header + 32) != 16 ||
      read_u32(bytes, rela_text_header + 4) != SHT_RELA ||
      read_u32(bytes, symtab_header + 4) != SHT_SYMTAB) {
    return fail("expected pcrel ELF section sizes and types");
  }

  const std::size_t symtab_offset = read_u64(bytes, symtab_header + 24);
  const std::size_t strtab_header =
      shoff + read_u32(bytes, symtab_header + 40) * shentsize;
  const std::size_t strtab_offset = read_u64(bytes, strtab_header + 24);
  const auto symbol_name_at = [&](std::size_t symbol_index) {
    const std::size_t symbol_offset = symtab_offset + symbol_index * 24;
    return read_c_string(bytes, strtab_offset + read_u32(bytes, symbol_offset));
  };
  const auto symbol_section_at = [&](std::size_t symbol_index) {
    const std::size_t symbol_offset = symtab_offset + symbol_index * 24;
    return read_u16(bytes, symbol_offset + 6);
  };
  const auto symbol_value_at = [&](std::size_t symbol_index) {
    const std::size_t symbol_offset = symtab_offset + symbol_index * 24;
    return read_u64(bytes, symbol_offset + 8);
  };

  const std::size_t rela_offset = read_u64(bytes, rela_text_header + 24);
  const std::size_t rela_size = read_u64(bytes, rela_text_header + 32);
  const std::size_t rela_entsize = read_u64(bytes, rela_text_header + 56);
  if (rela_size != 48 || rela_entsize != 24) {
    return fail("expected pcrel ELF to serialize exactly two rela entries");
  }
  const std::uint64_t hi_offset = read_u64(bytes, rela_offset);
  const std::uint64_t hi_info = read_u64(bytes, rela_offset + 8);
  const std::uint64_t hi_symbol = hi_info >> 32;
  const std::uint64_t lo_offset = read_u64(bytes, rela_offset + 24);
  const std::uint64_t lo_info = read_u64(bytes, rela_offset + 32);
  const std::uint64_t lo_symbol = lo_info >> 32;
  if (hi_offset != 0 || (hi_info & 0xffffffffull) != R_RISCV_PCREL_HI20 ||
      read_u64(bytes, rela_offset + 16) != 0) {
    return fail("expected pcrel high relocation at AUIPC offset zero");
  }
  if (lo_offset != 4 || (lo_info & 0xffffffffull) != R_RISCV_PCREL_LO12_I ||
      read_u64(bytes, rela_offset + 40) != 0) {
    return fail("expected pcrel low relocation at ADDI offset four");
  }
  if (symbol_name_at(hi_symbol) != "target" ||
      symbol_section_at(hi_symbol) != SHN_UNDEF) {
    return fail("expected pcrel high relocation to reference final target symbol");
  }
  if (symbol_name_at(lo_symbol) != ".Lpcrel_hi_load_addr_0" ||
      symbol_section_at(lo_symbol) == SHN_UNDEF ||
      symbol_value_at(lo_symbol) != 0) {
    return fail("expected pcrel low relocation to reference AUIPC-site label symbol");
  }
  return 0;
}

int writes_prepared_rv64_relocatable_elf_object_file() {
  const auto prepared = make_prepared_direct_call_module();
  const auto image = rv64::write_rv64_prepared_relocatable_elf_object(prepared);
  if (!image.has_value()) {
    return fail("expected prepared RV64 ELF writer to produce an image");
  }
  const auto& bytes = image->bytes;
  if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' ||
      bytes[2] != 'L' || bytes[3] != 'F' || read_u16(bytes, 18) != 243 ||
      read_u32(bytes, 48) != 0x5) {
    return fail("expected prepared object to serialize as RV64 ELF64");
  }

  const std::size_t shoff = read_u64(bytes, 40);
  const std::size_t shentsize = read_u16(bytes, 58);
  const std::size_t shnum = read_u16(bytes, 60);
  const std::size_t shstrndx = read_u16(bytes, 62);
  if (shoff == 0 || shentsize != 64 || shstrndx >= shnum) {
    return fail("expected prepared ELF section headers");
  }
  const std::size_t shstr_header = shoff + shstrndx * shentsize;
  const std::size_t shstr_offset = read_u64(bytes, shstr_header + 24);

  std::size_t text_header = 0;
  std::size_t rela_text_header = 0;
  std::size_t symtab_header = 0;
  for (std::size_t index = 1; index < shnum; ++index) {
    const std::size_t header = shoff + index * shentsize;
    const std::string name =
        read_c_string(bytes, shstr_offset + read_u32(bytes, header));
    if (name == ".text") {
      text_header = header;
    } else if (name == ".rela.text") {
      rela_text_header = header;
    } else if (name == ".symtab") {
      symtab_header = header;
    }
  }
  if (text_header == 0 || rela_text_header == 0 || symtab_header == 0) {
    return fail("expected prepared ELF to include .text, .rela.text, and .symtab");
  }
  if (read_u64(bytes, text_header + 32) != 40 ||
      read_u32(bytes, rela_text_header + 4) != SHT_RELA ||
      read_u32(bytes, symtab_header + 4) != SHT_SYMTAB) {
    return fail("expected prepared ELF section sizes and types");
  }

  const std::size_t rela_offset = read_u64(bytes, rela_text_header + 24);
  const std::size_t rela_size = read_u64(bytes, rela_text_header + 32);
  const std::size_t rela_entsize = read_u64(bytes, rela_text_header + 56);
  if (rela_size != 24 || rela_entsize != 24 || read_u64(bytes, rela_offset) != 8) {
    return fail("expected one prepared call relocation after call frame setup");
  }
  const std::uint64_t r_info = read_u64(bytes, rela_offset + 8);
  if ((r_info & 0xffffffffull) != 19) {
    return fail("expected prepared call relocation to be R_RISCV_CALL_PLT");
  }

  const std::size_t symtab_offset = read_u64(bytes, symtab_header + 24);
  const std::size_t strtab_header =
      shoff + read_u32(bytes, symtab_header + 40) * shentsize;
  const std::size_t strtab_offset = read_u64(bytes, strtab_header + 24);
  const auto symbol_name_at = [&](std::size_t symbol_index) {
    const std::size_t symbol_offset = symtab_offset + symbol_index * 24;
    return read_c_string(bytes, strtab_offset + read_u32(bytes, symbol_offset));
  };
  const std::size_t relocated_symbol_index = r_info >> 32;
  const std::size_t relocated_symbol_offset =
      symtab_offset + relocated_symbol_index * 24;
  if (symbol_name_at(relocated_symbol_index) != "callee" ||
      read_u16(bytes, relocated_symbol_offset + 6) == SHN_UNDEF) {
    return fail("expected prepared relocation to reference defined callee symbol");
  }

  bool saw_caller = false;
  bool saw_callee = false;
  const std::size_t symbol_count = read_u64(bytes, symtab_header + 32) / 24;
  for (std::size_t index = 1; index < symbol_count; ++index) {
    const std::size_t symbol_offset = symtab_offset + index * 24;
    const std::string name = symbol_name_at(index);
    if (name == "caller") {
      saw_caller = read_u64(bytes, symbol_offset + 8) == 0 &&
                   read_u64(bytes, symbol_offset + 16) == 32 &&
                   read_u16(bytes, symbol_offset + 6) != SHN_UNDEF;
    } else if (name == "callee") {
      saw_callee = read_u64(bytes, symbol_offset + 8) == 32 &&
                   read_u64(bytes, symbol_offset + 16) == 8 &&
                   read_u16(bytes, symbol_offset + 6) != SHN_UNDEF;
    }
  }
  if (!saw_caller || !saw_callee) {
    return fail("expected prepared object to publish defined caller/callee symbols");
  }

  const auto temp_dir = std::filesystem::temp_directory_path();
  const auto object_path = temp_dir / "c4c_rv64_prepared_object_emission_test.o";
  const auto linked_path = temp_dir / "c4c_rv64_prepared_object_emission_test.r.o";
  {
    std::ofstream out(object_path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!out.good()) {
      return fail("expected to write prepared ELF object temp file");
    }
  }

  const std::string object_arg = shell_quote(object_path);
  const std::string linked_arg = shell_quote(linked_path);
  if (std::system(("readelf -h -S -r -s " + object_arg + " > /dev/null").c_str()) !=
      0) {
    std::filesystem::remove(object_path);
    return fail("expected readelf to accept prepared RV64 object");
  }
  if (std::system(("llvm-objdump -r " + object_arg + " > /dev/null").c_str()) !=
      0) {
    std::filesystem::remove(object_path);
    return fail("expected llvm-objdump to read prepared RV64 relocations");
  }
  if (std::system(("riscv64-linux-gnu-ld -r -o " + linked_arg + " " +
                   object_arg + " > /dev/null").c_str()) != 0) {
    std::filesystem::remove(object_path);
    std::filesystem::remove(linked_path);
    return fail("expected riscv64-linux-gnu-ld -r to accept prepared RV64 object");
  }
  std::filesystem::remove(object_path);
  std::filesystem::remove(linked_path);
  return 0;
}

}  // namespace

int main() {
  int status = 0;
  status |= records_minimal_text_and_call_relocation();
  status |= records_same_module_direct_call_symbol();
  status |= records_pcrel_hi_lo_pairing_with_auipc_site_label();
  status |= builds_prepared_text_object_module_without_call_text();
  status |= builds_prepared_rematerialized_nonzero_return_object();
  status |= builds_prepared_scalar_same_module_call_object();
  status |= builds_prepared_two_arg_scalar_call_object();
  status |= builds_prepared_scalar_local_frame_object();
  status |= builds_prepared_local_register_arg_call_object();
  status |= builds_prepared_inline_asm_insn_r_object();
  status |= builds_structured_prepared_inline_asm_insn_r_object_without_text_reparse();
  status |= builds_prepared_inline_asm_insn_r_tied_input_object();
  status |= builds_structured_prepared_inline_asm_insn_r_readwrite_object();
  status |= substitutes_prepared_rv64_vector_inline_asm_base_registers();
  status |= substitutes_prepared_rv64_mixed_scalar_vector_inline_asm_registers();
  status |= substitutes_prepared_rv64_tied_vector_inline_asm_base_register();
  status |= parses_rv64_line_core_canonical_subset();
  status |= rejects_rv64_line_core_malformed_subset();
  status |= encodes_rv64_line_core_canonical_subset();
  status |= rejects_prepared_inline_asm_insn_r_without_complete_carrier();
  status |= rejects_structured_prepared_inline_asm_insn_r_bad_operand_metadata_object();
  status |= rejects_prepared_inline_asm_non_insn_r_object();
  status |= rejects_prepared_inline_asm_insn_r_extra_field_object();
  status |= rejects_prepared_inline_asm_insn_r_missing_field_object();
  status |= rejects_prepared_inline_asm_insn_r_out_of_range_numeric_object();
  status |= rejects_prepared_inline_asm_insn_r_bad_operand_token_object();
  status |= rejects_prepared_inline_asm_insn_r_named_operand_object();
  status |= rejects_prepared_inline_asm_insn_r_template_modifier_object();
  status |= rejects_prepared_inline_asm_insn_r_clobber_object();
  status |= rejects_structured_prepared_inline_asm_insn_r_closed_surface_object();
  status |= rejects_prepared_inline_asm_insn_r_unsupported_operand_kind_object();
  status |= rejects_prepared_inline_asm_insn_r_unsupported_constraint_object();
  status |= rejects_prepared_inline_asm_insn_r_vector_home_object();
  status |= classifies_prepared_inline_asm_insn_d_positional_shape();
  status |= encodes_prepared_inline_asm_insn_d_positional_shape();
  status |= rejects_prepared_inline_asm_insn_d_out_of_range_fields();
  status |= builds_prepared_inline_asm_insn_d_object();
  status |= builds_prepared_inline_asm_insn_d_adjacent_template_object();
  status |= builds_prepared_inline_asm_insn_d_helper_template_object();
  status |= builds_prepared_mixed_inline_asm_insn_object();
  status |= rejects_prepared_inline_asm_insn_d_out_of_range_object();
  status |= rejects_prepared_inline_asm_insn_d_missing_field_shape();
  status |= rejects_prepared_inline_asm_insn_d_extra_field_shape();
  status |= rejects_prepared_inline_asm_insn_d_literal_immediate_shape();
  status |= rejects_prepared_inline_asm_insn_d_missing_immediate_value_shape();
  status |= rejects_prepared_inline_asm_insn_d_register_in_immediate_slot_shape();
  status |= rejects_prepared_inline_asm_insn_d_unsupported_register_operand_shape();
  status |= rejects_prepared_inline_asm_insn_d_named_operand_shape();
  status |= rejects_prepared_inline_asm_insn_d_template_modifier_shape();
  status |= rejects_prepared_inline_asm_insn_d_object();
  status |= rejects_prepared_data_without_asm_fallback();
  status |= serializes_rv64_relocatable_elf_contract();
  status |= serializes_pcrel_hi_lo_relocations_with_auipc_label_symbol();
  status |= writes_prepared_rv64_relocatable_elf_object_file();
  return status;
}
