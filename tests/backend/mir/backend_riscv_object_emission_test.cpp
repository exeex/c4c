#include "src/backend/mir/object/model.hpp"
#include "src/backend/mir/riscv/codegen/object_emission.hpp"
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
  status |= rejects_prepared_data_without_asm_fallback();
  status |= serializes_rv64_relocatable_elf_contract();
  status |= serializes_pcrel_hi_lo_relocations_with_auipc_label_symbol();
  status |= writes_prepared_rv64_relocatable_elf_object_file();
  return status;
}
