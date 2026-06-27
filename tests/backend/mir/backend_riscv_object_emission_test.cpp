#include "src/backend/mir/object/model.hpp"
#include "src/backend/mir/riscv/codegen/object_emission.hpp"
#include "src/backend/mir/riscv/codegen/prepared_call_emit.hpp"
#include "src/backend/mir/riscv/codegen/prepared_emit_context.hpp"
#include "src/backend/mir/riscv/codegen/rv64_line_assembler.hpp"
#include "src/backend/prealloc/control_flow.hpp"
#include "src/backend/prealloc/module.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <variant>
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
constexpr std::uint32_t R_RISCV_BRANCH = 16;
constexpr std::uint32_t R_RISCV_JAL = 17;
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

bool contains_u32(const std::vector<std::uint8_t>& bytes, std::uint32_t word) {
  for (std::size_t offset = 0; offset + 4 <= bytes.size(); offset += 4) {
    if (read_u32(bytes, offset) == word) {
      return true;
    }
  }
  return false;
}

bir::Value null_pointer_value() {
  return bir::Value{
      .kind = bir::Value::Kind::Immediate,
      .type = bir::TypeKind::Ptr,
      .immediate = 0,
      .immediate_bits = 0,
  };
}

bir::Value nonnull_pointer_immediate(std::int64_t value) {
  return bir::Value{
      .kind = bir::Value::Kind::Immediate,
      .type = bir::TypeKind::Ptr,
      .immediate = value,
      .immediate_bits = static_cast<std::uint64_t>(value),
  };
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

int expect_prepared_rejection_diagnostic(
    const prepare::PreparedBirModule& prepared,
    const std::string& expected_diagnostic) {
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("expected prepared RV64 object path to reject");
  }
  if (result.prepared_consumer_category.has_value()) {
    return fail("expected RV64-local diagnostic rather than shared category");
  }
  if (result.diagnostic != expected_diagnostic) {
    return fail("expected prepared RV64 object diagnostic `" +
                expected_diagnostic + "`, got `" + result.diagnostic + "`");
  }
  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (image.ok() || image.image.has_value() ||
      image.prepared_consumer_category.has_value() ||
      image.diagnostic != expected_diagnostic) {
    return fail("expected prepared RV64 ELF writer to preserve RV64-local diagnostic");
  }
  return 0;
}

void publish_prepared_object_data(prepare::PreparedBirModule& prepared) {
  prepare::populate_prepared_object_data_plans(prepared);
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

prepare::PreparedBirModule make_prepared_critical_edge_parallel_copy_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("critical_edge_copy");
  const auto predecessor = prepared.names.block_labels.intern("pred");
  const auto successor = prepared.names.block_labels.intern("succ");

  prepared.module.functions.push_back(make_prepared_return_zero_function(
      "critical_edge_copy"));
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = predecessor,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
      .parallel_copy_bundles =
          {prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor,
              .successor_label = successor,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::CriticalEdge,
              .moves = {prepare::PreparedParallelCopyMove{}},
              .steps = {prepare::PreparedParallelCopyStep{}},
          }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_successor_entry_copy_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("entry_copy");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto exit_label = prepared.names.block_labels.intern("exit");
  const auto source_name = prepared.names.value_names.intern("%x");
  const auto destination_name = prepared.names.value_names.intern("%y");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.kind = bir::TerminatorKind::Branch;
  entry.terminator.target_label = "exit";
  bir::Block exit{
      .label = "exit",
      .terminator = bir::Terminator{},
  };
  exit.terminator.value = bir::Value::named(bir::TypeKind::I32, "%y");
  prepared.module.functions.push_back(bir::Function{
      .name = "entry_copy",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry), std::move(exit)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = exit_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = exit_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .parallel_copy_bundles =
          {prepare::PreparedParallelCopyBundle{
              .predecessor_label = entry_label,
              .successor_label = exit_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::SuccessorEntry,
              .execution_block_label = exit_label,
              .moves = {prepare::PreparedParallelCopyMove{
                  .source_value = bir::Value::named(bir::TypeKind::I32, "%x"),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "%y"),
              }},
              .steps = {prepare::PreparedParallelCopyStep{}},
          }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = source_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = destination_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
          },
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind =
                  prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 1,
              .source_parallel_copy_predecessor_label = entry_label,
              .source_parallel_copy_successor_label = exit_label,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 1,
                  .to_value_id = 2,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              }},
          }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_fused_compare_branch_module(
    bir::BinaryOpcode predicate,
    bir::TypeKind compare_type = bir::TypeKind::I32,
    std::optional<bir::Value> rhs_override = std::nullopt) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("cmp_branch");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto true_label = prepared.names.block_labels.intern("is_true");
  const auto false_label = prepared.names.block_labels.intern("is_false");
  const auto condition_name = prepared.names.value_names.intern("%cmp");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto lhs_value = bir::Value::named(compare_type, "%lhs");
  const auto rhs_value =
      rhs_override.has_value() ? *rhs_override : bir::Value::named(compare_type, "%rhs");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.kind = bir::TerminatorKind::CondBranch;
  entry.terminator.condition = bir::Value::named(bir::TypeKind::I32, "%cmp");
  entry.terminator.true_label = "is_true";
  entry.terminator.false_label = "is_false";

  bir::Block is_true{
      .label = "is_true",
      .terminator = bir::Terminator{},
  };
  is_true.terminator.value = bir::Value::immediate_i32(1);

  bir::Block is_false{
      .label = "is_false",
      .terminator = bir::Terminator{},
  };
  is_false.terminator.value = bir::Value::immediate_i32(0);

  prepared.module.functions.push_back(bir::Function{
      .name = "cmp_branch",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry), std::move(is_true), std::move(is_false)},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::CondBranch,
                  .true_label = true_label,
                  .false_label = false_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = true_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = false_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .branch_conditions =
          {
              prepare::PreparedBranchCondition{
                  .function_name = function_name,
                  .block_label = entry_label,
                  .kind = prepare::PreparedBranchConditionKind::FusedCompare,
                  .condition_value = bir::Value::named(bir::TypeKind::I32, "%cmp"),
                  .predicate = predicate,
                  .compare_type = compare_type,
                  .lhs = lhs_value,
                  .rhs = rhs_value,
                  .can_fuse_with_branch = true,
                  .true_label = true_label,
                  .false_label = false_label,
              },
          },
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = condition_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t2"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = lhs_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = function_name,
                  .value_name = rhs_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t1"},
              },
          },
  });

  return prepared;
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

prepare::PreparedBirModule make_prepared_variadic_return_zero_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("rv64_variadic");
  const auto entry_label = prepared.names.block_labels.intern("entry");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "rv64_variadic",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .is_variadic = true,
      .blocks = {std::move(entry)},
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

prepare::PreparedBirModule make_prepared_variadic_missing_required_facts_module() {
  auto prepared = make_prepared_variadic_return_zero_module();
  const auto function_name = prepared.names.function_names.find("rv64_variadic");
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .missing_required_facts = {"target_abi.va_list_layout"},
      });
  return prepared;
}

prepare::PreparedBirModule make_prepared_variadic_helper_free_incomplete_contract_module() {
  auto prepared = make_prepared_variadic_return_zero_module();
  const auto function_name = prepared.names.function_names.find("rv64_variadic");
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{8},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{8},
                  .align_bytes = std::size_t{8},
              },
      });
  return prepared;
}

prepare::PreparedBirModule make_prepared_variadic_helper_free_complete_module() {
  auto prepared = make_prepared_variadic_return_zero_module();
  const auto function_name = prepared.names.function_names.find("rv64_variadic");
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{8},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{8},
                  .align_bytes = std::size_t{8},
                  .fields = {prepare::PreparedVariadicVaListField{
                      .kind = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                      .offset_bytes = 0,
                      .size_bytes = 8,
                  }},
              },
      });
  return prepared;
}

prepare::PreparedBirModule make_prepared_variadic_va_start_module(
    bool include_overflow_area_initial_state = false,
    bool destination_va_list_is_stack_slot = true,
    bool destination_address_is_gpr = true,
    std::string destination_address_register_name = "a1") {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("rv64_va_start");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto ap_name = prepared.names.value_names.intern("%ap");
  const auto ap_addr_name = prepared.names.value_names.intern("%ap.addr");
  if (destination_va_list_is_stack_slot) {
    prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
        .slot_id = prepare::PreparedFrameSlotId{5},
        .object_id = 1,
        .function_name = function_name,
        .offset_bytes = 72,
        .size_bytes = 8,
        .align_bytes = 8,
    });
    prepared.stack_layout.frame_size_bytes = 80;
    prepared.stack_layout.frame_alignment_bytes = 16;
  }
  if (include_overflow_area_initial_state) {
    prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
        .slot_id = prepare::PreparedFrameSlotId{7},
        .object_id = 2,
        .function_name = function_name,
        .offset_bytes = 8,
        .size_bytes = 64,
        .align_bytes = 8,
    });
    prepared.stack_layout.frame_size_bytes = 80;
    prepared.stack_layout.frame_alignment_bytes = 16;
  }

  bir::CallInst va_start;
  va_start.callee = "llvm.va_start.p0";
  va_start.args = {bir::Value::named(bir::TypeKind::Ptr, "%ap.addr")};
  va_start.arg_types = {bir::TypeKind::Ptr};
  va_start.return_type = bir::TypeKind::Void;
  bir::Block entry{
      .label = "entry",
      .insts = {va_start},
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "rv64_va_start",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .is_variadic = true,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  const prepare::PreparedValueHome ap_home{
      .value_id = 1,
      .function_name = function_name,
      .value_name = ap_name,
      .kind = destination_va_list_is_stack_slot
                  ? prepare::PreparedValueHomeKind::StackSlot
                  : prepare::PreparedValueHomeKind::Register,
      .register_name = destination_va_list_is_stack_slot ? std::nullopt
                                                         : std::optional<std::string>{"a0"},
      .slot_id = destination_va_list_is_stack_slot
                     ? std::optional<prepare::PreparedFrameSlotId>{
                           prepare::PreparedFrameSlotId{5}}
                     : std::nullopt,
      .offset_bytes = destination_va_list_is_stack_slot
                          ? std::optional<std::size_t>{72}
                          : std::nullopt,
      .size_bytes = destination_va_list_is_stack_slot
                        ? std::optional<std::size_t>{8}
                        : std::nullopt,
      .align_bytes = destination_va_list_is_stack_slot
                         ? std::optional<std::size_t>{8}
                         : std::nullopt,
  };
  const prepare::PreparedValueHome ap_addr_home{
      .value_id = 2,
      .function_name = function_name,
      .value_name = ap_addr_name,
      .kind = destination_address_is_gpr
                  ? prepare::PreparedValueHomeKind::Register
                  : prepare::PreparedValueHomeKind::StackSlot,
      .register_name = destination_address_is_gpr
                           ? std::optional<std::string>{
                                 std::move(destination_address_register_name)}
                           : std::nullopt,
      .slot_id = destination_address_is_gpr
                     ? std::nullopt
                     : std::optional<prepare::PreparedFrameSlotId>{
                           prepare::PreparedFrameSlotId{5}},
      .offset_bytes = destination_address_is_gpr
                          ? std::nullopt
                          : std::optional<std::size_t>{72},
      .size_bytes = destination_address_is_gpr
                        ? std::nullopt
                        : std::optional<std::size_t>{8},
      .align_bytes = destination_address_is_gpr
                         ? std::nullopt
                         : std::optional<std::size_t>{8},
  };
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{8},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = include_overflow_area_initial_state
                                      ? std::optional<prepare::PreparedFrameSlotId>{7}
                                      : std::nullopt,
                  .base_stack_offset_bytes = include_overflow_area_initial_state
                                                 ? std::optional<std::size_t>{8}
                                                 : std::nullopt,
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{8},
                  .align_bytes = std::size_t{8},
                  .fields = {prepare::PreparedVariadicVaListField{
                      .kind = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                      .offset_bytes = 0,
                      .size_bytes = 8,
                  }},
              },
          .helper_resources =
              prepare::PreparedVariadicEntryHelperResources{
                  .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaStart},
                  .scratch_register_count = std::size_t{3},
                  .scratch_stack_bytes = std::size_t{0},
              },
          .helper_operand_homes =
              {prepare::PreparedVariadicEntryHelperOperandHomes{
                  .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
                  .block_index = 0,
                  .instruction_index = 0,
                  .destination_va_list = ap_home,
                  .destination_va_list_address = ap_addr_home,
              }},
      });
  return prepared;
}

void add_rv64_incoming_variadic_gpr_publications(
    prepare::PreparedVariadicEntryPlanFunction& entry_plan) {
  constexpr std::size_t kRv64ArgumentGprCount = 8;
  constexpr std::size_t kRv64ArgumentGprBytes = 8;
  const std::size_t named_gp_count =
      entry_plan.named_register_counts.gp.value_or(0);
  for (std::size_t abi_gpr_index = named_gp_count;
       abi_gpr_index < kRv64ArgumentGprCount;
       ++abi_gpr_index) {
    std::string source_register = "a";
    source_register += std::to_string(abi_gpr_index);
    const std::size_t destination_offset =
        (abi_gpr_index - named_gp_count) * kRv64ArgumentGprBytes;
    entry_plan.rv64_incoming_variadic_gpr_publications.push_back(
        prepare::PreparedRv64IncomingVariadicGprPublication{
            .abi_gpr_index = abi_gpr_index,
            .variadic_argument_index = abi_gpr_index - named_gp_count,
            .source_register_name = std::move(source_register),
            .destination_slot_id = *entry_plan.overflow_area.base_slot_id,
            .destination_stack_offset_bytes =
                *entry_plan.overflow_area.base_stack_offset_bytes +
                destination_offset,
            .destination_offset_bytes = destination_offset,
            .size_bytes = kRv64ArgumentGprBytes,
            .align_bytes = kRv64ArgumentGprBytes,
        });
  }
}

prepare::PreparedBirModule make_prepared_variadic_va_start_missing_saved_gpr_publication_module() {
  auto prepared = make_prepared_variadic_va_start_module(
      true /*include_overflow_area_initial_state*/);
  auto& entry_plan = prepared.variadic_entry_plans.functions.front();
  entry_plan.named_parameter_count = 1;
  entry_plan.named_register_counts.gp = std::size_t{1};
  entry_plan.missing_required_facts.push_back(
      "rv64.incoming_variadic_gpr_publications");
  return prepared;
}

prepare::PreparedBirModule make_prepared_variadic_va_start_with_saved_gpr_publications_module() {
  auto prepared = make_prepared_variadic_va_start_module(
      true /*include_overflow_area_initial_state*/,
      true /*destination_va_list_is_stack_slot*/,
      true /*destination_address_is_gpr*/,
      "a0");
  auto& entry_plan = prepared.variadic_entry_plans.functions.front();
  entry_plan.named_parameter_count = 1;
  entry_plan.named_register_counts.gp = std::size_t{1};
  add_rv64_incoming_variadic_gpr_publications(entry_plan);
  return prepared;
}

prepare::PreparedBirModule
make_prepared_variadic_va_start_then_load_published_word_module() {
  auto prepared = make_prepared_variadic_va_start_with_saved_gpr_publications_module();
  const auto function_name = prepared.names.function_names.find("rv64_va_start");
  const auto block_label = prepared.names.block_labels.find("entry");
  const auto loaded_name = prepared.names.value_names.intern("%loaded.ap");
  const auto stale_slot_name = prepared.names.slot_names.intern("%ap.addr");

  auto& entry = prepared.module.functions[0].blocks[0];
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%loaded.ap"),
      .slot_name = "%ap.addr",
      .slot_id = stale_slot_name,
      .align_bytes = 8,
  });

  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{8},
      .object_id = 3,
      .function_name = function_name,
      .offset_bytes = 0,
      .size_bytes = 8,
      .align_bytes = 8,
      .fixed_location = true,
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 3,
      .function_name = function_name,
      .value_name = prepared.names.value_names.find("%ap.addr"),
      .source_kind = "local_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 3,
          .function_name = function_name,
          .value_name = loaded_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a2"},
          .size_bytes = 8,
          .align_bytes = 8,
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 1,
          .result_value_name = loaded_name,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = prepare::PreparedFrameSlotId{8},
              .byte_offset = 0,
              .size_bytes = 8,
              .align_bytes = 8,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_variadic_va_end_module(
    std::string prepared_callee = "llvm.va_end.p0",
    std::size_t arg_count = 1) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("rv64_va_end");
  const auto entry_label = prepared.names.block_labels.intern("entry");

  bir::CallInst va_end;
  va_end.callee = "llvm.va_end.p0";
  va_end.return_type = bir::TypeKind::Void;
  for (std::size_t i = 0; i < arg_count; ++i) {
    va_end.args.push_back(
        bir::Value::named(bir::TypeKind::Ptr, i == 0 ? "%ap" : "%extra"));
    va_end.arg_types.push_back(bir::TypeKind::Ptr);
  }
  bir::Block entry{
      .label = "entry",
      .insts = {va_end},
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "rv64_va_end",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .is_variadic = true,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = prepare::PreparedFrameSlotId{7},
                  .base_stack_offset_bytes = std::size_t{64},
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{8},
                  .align_bytes = std::size_t{8},
                  .fields = {prepare::PreparedVariadicVaListField{
                      .kind = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                      .offset_bytes = 0,
                      .size_bytes = 8,
                  }},
              },
      });

  prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 0,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::move(prepared_callee),
  };
  for (std::size_t i = 0; i < arg_count; ++i) {
    call_plan.arguments.push_back(prepare::PreparedCallArgumentPlan{
        .instruction_index = 0,
        .arg_index = i,
        .value_bank = prepare::PreparedRegisterBank::Gpr,
        .source_encoding = prepare::PreparedStorageEncodingKind::Register,
        .source_value_id = prepare::PreparedValueId{i + 1},
        .source_register_name = std::string{i == 0 ? "s1" : "s2"},
        .source_register_bank = prepare::PreparedRegisterBank::Gpr,
        .destination_register_name = std::string{i == 0 ? "a0" : "a1"},
        .destination_contiguous_width = 1,
        .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
    });
  }
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {std::move(call_plan)},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_variadic_aggregate_va_arg_module(
    bool include_access_plan_payload_write_address) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("rv64_aggregate_va_arg");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto ap_name = prepared.names.value_names.intern("%ap");
  const auto aggregate_name = prepared.names.value_names.intern("%aggregate");
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = 5,
      .object_id = 1,
      .function_name = function_name,
      .offset_bytes = 32,
      .size_bytes = 9,
      .align_bytes = 1,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = 8,
      .object_id = 2,
      .function_name = function_name,
      .offset_bytes = 40,
      .size_bytes = 0,
      .align_bytes = 8,
  });
  prepared.stack_layout.frame_size_bytes = 48;
  prepared.stack_layout.frame_alignment_bytes = 16;

  bir::CallInst va_arg;
  va_arg.callee = "llvm.va_arg.aggregate";
  va_arg.args = {bir::Value::named(bir::TypeKind::Ptr, "%aggregate"),
                 bir::Value::named(bir::TypeKind::Ptr, "%ap")};
  va_arg.arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr};
  va_arg.return_type = bir::TypeKind::Void;
  bir::Block entry{
      .label = "entry",
      .insts = {va_arg},
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "rv64_aggregate_va_arg",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .is_variadic = true,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  const prepare::PreparedValueHome ap_home{
      .value_id = 1,
      .function_name = function_name,
      .value_name = ap_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"s1"},
  };
  const prepare::PreparedValueHome aggregate_payload_home{
      .value_id = 2,
      .function_name = function_name,
      .value_name = aggregate_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = std::size_t{5},
      .offset_bytes = std::size_t{32},
  };
  prepare::PreparedVariadicAggregateVaArgAccessPlan aggregate_access_plan{
      .source_class = prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
      .block_index = 0,
      .instruction_index = 0,
      .payload_size_bytes = 9,
      .payload_align_bytes = 1,
      .destination_payload_home = aggregate_payload_home,
      .source_field = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
      .source_field_offset_bytes = std::size_t{0},
      .source_payload_offset_bytes = std::size_t{0},
      .source_slot_size_bytes = std::size_t{9},
      .copy_size_bytes = std::size_t{9},
      .copy_align_bytes = std::size_t{1},
      .progression_field = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
      .progression_field_offset_bytes = std::size_t{0},
      .progression_stride_bytes = std::size_t{9},
      .overflow_source_field_offset_bytes = std::size_t{0},
      .overflow_stride_bytes = std::size_t{9},
  };
  if (include_access_plan_payload_write_address) {
    aggregate_access_plan.payload_write_address =
        prepare::PreparedVariadicAggregatePayloadWriteAddress{
            .result_value_name = aggregate_payload_home.value_name,
            .materialization_block_label = entry_label,
            .materialization_instruction_index = 0,
            .frame_slot_id = prepare::PreparedFrameSlotId{5},
            .stack_offset_bytes = 32,
        };
  }
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .named_parameter_count = 1,
          .named_register_counts =
              prepare::PreparedVariadicEntryNamedRegisterCounts{
                  .gp = std::size_t{8},
              },
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = prepare::PreparedFrameSlotId{8},
                  .base_stack_offset_bytes = std::size_t{40},
                  .align_bytes = std::size_t{8},
              },
          .va_list_layout =
              prepare::PreparedVariadicVaListLayout{
                  .required = true,
                  .size_bytes = std::size_t{8},
                  .align_bytes = std::size_t{8},
                  .fields = {prepare::PreparedVariadicVaListField{
                      .kind = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
                      .offset_bytes = 0,
                      .size_bytes = 8,
                  }},
              },
          .helper_resources =
              prepare::PreparedVariadicEntryHelperResources{
                  .required_helpers = {prepare::PreparedVariadicEntryHelperKind::VaArgAggregate},
                  .scratch_register_count = std::size_t{3},
                  .scratch_stack_bytes = std::size_t{0},
              },
          .helper_operand_homes =
              {prepare::PreparedVariadicEntryHelperOperandHomes{
                  .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_va_list = ap_home,
                  .aggregate_destination_payload = aggregate_payload_home,
                  .aggregate_access_plan = aggregate_access_plan,
              }},
      });
  return prepared;
}

prepare::PreparedBirModule make_prepared_symbol_address_module(
    prepare::PreparedAddressMaterializationKind kind,
    std::string symbol_name) {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%addr");
  const auto link_name = prepared.names.link_names.intern(symbol_name);
  const auto text_name = prepared.names.texts.intern(symbol_name);

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::Ptr, "%addr"),
                  .operand_type = bir::TypeKind::Ptr,
                  .lhs = bir::Value::immediate_i64(0),
                  .rhs = bir::Value::immediate_i64(0),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
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
                  .register_name = std::string{"a0"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .address_materializations =
          {
              prepare::PreparedAddressMaterialization{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .kind = kind,
                  .result_value_name = result_name,
                  .symbol_name = kind ==
                                           prepare::PreparedAddressMaterializationKind::
                                               DirectGlobal
                                       ? std::optional<c4c::LinkNameId>{link_name}
                                       : std::nullopt,
                  .text_name = kind ==
                                        prepare::PreparedAddressMaterializationKind::
                                            StringConstant
                                    ? std::optional<c4c::TextId>{text_name}
                                    : std::nullopt,
                  .address_materialization_policy =
                      kind == prepare::PreparedAddressMaterializationKind::DirectGlobal
                          ? bir::GlobalAddressMaterializationPolicy::Direct
                          : bir::GlobalAddressMaterializationPolicy::Unspecified,
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_string_address_module() {
  auto prepared = make_prepared_symbol_address_module(
      prepare::PreparedAddressMaterializationKind::StringConstant,
      ".LC0");
  const auto text_name = prepared.module.names.texts.intern(".LC0");
  prepared.module.string_constants.push_back(bir::StringConstant{
      .name = ".LC0",
      .name_id = text_name,
      .bytes = "abc",
      .align_bytes = 1,
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_string_call_argument_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto text_name = prepared.names.texts.intern(".LC0");

  bir::CallInst call;
  call.callee = "sink";
  call.args = {bir::Value::named(bir::TypeKind::Ptr, "@.LC0")};
  call.arg_types = {bir::TypeKind::Ptr};
  call.return_type = bir::TypeKind::Void;
  bir::Block entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.module.string_constants.push_back(bir::StringConstant{
      .name = ".LC0",
      .name_id = text_name,
      .bytes = "abc",
      .align_bytes = 1,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"sink"},
          .arguments = {prepare::PreparedCallArgumentPlan{
              .instruction_index = 0,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
              .source_symbol_name = std::string{"@.LC0"},
              .destination_register_name = std::string{"a0"},
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          }},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_global_address_module() {
  auto prepared = make_prepared_symbol_address_module(
      prepare::PreparedAddressMaterializationKind::DirectGlobal,
      "global_i32");
  const auto link_name = prepared.module.names.link_names.intern("global_i32");
  prepared.module.globals.push_back(bir::Global{
      .name = "global_i32",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .is_constant = true,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(7),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  publish_prepared_object_data(prepared);
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

prepare::PreparedBirModule make_prepared_wide_rematerialized_return_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto base_name = prepared.names.value_names.intern("%wide.base");
  const auto min_name = prepared.names.value_names.intern("%wide.min");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Sub,
                  .result = bir::Value::named(bir::TypeKind::I32, "%wide.base"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::immediate_i32(0),
                  .rhs = bir::Value::immediate_i32(2147483647),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Sub,
                  .result = bir::Value::named(bir::TypeKind::I32, "%wide.min"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%wide.base"),
                  .rhs = bir::Value::immediate_i32(1),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%wide.min");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
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
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = base_name,
                  .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
                  .immediate_i32 = -2147483647,
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = min_name,
                  .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
                  .immediate_i32 = std::numeric_limits<std::int32_t>::min(),
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

prepare::PreparedBirModule make_prepared_byval_stack_copy_same_module_call_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto callee_name = prepared.names.function_names.intern("consume_pair");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto payload_name = prepared.names.value_names.intern("%lv.pair");
  const auto result_name = prepared.names.value_names.intern("%main.t0");
  const auto source_slot_id = prepare::PreparedFrameSlotId{7};
  const auto source_object_id = prepare::PreparedObjectId{9};

  bir::Block callee_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  callee_entry.terminator.value = bir::Value::immediate_i32(5);

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%main.t0");
  call.callee = "consume_pair";
  call.args = {bir::Value::named(bir::TypeKind::Ptr, "%lv.pair"),
               bir::Value::immediate_i32(19)};
  call.arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32};
  call.arg_abi = {
      bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 24,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
      bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  };
  call.return_type = bir::TypeKind::I32;
  bir::Block main_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%main.t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "consume_pair",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
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
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = source_object_id,
      .function_name = main_name,
      .value_name = payload_name,
      .source_kind = "local",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 24,
      .align_bytes = 8,
      .address_exposed = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = source_slot_id,
      .object_id = source_object_id,
      .function_name = main_name,
      .offset_bytes = 96,
      .size_bytes = 24,
      .align_bytes = 8,
  });
  prepared.stack_layout.frame_size_bytes = 128;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = main_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = main_name,
                  .value_name = payload_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = source_slot_id,
                  .offset_bytes = std::size_t{96},
                  .size_bytes = std::size_t{24},
                  .align_bytes = std::size_t{8},
              },
              prepare::PreparedValueHome{
                  .value_id = 4,
                  .function_name = main_name,
                  .value_name = result_name,
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
          .direct_callee_name = std::string{"consume_pair"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::AggregateAddress,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{3},
                      .source_register_name = std::string{"s2"},
                      .source_slot_id = source_slot_id,
                      .source_stack_offset_bytes = std::size_t{96},
                      .source_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::PreparedCallArgumentSourceSelectionKind::
                                  LocalFrameAddressMaterialization,
                              .source_value_id = prepare::PreparedValueId{3},
                              .source_value_name = payload_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::Register,
                              .source_slot_id = source_slot_id,
                              .source_stack_offset_bytes = std::size_t{96},
                              .source_size_bytes = std::size_t{24},
                              .source_align_bytes = std::size_t{8},
                              .source_pointer_byte_delta = std::int64_t{0},
                              .address_materialization_block_label =
                                  c4c::BlockLabelId{1},
                              .address_materialization_inst_index = std::size_t{0},
                              .address_materialization_frame_slot_id = source_slot_id,
                              .address_materialization_byte_offset = std::int64_t{96},
                          },
                      .aggregate_transport =
                          prepare::PreparedAggregateTransportPlan{
                              .kind = prepare::PreparedAggregateTransportKind::StackCopy,
                              .payload_size_bytes = 24,
                              .payload_align_bytes = 8,
                              .copy_size_bytes = 24,
                              .copy_align_bytes = 8,
                              .source_slot_id = source_slot_id,
                              .source_stack_offset_bytes = std::size_t{96},
                              .chunks =
                                  {
                                      prepare::PreparedAggregateTransportChunk{
                                          .chunk_index = 0,
                                          .kind = prepare::PreparedAggregateTransportChunkKind::
                                              RequiredPayload,
                                          .payload_offset_bytes = 0,
                                          .source_offset_bytes = 96,
                                          .destination_offset_bytes = 0,
                                          .size_bytes = 8,
                                          .align_bytes = 8,
                                          .preferred_width_bytes = std::size_t{8},
                                      },
                                      prepare::PreparedAggregateTransportChunk{
                                          .chunk_index = 1,
                                          .kind = prepare::PreparedAggregateTransportChunkKind::
                                              RequiredPayload,
                                          .payload_offset_bytes = 8,
                                          .source_offset_bytes = 104,
                                          .destination_offset_bytes = 8,
                                          .size_bytes = 8,
                                          .align_bytes = 8,
                                          .preferred_width_bytes = std::size_t{8},
                                      },
                                      prepare::PreparedAggregateTransportChunk{
                                          .chunk_index = 2,
                                          .kind = prepare::PreparedAggregateTransportChunkKind::
                                              RequiredPayload,
                                          .payload_offset_bytes = 16,
                                          .source_offset_bytes = 112,
                                          .destination_offset_bytes = 16,
                                          .size_bytes = 8,
                                          .align_bytes = 8,
                                          .preferred_width_bytes = std::size_t{8},
                                      },
                                  },
                              .scratch_requirements =
                                  {prepare::PreparedAggregateTransportScratchRequirement{
                                      .kind = prepare::PreparedAggregateTransportScratchKind::
                                          GeneralPurpose,
                                      .width_bytes = 8,
                                  }},
                          },
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
                      .source_literal = bir::Value::immediate_i32(19),
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

prepare::PreparedValueHome make_fpr_home(c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         prepare::PreparedValueId value_id,
                                         std::string register_name,
                                         std::size_t physical_index);

prepare::PreparedBirModule make_prepared_fpr_same_module_call_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("fpr_call");
  const auto arg_name = prepared.names.value_names.intern("%arg");
  const auto result_name = prepared.names.value_names.intern("%result");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::F64, "%result");
  call.callee = "sin";
  call.args = {bir::Value::named(bir::TypeKind::F64, "%arg")};
  call.arg_types = {bir::TypeKind::F64};
  call.return_type = bir::TypeKind::F64;
  bir::Block entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "fpr_call",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, arg_name, 1, "ft0", 0),
              make_fpr_home(function_name, result_name, 2, "fs1", 9),
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"sin"},
          .arguments = {prepare::PreparedCallArgumentPlan{
              .instruction_index = 0,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Fpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::Register,
              .source_value_id = prepare::PreparedValueId{1},
              .source_register_name = std::string{"ft0"},
              .source_register_bank = prepare::PreparedRegisterBank::Fpr,
              .destination_register_name = std::string{"fa0"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Fpr,
              .destination_register_placement =
                  prepare::PreparedRegisterPlacement{
                      .bank = prepare::PreparedRegisterBank::Fpr,
                      .pool = prepare::PreparedRegisterSlotPool::CallArgument,
                      .slot_index = 0,
                      .contiguous_width = 1,
                  },
          }},
          .result = prepare::PreparedCallResultPlan{
              .instruction_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Fpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_value_id = 2,
              .source_register_name = std::string{"fa0"},
              .source_contiguous_width = 1,
              .source_register_bank = prepare::PreparedRegisterBank::Fpr,
              .destination_register_name = std::string{"fs1"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Fpr,
              .source_register_placement =
                  prepare::PreparedRegisterPlacement{
                      .bank = prepare::PreparedRegisterBank::Fpr,
                      .pool = prepare::PreparedRegisterSlotPool::CallResult,
                      .slot_index = 0,
                      .contiguous_width = 1,
                  },
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

prepare::PreparedCallArgumentSourceSelection prior_preserved_s1_selection(
    c4c::ValueNameId source_name) {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation,
      .source_value_id = prepare::PreparedValueId{1},
      .source_value_name = source_name,
      .preserved_call_block_index = std::size_t{0},
      .preserved_call_instruction_index = std::size_t{0},
      .preservation_route =
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
      .preserved_register_name = std::string{"s1"},
      .preserved_register_bank = prepare::PreparedRegisterBank::Gpr,
      .preserved_register_contiguous_width = std::size_t{1},
      .preserved_occupied_register_names = {std::string{"s1"}},
      .preserved_register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
              .slot_index = 1,
              .contiguous_width = 1,
          },
      .preserved_callee_saved_save_index = std::size_t{0},
  };
}

prepare::PreparedBirModule make_prepared_prior_preserved_arg_call_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("reload_prior_preserved_arg");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto param_name = prepared.names.value_names.intern("%p.ptr");
  const auto first_result_name = prepared.names.value_names.intern("%first");
  const auto second_result_name = prepared.names.value_names.intern("%second");

  bir::CallInst first_call;
  first_call.result = bir::Value::named(bir::TypeKind::Ptr, "%first");
  first_call.callee = "probe";
  first_call.args = {bir::Value::named(bir::TypeKind::Ptr, "%p.ptr")};
  first_call.arg_types = {bir::TypeKind::Ptr};
  first_call.return_type = bir::TypeKind::Ptr;

  bir::CallInst second_call;
  second_call.result = bir::Value::named(bir::TypeKind::Ptr, "%second");
  second_call.callee = "probe";
  second_call.args = {bir::Value::named(bir::TypeKind::Ptr, "%p.ptr")};
  second_call.arg_types = {bir::TypeKind::Ptr};
  second_call.return_type = bir::TypeKind::Ptr;

  bir::Block entry{
      .label = "entry",
      .insts = {first_call, second_call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::Ptr, "%second");

  prepared.module.functions.push_back(bir::Function{
      .name = "reload_prior_preserved_arg",
      .return_type = bir::TypeKind::Ptr,
      .return_size_bytes = 8,
      .return_align_bytes = 8,
      .params = {bir::Param{
          .type = bir::TypeKind::Ptr,
          .name = "%p.ptr",
          .size_bytes = 8,
          .align_bytes = 8,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = param_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = first_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = function_name,
                  .value_name = second_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 0,
                  .wrapper_kind =
                      prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .direct_callee_name = std::string{"probe"},
                  .arguments = {prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_register_name = std::string{"s1"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  }},
                  .result = prepare::PreparedCallResultPlan{
                      .instruction_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_value_id = prepare::PreparedValueId{2},
                      .source_register_name = std::string{"a0"},
                      .source_contiguous_width = 1,
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  },
                  .preserved_values = {prepare::PreparedCallPreservedValue{
                      .value_id = prepare::PreparedValueId{1},
                      .value_name = param_name,
                      .route = prepare::PreparedCallPreservationRoute::
                          CalleeSavedRegister,
                      .callee_saved_save_index = std::size_t{0},
                      .contiguous_width = 1,
                      .register_name = std::string{"s1"},
                      .register_bank = prepare::PreparedRegisterBank::Gpr,
                      .occupied_register_names = {std::string{"s1"}},
                      .register_placement =
                          prepare::PreparedRegisterPlacement{
                              .bank = prepare::PreparedRegisterBank::Gpr,
                              .pool =
                                  prepare::PreparedRegisterSlotPool::CalleeSaved,
                              .slot_index = 1,
                              .contiguous_width = 1,
                          },
                      .preservation_source =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::Register,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .value_id = prepare::PreparedValueId{1},
                              .value_name = param_name,
                              .register_name = std::string{"a0"},
                              .register_bank = prepare::PreparedRegisterBank::Gpr,
                              .contiguous_width = 1,
                              .occupied_register_names = {std::string{"a0"}},
                          },
                      .preservation_destination =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::Register,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .value_id = prepare::PreparedValueId{1},
                              .value_name = param_name,
                              .register_name = std::string{"s1"},
                              .register_bank = prepare::PreparedRegisterBank::Gpr,
                              .contiguous_width = 1,
                              .occupied_register_names = {std::string{"s1"}},
                              .callee_saved_save_index = std::size_t{0},
                              .register_placement =
                                  prepare::PreparedRegisterPlacement{
                                      .bank = prepare::PreparedRegisterBank::Gpr,
                                      .pool = prepare::PreparedRegisterSlotPool::
                                          CalleeSaved,
                                      .slot_index = 1,
                                      .contiguous_width = 1,
                                  },
                          },
                  }},
              },
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 1,
                  .wrapper_kind =
                      prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .direct_callee_name = std::string{"probe"},
                  .arguments = {prepare::PreparedCallArgumentPlan{
                      .instruction_index = 1,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_register_name = std::string{"a0"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection = prior_preserved_s1_selection(param_name),
                  }},
                  .result = prepare::PreparedCallResultPlan{
                      .instruction_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_value_id = prepare::PreparedValueId{3},
                      .source_register_name = std::string{"a0"},
                      .source_contiguous_width = 1,
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  },
              },
          },
  });
  const prepare::PreparedRegisterPlacement s1_placement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
      .slot_index = 1,
      .contiguous_width = 1,
  };
  const prepare::PreparedSavedRegisterSlotPlacement s1_slot{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "s1",
      .contiguous_width = 1,
      .occupied_register_names = {"s1"},
      .save_index = 0,
      .register_placement = s1_placement,
      .slot_id = prepare::PreparedFrameSlotId{20},
      .stack_offset_bytes = std::size_t{0},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
      .fixed_location = true,
  };
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 16,
      .saved_callee_registers =
          {prepare::PreparedSavedRegister{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = "s1",
              .contiguous_width = 1,
              .occupied_register_names = {"s1"},
              .save_index = 0,
              .placement = s1_placement,
              .slot_placement = s1_slot,
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

prepare::PreparedBirModule
make_prepared_frame_slot_address_local_store_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto ptr_slot_name = prepared.names.slot_names.intern("%lv.ptr");
  const auto pointee_slot_name = prepared.names.slot_names.intern("%lv.target");
  const auto address_name = prepared.names.value_names.intern("%addr");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.ptr",
                  .slot_id = ptr_slot_name,
                  .value = bir::Value::named(bir::TypeKind::Ptr, "%addr"),
                  .align_bytes = 8,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
                          .name = "%lv.ptr",
                          .slot_id = ptr_slot_name,
                          .type = bir::TypeKind::Ptr,
                          .size_bytes = 8,
                          .align_bytes = 8,
                      },
                      bir::LocalSlot{
                          .name = "%lv.target",
                          .slot_id = pointee_slot_name,
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
                  .value_name = address_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 8,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{1},
                      .byte_offset = 0,
                      .size_bytes = 8,
                      .align_bytes = 8,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
      .address_materializations =
          {
              prepare::PreparedAddressMaterialization{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
                  .result_value_name = address_name,
                  .result_value_id = prepare::PreparedValueId{1},
                  .result_home_kind = prepare::PreparedValueHomeKind::Register,
                  .frame_slot_id = prepare::PreparedFrameSlotId{0},
                  .byte_offset = 24,
              },
          },
  });
  prepared.stack_layout.frame_size_bytes = 32;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 24,
          .size_bytes = 4,
          .align_bytes = 4,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{1},
          .function_name = function_name,
          .offset_bytes = 8,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  };
  prepared.frame_plan.functions = {
      prepare::PreparedFramePlanFunction{
          .function_name = function_name,
          .frame_size_bytes = 32,
          .frame_alignment_bytes = 8,
          .frame_slot_order = {prepare::PreparedFrameSlotId{0},
                               prepare::PreparedFrameSlotId{1}},
      },
  };
  return prepared;
}

prepare::PreparedBirModule make_prepared_f64_local_frame_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("f64_local_frame");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.d");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.d",
                  .slot_id = slot_name,
                  .value = bir::Value::immediate_f64_bits(0x40091eb851eb851full),
                  .align_bytes = 8,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::F64, "%t0"),
                  .slot_name = "%lv.d",
                  .slot_id = slot_name,
                  .align_bytes = 8,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "f64_local_frame",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
          .name = "%lv.d",
          .slot_id = slot_name,
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, result_name, 1, "ft0", 0),
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 8,
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
                      .size_bytes = 8,
                      .align_bytes = 8,
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
                      .size_bytes = 8,
                      .align_bytes = 8,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_f32_local_frame_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("f32_local_frame");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.f");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.f",
                  .slot_id = slot_name,
                  .value = bir::Value::immediate_f32_bits(0x3f800000u),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::F32, "%t0"),
                  .slot_name = "%lv.f",
                  .slot_id = slot_name,
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "f32_local_frame",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
          .name = "%lv.f",
          .slot_id = slot_name,
          .type = bir::TypeKind::F32,
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
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, result_name, 1, "ft0", 0),
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
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

prepare::PreparedBirModule make_prepared_f32_i32_local_overlay_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("f32_i32_overlay");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto u2f_slot_name = prepared.names.slot_names.intern("%lv.u2f");
  const auto f2u_slot_name = prepared.names.slot_names.intern("%lv.f2u");
  const auto loaded_f32_name = prepared.names.value_names.intern("%loaded.f32");
  const auto loaded_i32_name = prepared.names.value_names.intern("%loaded.i32");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.u2f",
                  .slot_id = u2f_slot_name,
                  .value = bir::Value::immediate_i32(0x3f800000),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::F32, "%loaded.f32"),
                  .slot_name = "%lv.u2f",
                  .slot_id = u2f_slot_name,
                  .align_bytes = 4,
              },
              bir::StoreLocalInst{
                  .slot_name = "%lv.f2u",
                  .slot_id = f2u_slot_name,
                  .value = bir::Value::immediate_f32_bits(0x40000000u),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%loaded.i32"),
                  .slot_name = "%lv.f2u",
                  .slot_id = f2u_slot_name,
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "f32_i32_overlay",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
                          .name = "%lv.u2f",
                          .slot_id = u2f_slot_name,
                          .type = bir::TypeKind::I32,
                          .size_bytes = 4,
                          .align_bytes = 4,
                      },
                      bir::LocalSlot{
                          .name = "%lv.f2u",
                          .slot_id = f2u_slot_name,
                          .type = bir::TypeKind::F32,
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
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 4,
          .align_bytes = 4,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{1},
          .function_name = function_name,
          .offset_bytes = 4,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, loaded_f32_name, 1, "ft1", 1),
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = loaded_i32_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
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
                  .result_value_name = loaded_f32_name,
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
                  .inst_index = 2,
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
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 3,
                  .result_value_name = loaded_i32_name,
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
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_local_subobject_frame_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.member");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.member",
                  .slot_id = slot_name,
                  .value = bir::Value::immediate_i32(5),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .slot_name = "%lv.member",
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
          .name = "%lv.member",
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
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 8,
          .align_bytes = 8,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{1},
          .function_name = function_name,
          .offset_bytes = 8,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  };
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
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 8,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{1},
                      .byte_offset = 4,
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
                      .frame_slot_id = prepare::PreparedFrameSlotId{1},
                      .byte_offset = 4,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_pointer_value_scalar_local_module(
    std::string pointer_register = "t2") {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%ptr");
  const auto pointer_name = prepared.names.value_names.intern("%p");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%ptr",
                  .slot_id = slot_name,
                  .value = bir::Value::immediate_i16(9),
                  .align_bytes = 2,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I16, "%t0"),
                  .slot_name = "%ptr",
                  .slot_id = slot_name,
                  .align_bytes = 2,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I16, "%t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I16,
      .return_size_bytes = 2,
      .return_align_bytes = 2,
      .local_slots = {bir::LocalSlot{
          .name = "%ptr",
          .slot_id = slot_name,
          .type = bir::TypeKind::I16,
          .size_bytes = 2,
          .align_bytes = 2,
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
                  .value_name = pointer_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::move(pointer_register),
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 1,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                      .pointer_value_name = pointer_name,
                      .byte_offset = 2,
                      .size_bytes = 2,
                      .align_bytes = 2,
                      .can_use_base_plus_offset = true,
                  },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .result_value_name = result_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                      .pointer_value_name = pointer_name,
                      .byte_offset = 2,
                      .size_bytes = 2,
                      .align_bytes = 2,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_pointer_value_f64_local_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("f64_pointer_local");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%ptr");
  const auto pointer_name = prepared.names.value_names.intern("%p");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto result_name = prepared.names.value_names.intern("%dst");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%ptr",
                  .slot_id = slot_name,
                  .value = bir::Value::named(bir::TypeKind::F64, "%src"),
                  .align_bytes = 8,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::F64, "%dst"),
                  .slot_name = "%ptr",
                  .slot_id = slot_name,
                  .align_bytes = 8,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "f64_pointer_local",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
          .name = "%ptr",
          .slot_id = slot_name,
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
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
                  .value_name = pointer_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
              },
              make_fpr_home(function_name, source_name, 2, "ft0", 0),
              make_fpr_home(function_name, result_name, 3, "ft1", 1),
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 1,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                      .pointer_value_name = pointer_name,
                      .byte_offset = 0,
                      .size_bytes = 8,
                      .align_bytes = 8,
                      .can_use_base_plus_offset = true,
                  },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .result_value_name = result_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                      .pointer_value_name = pointer_name,
                      .byte_offset = 0,
                      .size_bytes = 8,
                      .align_bytes = 8,
                      .can_use_base_plus_offset = true,
                  },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_sret_stack_pointer_store_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("sret_stack_pointer_store");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%ret.sret");
  const auto pointer_name = prepared.names.value_names.intern("%ret.sret");
  const auto object_id = prepare::PreparedObjectId{31};
  const auto slot_id = prepare::PreparedFrameSlotId{19};

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.result",
                  .value = bir::Value::immediate_i32(42),
                  .align_bytes = 4,
                  .address =
                      bir::MemoryAddress{
                          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                          .base_name = "%ret.sret",
                          .base_slot_id = slot_name,
                      },
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "sret_stack_pointer_store",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = bir::TypeKind::Ptr,
          .name = "%ret.sret",
          .size_bytes = 8,
          .align_bytes = 8,
          .abi = bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 8,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
          .is_sret = true,
      }},
      .local_slots = {bir::LocalSlot{
          .name = "%ret.sret",
          .slot_id = slot_name,
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = object_id,
      .function_name = function_name,
      .value_name = pointer_name,
      .source_kind = "sret_param",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = object_id,
      .function_name = function_name,
      .offset_bytes = 0,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  prepared.stack_layout.frame_size_bytes = 16;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 8,
      .frame_slot_order = {slot_id},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = function_name,
          .value_name = pointer_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = slot_id,
          .offset_bytes = std::size_t{0},
          .size_bytes = std::size_t{8},
          .align_bytes = std::size_t{8},
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 8,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
              .pointer_value_name = pointer_name,
              .byte_offset = 4,
              .size_bytes = 4,
              .align_bytes = 4,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_i16_local_store_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.half");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%lv.half",
                  .slot_id = slot_name,
                  .value = bir::Value::immediate_i16(7),
                  .align_bytes = 2,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .local_slots = {bir::LocalSlot{
          .name = "%lv.half",
          .slot_id = slot_name,
          .type = bir::TypeKind::I16,
          .size_bytes = 2,
          .align_bytes = 2,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 2,
      .frame_alignment_bytes = 2,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = prepare::PreparedFrameSlotId{0},
              .byte_offset = 0,
              .size_bytes = 2,
              .align_bytes = 2,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_stack_result_call_module() {
  auto prepared = make_prepared_scalar_same_module_call_module();
  const auto main_name = prepared.names.function_names.intern("main");

  auto& main = prepared.module.functions[1];
  main.return_type = bir::TypeKind::Void;
  main.return_size_bytes = 0;
  main.return_align_bytes = 1;
  auto& call = std::get<bir::CallInst>(main.blocks[0].insts[0]);
  call.result->type = bir::TypeKind::I16;
  call.arg_types[0] = bir::TypeKind::I16;
  call.return_type = bir::TypeKind::I16;
  main.blocks[0].terminator.value = std::nullopt;

  auto& result_home = prepared.value_locations.functions[1].value_homes[0];
  result_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  result_home.register_name = std::nullopt;
  result_home.slot_id = prepare::PreparedFrameSlotId{12};
  result_home.offset_bytes = 4;
  result_home.size_bytes = 2;
  result_home.align_bytes = 2;

  auto& result = *prepared.call_plans.functions[0].calls[0].result;
  result.value_bank = prepare::PreparedRegisterBank::Gpr;
  result.destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  result.destination_register_name = std::nullopt;
  result.destination_register_bank = std::nullopt;
  result.destination_slot_id = prepare::PreparedFrameSlotId{12};
  result.destination_stack_offset_bytes = 4;
  result.destination_contiguous_width = 1;

  prepared.frame_plan.functions = {
      prepare::PreparedFramePlanFunction{
          .function_name = prepared.names.function_names.intern("add_three"),
          .frame_size_bytes = 0,
          .frame_alignment_bytes = 1,
      },
      prepare::PreparedFramePlanFunction{
          .function_name = main_name,
          .frame_size_bytes = 16,
          .frame_alignment_bytes = 4,
          .frame_slot_order = {prepare::PreparedFrameSlotId{12}},
      },
  };
  prepared.stack_layout.frame_size_bytes = 16;
  prepared.stack_layout.frame_alignment_bytes = 4;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{12},
          .function_name = main_name,
          .offset_bytes = 4,
          .size_bytes = 2,
          .align_bytes = 2,
      },
  };
  return prepared;
}

prepare::PreparedValueHome make_fpr_home(c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         prepare::PreparedValueId value_id,
                                         std::string register_name,
                                         std::size_t physical_index) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::move(register_name),
      .target_register_identity =
          prepare::PreparedTargetRegisterIdentity{
              .target_arch = c4c::TargetArch::Riscv64,
              .bank = prepare::PreparedRegisterBank::Fpr,
              .register_class = prepare::PreparedRegisterClass::Float,
              .physical_index = physical_index,
          },
  };
}

prepare::PreparedBirModule make_prepared_fpr_cast_module(const char* function,
                                                         bir::CastOpcode opcode,
                                                         bir::TypeKind source_type,
                                                         bir::TypeKind result_type) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern(function);
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto result_name = prepared.names.value_names.intern("%dst");

  bir::CastInst cast;
  cast.opcode = opcode;
  cast.operand = bir::Value::named(source_type, "%src");
  cast.result = bir::Value::named(result_type, "%dst");
  bir::Block entry{
      .label = "entry",
      .insts = {cast},
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = function,
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, source_name, 1, "fa0", 10),
              make_fpr_home(function_name, result_name, 2, "fa1", 11),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_fpr_fpext_module() {
  return make_prepared_fpr_cast_module("fpr_fpext",
                                       bir::CastOpcode::FPExt,
                                       bir::TypeKind::F32,
                                       bir::TypeKind::F64);
}

prepare::PreparedBirModule make_prepared_fpr_fptrunc_module() {
  return make_prepared_fpr_cast_module("fpr_fptrunc",
                                       bir::CastOpcode::FPTrunc,
                                       bir::TypeKind::F64,
                                       bir::TypeKind::F32);
}

prepare::PreparedBirModule make_prepared_formal_fpr_fpext_to_ft0_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("formal_fpr_fpext_to_ft0");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%p.a");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::CastInst cast;
  cast.opcode = bir::CastOpcode::FPExt;
  cast.operand = bir::Value::named(bir::TypeKind::F32, "%p.a");
  cast.result = bir::Value::named(bir::TypeKind::F64, "%t0");
  bir::Block entry{
      .label = "entry",
      .insts = {cast},
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "formal_fpr_fpext_to_ft0",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = bir::TypeKind::F32,
          .name = "%p.a",
          .size_bytes = 4,
          .align_bytes = 4,
          .abi = bir::CallArgAbiInfo{
              .type = bir::TypeKind::F32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Sse,
              .passed_in_register = true,
          },
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, source_name, 1, "fa0", 10),
              make_fpr_home(function_name, result_name, 2, "ft0", 0),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_sitofp_i32_immediate_to_f64_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("sitofp_i32_immediate_to_f64");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%t0");

  bir::CastInst cast;
  cast.opcode = bir::CastOpcode::SIToFP;
  cast.operand = bir::Value::immediate_i32(0);
  cast.result = bir::Value::named(bir::TypeKind::F64, "%t0");
  bir::Block entry{
      .label = "entry",
      .insts = {cast},
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "sitofp_i32_immediate_to_f64",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, result_name, 1, "ft0", 0),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_unsupported_floating_cast_module() {
  return make_prepared_fpr_cast_module("unsupported_floating_cast",
                                       bir::CastOpcode::FPExt,
                                       bir::TypeKind::F64,
                                       bir::TypeKind::F64);
}

prepare::PreparedBirModule make_prepared_before_return_fpr_abi_move_module(
    bir::TypeKind return_type) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("fpr_return_move");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%ret");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::named(return_type, "%ret");
  const std::size_t return_size =
      return_type == bir::TypeKind::F64 ? 8
      : return_type == bir::TypeKind::F128 ? 16
                                           : 4;
  prepared.module.functions.push_back(bir::Function{
      .name = "fpr_return_move",
      .return_type = return_type,
      .return_size_bytes = return_size,
      .return_align_bytes = return_size,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_fpr_home(function_name, result_name, 1, "ft0", 0),
          },
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeReturn,
              .block_index = 0,
              .instruction_index = 0,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 1,
                  .to_value_id = 1,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"fa0"},
                  .destination_contiguous_width = 1,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .destination_register_placement =
                      prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Fpr,
                          .pool = prepare::PreparedRegisterSlotPool::CallResult,
                          .slot_index = 0,
                          .contiguous_width = 1,
                      },
              }},
          }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_before_return_fpr_f32_abi_move_module() {
  return make_prepared_before_return_fpr_abi_move_module(bir::TypeKind::F32);
}

prepare::PreparedBirModule make_prepared_before_return_fpr_f64_abi_move_module() {
  return make_prepared_before_return_fpr_abi_move_module(bir::TypeKind::F64);
}

prepare::PreparedBirModule make_prepared_before_return_fpr_f128_abi_move_module() {
  return make_prepared_before_return_fpr_abi_move_module(bir::TypeKind::F128);
}

prepare::PreparedBirModule make_prepared_fpr_immediate_return_module(
    bir::TypeKind return_type,
    std::uint64_t immediate_bits) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("fpr_immediate_return");
  const auto block_label = prepared.names.block_labels.intern("entry");

  bir::Value return_value = bir::Value::immediate_f128_bits(immediate_bits, 0);
  std::size_t return_size = 16;
  if (return_type == bir::TypeKind::F32) {
    return_value =
        bir::Value::immediate_f32_bits(static_cast<std::uint32_t>(immediate_bits));
    return_size = 4;
  } else if (return_type == bir::TypeKind::F64) {
    return_value = bir::Value::immediate_f64_bits(immediate_bits);
    return_size = 8;
  }

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = return_value;
  prepared.module.functions.push_back(bir::Function{
      .name = "fpr_immediate_return",
      .return_type = return_type,
      .return_size_bytes = return_size,
      .return_align_bytes = return_size,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_global_load_module(bool publish_access = true) {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%g");
  const auto global_name = prepared.names.link_names.intern("g");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadGlobalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%g"),
                  .global_name = "g",
                  .global_name_id = global_name,
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%g");

  prepared.module.globals.push_back(bir::Global{
      .name = "g",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .is_constant = true,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(9),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = function_name,
          .value_name = result_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a0"},
      }},
  });
  if (publish_access) {
    prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
        .function_name = function_name,
        .accesses = {prepare::PreparedMemoryAccess{
            .function_name = function_name,
            .block_label = block_label,
            .inst_index = 0,
            .result_value_name = result_name,
            .address = prepare::PreparedAddress{
                .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                .symbol_name = global_name,
                .global_address_materialization_policy =
                    bir::GlobalAddressMaterializationPolicy::Direct,
                .byte_offset = 0,
                .size_bytes = 4,
                .align_bytes = 4,
                .can_use_base_plus_offset = true,
            },
        }},
    });
  }
  publish_prepared_object_data(prepared);
  return prepared;
}

prepare::PreparedBirModule make_prepared_global_i8_zext_load_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto load_name = prepared.names.value_names.intern("%byte");
  const auto zext_name = prepared.names.value_names.intern("%wide");
  const auto global_name = prepared.names.link_names.intern("bytes");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadGlobalInst{
                  .result = bir::Value::named(bir::TypeKind::I8, "%byte"),
                  .global_name = "bytes",
                  .global_name_id = global_name,
                  .byte_offset = 1,
                  .align_bytes = 1,
              },
              bir::CastInst{
                  .opcode = bir::CastOpcode::ZExt,
                  .result = bir::Value::named(bir::TypeKind::I32, "%wide"),
                  .operand = bir::Value::named(bir::TypeKind::I8, "%byte"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%wide");

  prepared.module.globals.push_back(bir::Global{
      .name = "bytes",
      .link_name_id = global_name,
      .type = bir::TypeKind::I8,
      .is_constant = true,
      .size_bytes = 3,
      .align_bytes = 1,
      .initializer_elements =
          {
              bir::Value::immediate_i8(0),
              bir::Value::immediate_i8(static_cast<std::int8_t>(0xff)),
              bir::Value::immediate_i8(0),
          },
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = load_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = zext_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .result_value_name = load_name,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
              .symbol_name = global_name,
              .global_address_materialization_policy =
                  bir::GlobalAddressMaterializationPolicy::Direct,
              .byte_offset = 1,
              .size_bytes = 1,
              .align_bytes = 1,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  publish_prepared_object_data(prepared);
  return prepared;
}

prepare::PreparedBirModule make_prepared_global_aggregate_lane_load_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%lane");
  const auto global_name = prepared.names.link_names.intern("aggregate_lanes");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadGlobalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%lane"),
                  .global_name = "aggregate_lanes",
                  .global_name_id = global_name,
                  .byte_offset = 68,
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%lane");

  bir::Global aggregate{
      .name = "aggregate_lanes",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .is_constant = true,
      .size_bytes = 72,
      .align_bytes = 4,
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  };
  for (std::int32_t lane = 0; lane != 18; ++lane) {
    aggregate.initializer_elements.push_back(bir::Value::immediate_i32(lane));
  }
  prepared.module.globals.push_back(std::move(aggregate));
  publish_prepared_object_data(prepared);
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = function_name,
          .value_name = result_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a0"},
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .result_value_name = result_name,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
              .symbol_name = global_name,
              .global_address_materialization_policy =
                  bir::GlobalAddressMaterializationPolicy::Direct,
              .byte_offset = 68,
              .size_bytes = 4,
              .align_bytes = 4,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_raw_global_address_load_local_lane_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%lane");
  const auto global_name = prepared.names.link_names.intern("aggregate_lanes");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%lane"),
                  .slot_name = "aggregate_lanes",
                  .byte_offset = 68,
                  .align_bytes = 4,
                  .address = bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                      .base_name = "aggregate_lanes",
                      .byte_offset = 68,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .base_link_name_id = global_name,
                  },
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%lane");

  bir::Global aggregate{
      .name = "aggregate_lanes",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .is_constant = true,
      .size_bytes = 72,
      .align_bytes = 4,
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  };
  for (std::int32_t lane = 0; lane != 18; ++lane) {
    aggregate.initializer_elements.push_back(bir::Value::immediate_i32(lane));
  }
  prepared.module.globals.push_back(std::move(aggregate));
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = function_name,
          .value_name = result_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a0"},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_same_width_integer_zext_module(
    bir::CastOpcode opcode = bir::CastOpcode::ZExt,
    bir::TypeKind operand_type = bir::TypeKind::I32,
    bir::TypeKind result_type = bir::TypeKind::I32,
    prepare::PreparedValueHomeKind source_home_kind =
        prepare::PreparedValueHomeKind::Register,
    prepare::PreparedValueHomeKind result_home_kind =
        prepare::PreparedValueHomeKind::Register) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("same_width_zext");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%in");
  const auto result_name = prepared.names.value_names.intern("%out");
  const std::size_t operand_size_bytes =
      operand_type == bir::TypeKind::I64 || operand_type == bir::TypeKind::Ptr
          ? 8
          : 4;

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::CastInst{
                  .opcode = opcode,
                  .result = bir::Value::named(result_type, "%out"),
                  .operand = bir::Value::named(operand_type, "%in"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "same_width_zext",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = operand_type,
          .name = "%in",
          .size_bytes = operand_size_bytes,
          .align_bytes = operand_size_bytes,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  prepare::PreparedValueHome source_home{
      .value_id = 1,
      .function_name = function_name,
      .value_name = source_name,
      .kind = source_home_kind,
      .register_name = std::string{"t0"},
  };
  if (source_home_kind == prepare::PreparedValueHomeKind::StackSlot) {
    source_home.register_name = std::nullopt;
    source_home.slot_id = prepare::PreparedFrameSlotId{0};
    source_home.offset_bytes = 0;
    source_home.size_bytes = 4;
    source_home.align_bytes = 4;
  }
  prepare::PreparedValueHome result_home{
      .value_id = 2,
      .function_name = function_name,
      .value_name = result_name,
      .kind = result_home_kind,
      .register_name = std::string{"s2"},
  };
  if (result_home_kind == prepare::PreparedValueHomeKind::StackSlot) {
    result_home.register_name = std::nullopt;
    result_home.slot_id = prepare::PreparedFrameSlotId{1};
    result_home.offset_bytes = 8;
    result_home.size_bytes = 4;
    result_home.align_bytes = 4;
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {std::move(source_home), std::move(result_home)},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_global_store_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto global_name = prepared.names.link_names.intern("g");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreGlobalInst{
                  .global_name = "g",
                  .global_name_id = global_name,
                  .value = bir::Value::immediate_i32(11),
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);

  prepared.module.globals.push_back(bir::Global{
      .name = "g",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(1),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
              .symbol_name = global_name,
              .global_address_materialization_policy =
                  bir::GlobalAddressMaterializationPolicy::Direct,
              .byte_offset = 0,
              .size_bytes = 4,
              .align_bytes = 4,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  publish_prepared_object_data(prepared);
  return prepared;
}

prepare::PreparedBirModule make_prepared_global_i16_store_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto global_name = prepared.names.link_names.intern("h");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::StoreGlobalInst{
                  .global_name = "h",
                  .global_name_id = global_name,
                  .value = bir::Value::immediate_i16(7),
                  .align_bytes = 2,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::immediate_i32(0);

  prepared.module.globals.push_back(bir::Global{
      .name = "h",
      .link_name_id = global_name,
      .type = bir::TypeKind::I16,
      .size_bytes = 2,
      .align_bytes = 2,
      .initializer = bir::Value::immediate_i16(1),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
              .symbol_name = global_name,
              .global_address_materialization_policy =
                  bir::GlobalAddressMaterializationPolicy::Direct,
              .byte_offset = 0,
              .size_bytes = 2,
              .align_bytes = 2,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  publish_prepared_object_data(prepared);
  return prepared;
}

prepare::PreparedValueHome rv64_stack_slot_home(
    prepare::PreparedValueId value_id,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t offset_bytes);

prepare::PreparedValueHome rv64_gpr_home(prepare::PreparedValueId value_id,
                                         c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         std::string register_name,
                                         std::size_t physical_index);

prepare::PreparedBirModule make_prepared_stack_slot_scalar_flow_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.x");
  const auto loaded_name = prepared.names.value_names.intern("%loaded");
  const auto sum_name = prepared.names.value_names.intern("%sum");

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
                  .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
                  .slot_name = "%lv.x",
                  .slot_id = slot_name,
                  .align_bytes = 4,
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%loaded"),
                  .rhs = bir::Value::immediate_i32(7),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");

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
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 4,
          .align_bytes = 4,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{1},
          .function_name = function_name,
          .offset_bytes = 4,
          .size_bytes = 4,
          .align_bytes = 4,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{2},
          .function_name = function_name,
          .offset_bytes = 8,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_stack_slot_home(1,
                                   function_name,
                                   loaded_name,
                                   prepare::PreparedFrameSlotId{1},
                                   4),
              rv64_stack_slot_home(2,
                                   function_name,
                                   sum_name,
                                   prepare::PreparedFrameSlotId{2},
                                   8),
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 12,
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
                  .result_value_name = loaded_name,
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

prepare::PreparedValueHome rv64_i16_stack_slot_home(
    prepare::PreparedValueId value_id,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t offset_bytes) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = slot_id,
      .offset_bytes = offset_bytes,
      .size_bytes = std::size_t{2},
      .align_bytes = std::size_t{2},
  };
}

prepare::PreparedBirModule make_prepared_scalar_compare_trunc_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto compare_name = prepared.names.value_names.intern("%cmp");
  const auto trunc_name = prepared.names.value_names.intern("%trunc");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Sge,
                  .result = bir::Value::named(bir::TypeKind::I32, "%cmp"),
                  .operand_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
                  .rhs = bir::Value::immediate_i32(8),
              },
              bir::CastInst{
                  .opcode = bir::CastOpcode::Trunc,
                  .result = bir::Value::named(bir::TypeKind::I16, "%trunc"),
                  .operand = bir::Value::named(bir::TypeKind::I32, "%cmp"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I16, "%trunc");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I16,
      .return_size_bytes = 2,
      .return_align_bytes = 2,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.stack_layout.frame_size_bytes = 24;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{11},
          .function_name = function_name,
          .offset_bytes = 20,
          .size_bytes = 2,
          .align_bytes = 2,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, lhs_name, "t0", 5),
              rv64_gpr_home(2, function_name, compare_name, "s1", 9),
              rv64_i16_stack_slot_home(3,
                                       function_name,
                                       trunc_name,
                                       prepare::PreparedFrameSlotId{11},
                                       20),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_ashr_module(
    bir::TypeKind type,
    bool immediate_shift) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto result_name = prepared.names.value_names.intern("%result");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto rhs =
      immediate_shift
          ? (type == bir::TypeKind::I32 ? bir::Value::immediate_i32(31)
                                        : bir::Value::immediate_i64(31))
          : bir::Value::named(type, "%rhs");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::AShr,
                  .result = bir::Value::named(type, "%result"),
                  .operand_type = type,
                  .lhs = bir::Value::named(type, "%lhs"),
                  .rhs = rhs,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(type, "%result");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = type,
      .return_size_bytes = type == bir::TypeKind::I32 ? std::size_t{4}
                                                      : std::size_t{8},
      .return_align_bytes = type == bir::TypeKind::I32 ? std::size_t{4}
                                                       : std::size_t{8},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  auto homes = std::vector<prepare::PreparedValueHome>{
      rv64_gpr_home(1, function_name, lhs_name, "t0", 5),
      rv64_gpr_home(2, function_name, result_name, "s2", 18),
  };
  if (!immediate_shift) {
    homes.push_back(rv64_gpr_home(3, function_name, rhs_name, "s1", 9));
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = std::move(homes),
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_binary_module(
    bir::BinaryOpcode opcode,
    bir::TypeKind type) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto result_name = prepared.names.value_names.intern("%result");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = opcode,
                  .result = bir::Value::named(type, "%result"),
                  .operand_type = type,
                  .lhs = bir::Value::named(type, "%lhs"),
                  .rhs = bir::Value::named(type, "%rhs"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(type, "%result");

  const std::size_t size_bytes = type == bir::TypeKind::I32 ? 4 : 8;
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = type,
      .return_size_bytes = size_bytes,
      .return_align_bytes = size_bytes,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, lhs_name, "t0", 5),
              rv64_gpr_home(2, function_name, rhs_name, "s1", 9),
              rv64_gpr_home(3, function_name, result_name, "s2", 18),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_stack_slot_to_gpr_move_bundle_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("stack_move");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto destination_name = prepared.names.value_names.intern("%dst");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I16, "%src");
  prepared.module.functions.push_back(bir::Function{
      .name = "stack_move",
      .return_type = bir::TypeKind::I16,
      .return_size_bytes = 2,
      .return_align_bytes = 2,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.stack_layout.frame_size_bytes = 16;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{6},
          .function_name = function_name,
          .offset_bytes = 8,
          .size_bytes = 2,
          .align_bytes = 2,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_i16_stack_slot_home(1,
                                       function_name,
                                       source_name,
                                       prepare::PreparedFrameSlotId{6},
                                       8),
              rv64_gpr_home(2, function_name, destination_name, "s1", 9),
          },
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeReturn,
              .block_index = 0,
              .instruction_index = 0,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 1,
                  .to_value_id = 2,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_contiguous_width = 1,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              }},
          }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_join_transfer_select_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto true_predecessor = prepared.names.block_labels.intern("pred.true");
  const auto false_predecessor = prepared.names.block_labels.intern("pred.false");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto false_name = prepared.names.value_names.intern("%fallback");
  const auto result_name = prepared.names.value_names.intern("%selected");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
                  .compare_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
                  .rhs = bir::Value::immediate_i32(1),
                  .true_value = bir::Value::immediate_i32(1),
                  .false_value =
                      bir::Value::named(bir::TypeKind::I32, "%fallback"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%selected");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
      .join_transfers =
          {
              prepare::PreparedJoinTransfer{
                  .function_name = function_name,
                  .join_block_label = block_label,
                  .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
                  .kind = prepare::PreparedJoinTransferKind::PhiEdge,
                  .carrier_kind =
                      prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
                  .incomings =
                      {
                          bir::PhiIncoming{
                              .label = "pred.true",
                              .value = bir::Value::immediate_i32(1),
                              .label_id = true_predecessor,
                          },
                          bir::PhiIncoming{
                              .label = "pred.false",
                              .value =
                                  bir::Value::named(bir::TypeKind::I32, "%fallback"),
                              .label_id = false_predecessor,
                          },
                      },
                  .edge_transfers =
                      {
                          prepare::PreparedEdgeValueTransfer{
                              .predecessor_label = true_predecessor,
                              .successor_label = block_label,
                              .incoming_value = bir::Value::immediate_i32(1),
                              .destination_value =
                                  bir::Value::named(bir::TypeKind::I32, "%selected"),
                          },
                          prepare::PreparedEdgeValueTransfer{
                              .predecessor_label = false_predecessor,
                              .successor_label = block_label,
                              .incoming_value =
                                  bir::Value::named(bir::TypeKind::I32, "%fallback"),
                              .destination_value =
                                  bir::Value::named(bir::TypeKind::I32, "%selected"),
                          },
                      },
                  .source_branch_block_label = block_label,
                  .source_true_transfer_index = std::size_t{0},
                  .source_false_transfer_index = std::size_t{1},
                  .source_true_incoming_label = true_predecessor,
                  .source_false_incoming_label = false_predecessor,
              },
          },
  });
  prepared.stack_layout.frame_size_bytes = 4;
  prepared.stack_layout.frame_alignment_bytes = 4;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{3},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, lhs_name, "t0", 5),
              rv64_gpr_home(2, function_name, false_name, "t1", 6),
              rv64_stack_slot_home(3,
                                   function_name,
                                   result_name,
                                   prepare::PreparedFrameSlotId{3},
                                   0),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_join_transfer_select_with_published_copies_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto true_predecessor = prepared.names.block_labels.intern("pred.true");
  const auto false_predecessor = prepared.names.block_labels.intern("pred.false");
  const auto join_label = prepared.names.block_labels.intern("join");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto false_name = prepared.names.value_names.intern("%fallback");
  const auto result_name = prepared.names.value_names.intern("%selected");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  entry.terminator.kind = bir::TerminatorKind::CondBranch;
  entry.terminator.condition = bir::Value::immediate_i1(1);
  entry.terminator.true_label = "pred.true";
  entry.terminator.true_label_id = true_predecessor;
  entry.terminator.false_label = "pred.false";
  entry.terminator.false_label_id = false_predecessor;
  bir::Block pred_true{
      .label = "pred.true",
      .terminator = bir::Terminator{},
      .label_id = true_predecessor,
  };
  pred_true.terminator.kind = bir::TerminatorKind::Branch;
  pred_true.terminator.target_label = "join";
  pred_true.terminator.target_label_id = join_label;
  bir::Block pred_false{
      .label = "pred.false",
      .terminator = bir::Terminator{},
      .label_id = false_predecessor,
  };
  pred_false.terminator.kind = bir::TerminatorKind::Branch;
  pred_false.terminator.target_label = "join";
  pred_false.terminator.target_label_id = join_label;
  bir::Block join{
      .label = "join",
      .insts =
          {
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
                  .compare_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
                  .rhs = bir::Value::immediate_i32(1),
                  .true_value = bir::Value::immediate_i32(1),
                  .false_value =
                      bir::Value::named(bir::TypeKind::I32, "%fallback"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = join_label,
  };
  join.terminator.value = bir::Value::named(bir::TypeKind::I32, "%selected");

  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry),
                 std::move(pred_true),
                 std::move(pred_false),
                 std::move(join)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::CondBranch,
                  .true_label = true_predecessor,
                  .false_label = false_predecessor,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = true_predecessor,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = join_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = false_predecessor,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = join_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = join_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .join_transfers =
          {
              prepare::PreparedJoinTransfer{
                  .function_name = function_name,
                  .join_block_label = join_label,
                  .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
                  .kind = prepare::PreparedJoinTransferKind::PhiEdge,
                  .carrier_kind =
                      prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
                  .incomings =
                      {
                          bir::PhiIncoming{
                              .label = "pred.true",
                              .value = bir::Value::immediate_i32(1),
                              .label_id = true_predecessor,
                          },
                          bir::PhiIncoming{
                              .label = "pred.false",
                              .value =
                                  bir::Value::named(bir::TypeKind::I32, "%fallback"),
                              .label_id = false_predecessor,
                          },
                      },
                  .edge_transfers =
                      {
                          prepare::PreparedEdgeValueTransfer{
                              .predecessor_label = true_predecessor,
                              .successor_label = join_label,
                              .incoming_value = bir::Value::immediate_i32(1),
                              .destination_value =
                                  bir::Value::named(bir::TypeKind::I32, "%selected"),
                          },
                          prepare::PreparedEdgeValueTransfer{
                              .predecessor_label = false_predecessor,
                              .successor_label = join_label,
                              .incoming_value =
                                  bir::Value::named(bir::TypeKind::I32, "%fallback"),
                              .destination_value =
                                  bir::Value::named(bir::TypeKind::I32, "%selected"),
                          },
                      },
                  .source_branch_block_label = entry_label,
                  .source_true_transfer_index = std::size_t{0},
                  .source_false_transfer_index = std::size_t{1},
                  .source_true_incoming_label = true_predecessor,
                  .source_false_incoming_label = false_predecessor,
              },
          },
      .parallel_copy_bundles =
          {
              prepare::PreparedParallelCopyBundle{
                  .predecessor_label = true_predecessor,
                  .successor_label = join_label,
                  .execution_site =
                      prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
                  .execution_block_label = true_predecessor,
                  .moves = {prepare::PreparedParallelCopyMove{
                      .join_transfer_index = 0,
                      .edge_transfer_index = 0,
                      .source_value = bir::Value::immediate_i32(1),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%selected"),
                      .carrier_kind =
                          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
                  }},
                  .steps = {prepare::PreparedParallelCopyStep{}},
              },
              prepare::PreparedParallelCopyBundle{
                  .predecessor_label = false_predecessor,
                  .successor_label = join_label,
                  .execution_site =
                      prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
                  .execution_block_label = false_predecessor,
                  .moves = {prepare::PreparedParallelCopyMove{
                      .join_transfer_index = 0,
                      .edge_transfer_index = 1,
                      .source_value =
                          bir::Value::named(bir::TypeKind::I32, "%fallback"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%selected"),
                      .carrier_kind =
                          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
                  }},
                  .steps = {prepare::PreparedParallelCopyStep{}},
              },
          },
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, lhs_name, "s1", 9),
              rv64_gpr_home(2, function_name, false_name, "t1", 6),
              rv64_gpr_home(3, function_name, result_name, "a0", 10),
          },
      .move_bundles =
          {
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BlockEntry,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .block_index = 1,
                  .source_parallel_copy_predecessor_label = true_predecessor,
                  .source_parallel_copy_successor_label = join_label,
                  .moves = {prepare::PreparedMoveResolution{
                      .to_value_id = 3,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .source_immediate_i32 = 1,
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  }},
              },
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BlockEntry,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .block_index = 2,
                  .source_parallel_copy_predecessor_label = false_predecessor,
                  .source_parallel_copy_successor_label = join_label,
                  .moves = {prepare::PreparedMoveResolution{
                      .from_value_id = 2,
                      .to_value_id = 3,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  }},
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_join_transfer_select_with_edge_compare_source_module() {
  auto prepared = make_prepared_join_transfer_select_with_published_copies_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto cmp_name = prepared.names.value_names.intern("%rhs.cmp");
  auto& join = prepared.module.functions.front().blocks.at(3);
  auto* select = std::get_if<bir::SelectInst>(&join.insts.front());
  if (select == nullptr) {
    return prepared;
  }

  join.insts.insert(join.insts.begin(),
                    bir::BinaryInst{
                        .opcode = bir::BinaryOpcode::Ne,
                        .result = bir::Value::named(bir::TypeKind::I32, "%rhs.cmp"),
                        .operand_type = bir::TypeKind::I32,
                        .lhs = bir::Value::named(bir::TypeKind::I32, "%fallback"),
                        .rhs = bir::Value::immediate_i32(0),
                    });
  select = std::get_if<bir::SelectInst>(&join.insts.at(1));
  select->false_value = bir::Value::named(bir::TypeKind::I32, "%rhs.cmp");

  auto& join_transfer =
      prepared.control_flow.functions.front().join_transfers.front();
  join_transfer.incomings.at(1).value =
      bir::Value::named(bir::TypeKind::I32, "%rhs.cmp");
  join_transfer.edge_transfers.at(1).incoming_value =
      bir::Value::named(bir::TypeKind::I32, "%rhs.cmp");
  auto& false_parallel_copy =
      prepared.control_flow.functions.front().parallel_copy_bundles.at(1);
  false_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I32, "%rhs.cmp");

  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes.push_back(rv64_gpr_home(4, function_name, cmp_name, "t0", 5));
  locations.move_bundles.at(1).moves.front().from_value_id = 4;
  return prepared;
}

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_dependent_edge_compare_source_module() {
  auto prepared = make_prepared_join_transfer_select_with_edge_compare_source_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto shr_name = prepared.names.value_names.intern("%rhs.shr");
  const auto mask_name = prepared.names.value_names.intern("%rhs.mask");
  const auto value_name = prepared.names.value_names.intern("%rhs.value");
  auto& join = prepared.module.functions.front().blocks.at(3);
  auto* compare = std::get_if<bir::BinaryInst>(&join.insts.front());
  if (compare == nullptr) {
    return prepared;
  }

  join.insts.insert(join.insts.begin(),
                    {
                        bir::BinaryInst{
                            .opcode = bir::BinaryOpcode::LShr,
                            .result = bir::Value::named(bir::TypeKind::I32, "%rhs.shr"),
                            .operand_type = bir::TypeKind::I32,
                            .lhs = bir::Value::named(bir::TypeKind::I32, "%fallback"),
                            .rhs = bir::Value::immediate_i32(3),
                        },
                        bir::BinaryInst{
                            .opcode = bir::BinaryOpcode::And,
                            .result = bir::Value::named(bir::TypeKind::I32, "%rhs.mask"),
                            .operand_type = bir::TypeKind::I32,
                            .lhs = bir::Value::named(bir::TypeKind::I32, "%rhs.shr"),
                            .rhs = bir::Value::immediate_i32(7),
                        },
                        bir::BinaryInst{
                            .opcode = bir::BinaryOpcode::Add,
                            .result = bir::Value::named(bir::TypeKind::I32, "%rhs.value"),
                            .operand_type = bir::TypeKind::I32,
                            .lhs = bir::Value::named(bir::TypeKind::I32, "%rhs.mask"),
                            .rhs = bir::Value::immediate_i32(0),
                        },
                    });
  compare = std::get_if<bir::BinaryInst>(&join.insts.at(3));
  compare->lhs = bir::Value::named(bir::TypeKind::I32, "%rhs.value");
  compare->rhs = bir::Value::immediate_i32(5);

  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes.push_back(rv64_gpr_home(5, function_name, shr_name, "s2", 18));
  locations.value_homes.push_back(rv64_gpr_home(6, function_name, mask_name, "s1", 9));
  locations.value_homes.push_back(rv64_gpr_home(7, function_name, value_name, "s2", 18));
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

prepare::PreparedValueHome rv64_stack_slot_home(
    prepare::PreparedValueId value_id,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t offset_bytes) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = slot_id,
      .offset_bytes = offset_bytes,
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
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

prepare::PreparedBirModule make_prepared_byval_stack_slot_param_module(
    std::int64_t access_byte_offset = 0) {
  prepare::PreparedBirModule prepared;
  const auto function_name =
      prepared.names.function_names.intern("byval_stack_param");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto param_name = prepared.names.value_names.intern("%p.pa");
  const auto result_name = prepared.names.value_names.intern("%t0");
  const auto object_id = prepare::PreparedObjectId{18};
  const auto slot_id = prepare::PreparedFrameSlotId{0};

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .slot_name = "%p.pa",
                  .align_bytes = 4,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "byval_stack_param",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .params = {bir::Param{
          .type = bir::TypeKind::Ptr,
          .name = "%p.pa",
          .size_bytes = 72,
          .align_bytes = 4,
          .abi = bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 72,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Memory,
              .passed_in_register = false,
              .byval_copy = true,
          },
          .is_byval = true,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = object_id,
      .function_name = function_name,
      .value_name = param_name,
      .source_kind = "byval_param",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 72,
      .align_bytes = 4,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = object_id,
      .function_name = function_name,
      .offset_bytes = 0,
      .size_bytes = 72,
      .align_bytes = 4,
  });
  prepared.stack_layout.frame_size_bytes = 72;
  prepared.stack_layout.frame_alignment_bytes = 4;
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = param_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = slot_id,
                  .offset_bytes = std::size_t{0},
                  .size_bytes = std::size_t{72},
                  .align_bytes = std::size_t{4},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 72,
      .frame_alignment_bytes = 4,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .result_value_name = result_name,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
              .pointer_value_name = param_name,
              .byte_offset = access_byte_offset,
              .size_bytes = 4,
              .align_bytes = 4,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_same_module_sret_call_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto callee_name = prepared.names.function_names.intern("fill_result");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto ret_value_name = prepared.names.value_names.intern("%ret.obj");
  const auto param_x_name = prepared.names.value_names.intern("%p.x");
  const auto ret_slot_name = prepared.names.slot_names.intern("%ret.obj");
  const auto ret_slot_id = prepare::PreparedFrameSlotId{7};
  const auto ret_object_id = prepare::PreparedObjectId{11};

  bir::Block callee_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  bir::CallInst call;
  call.callee = "fill_result";
  call.args = {bir::Value::named(bir::TypeKind::Ptr, "%ret.obj"),
               bir::Value::immediate_i32(2)};
  call.arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32};
  call.arg_abi = {
      bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
      bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  };
  call.return_type = bir::TypeKind::Void;
  bir::Block main_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "fill_result",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
                     .type = bir::TypeKind::Ptr,
                     .name = "%ret.sret",
                     .size_bytes = 8,
                     .align_bytes = 8,
                     .is_sret = true,
                 },
                 bir::Param{
                     .type = bir::TypeKind::I32,
                     .name = "%p.x",
                     .size_bytes = 4,
                     .align_bytes = 4,
                     .abi = bir::CallArgAbiInfo{
                         .type = bir::TypeKind::I32,
                         .size_bytes = 4,
                         .align_bytes = 4,
                         .primary_class = bir::AbiValueClass::Integer,
                         .passed_in_register = true,
                     },
                 }},
      .blocks = {std::move(callee_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
          .name = "%ret.obj",
          .slot_id = ret_slot_name,
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
      }},
      .blocks = {std::move(main_entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = callee_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = main_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = ret_object_id,
      .function_name = main_name,
      .value_name = ret_value_name,
      .source_kind = "local_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = ret_slot_id,
      .object_id = ret_object_id,
      .function_name = main_name,
      .offset_bytes = 16,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.stack_layout.frame_size_bytes = 32;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = callee_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 1,
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = main_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .frame_slot_order = {ret_slot_id},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = callee_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 2,
          .function_name = callee_name,
          .value_name = param_x_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a1"},
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"fill_result"},
          .memory_return = prepare::PreparedMemoryReturnPlan{
              .sret_arg_index = std::size_t{0},
              .storage_slot_name = ret_slot_name,
              .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .slot_id = ret_slot_id,
              .stack_offset_bytes = std::size_t{16},
              .size_bytes = 4,
              .align_bytes = 4,
          },
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::AggregateAddress,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_register_name = std::string{"s1"},
                      .source_slot_id = ret_slot_id,
                      .source_stack_offset_bytes = std::size_t{16},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::PreparedCallArgumentSourceSelectionKind::
                                  LocalFrameAddressMaterialization,
                              .source_value_id = prepare::PreparedValueId{1},
                              .source_value_name = ret_value_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::Register,
                              .source_slot_id = ret_slot_id,
                              .source_stack_offset_bytes = std::size_t{16},
                              .source_size_bytes = std::size_t{4},
                              .source_align_bytes = std::size_t{4},
                              .source_pointer_byte_delta = std::int64_t{0},
                              .address_materialization_block_label = block_label,
                              .address_materialization_inst_index = std::size_t{0},
                              .address_materialization_frame_slot_id = ret_slot_id,
                              .address_materialization_byte_offset = std::int64_t{16},
                          },
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
                      .source_literal = bir::Value::immediate_i32(2),
                      .destination_register_name = std::string{"a1"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                  },
              },
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = main_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .address_materializations = {prepare::PreparedAddressMaterialization{
          .function_name = main_name,
          .block_label = block_label,
          .inst_index = 0,
          .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
          .result_value_name = ret_value_name,
          .result_value_id = prepare::PreparedValueId{1},
          .result_home_kind = prepare::PreparedValueHomeKind::StackSlot,
          .frame_slot_id = ret_slot_id,
          .byte_offset = 16,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_gpr_stack_slot_param_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("scalar_gpr_stack_param_home");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto param_name = prepared.names.value_names.intern("%p.fmt");
  const auto object_id = prepare::PreparedObjectId{9};
  const auto slot_id = prepare::PreparedFrameSlotId{11};

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "scalar_gpr_stack_param_home",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = bir::TypeKind::I32,
          .name = "%p.fmt",
          .size_bytes = 4,
          .align_bytes = 4,
          .abi = bir::CallArgAbiInfo{
              .type = bir::TypeKind::I32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = object_id,
      .function_name = function_name,
      .value_name = param_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .address_exposed = false,
      .requires_home_slot = false,
      .permanent_home_slot = false,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = object_id,
      .function_name = function_name,
      .offset_bytes = 64,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.stack_layout.frame_size_bytes = 80;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 6,
          .function_name = function_name,
          .value_name = param_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = slot_id,
          .offset_bytes = std::size_t{64},
          .size_bytes = std::size_t{4},
          .align_bytes = std::size_t{4},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_fpr_formal_param_home_module(
    bool publish_target_identity) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("fpr_formal_param_home");
  const auto param_name = prepared.names.value_names.intern("%p.a");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "fpr_formal_param_home",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = bir::TypeKind::F32,
          .name = "%p.a",
          .size_bytes = 4,
          .align_bytes = 4,
      }},
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
  });

  prepare::PreparedValueHome home{
      .value_id = 1,
      .function_name = function_name,
      .value_name = param_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"fa0"},
  };
  if (publish_target_identity) {
    home.target_register_identity = prepare::PreparedTargetRegisterIdentity{
        .target_arch = c4c::TargetArch::Riscv64,
        .bank = prepare::PreparedRegisterBank::Fpr,
        .register_class = prepare::PreparedRegisterClass::Float,
        .physical_index = 10,
    };
  }
  prepared.value_locations.functions.push_back(
      prepare::PreparedValueLocationFunction{
          .function_name = function_name,
          .value_homes = {std::move(home)},
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

prepare::PreparedBirModule make_prepared_frame_slot_value_arg_call_module() {
  prepare::PreparedBirModule prepared;
  const auto callee_name = prepared.names.function_names.intern("sink");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto byte_name = prepared.names.value_names.intern("%byte");
  const auto spill_name = prepared.names.value_names.intern("%spill");

  bir::Block callee_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };

  bir::CallInst call;
  call.callee = "sink";
  call.args = {bir::Value::named(bir::TypeKind::I64, "%spill")};
  call.arg_types = {bir::TypeKind::I64};
  call.return_type = bir::TypeKind::Void;
  bir::CastInst cast;
  cast.opcode = bir::CastOpcode::SExt;
  cast.result = bir::Value::named(bir::TypeKind::I64, "%spill");
  cast.operand = bir::Value::named(bir::TypeKind::I8, "%byte");
  bir::Block main_entry{
      .label = "entry",
      .insts = {cast, call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "sink",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = bir::TypeKind::I64,
          .name = "%p.x",
          .size_bytes = 8,
          .align_bytes = 8,
      }},
      .blocks = {std::move(callee_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
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
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 2,
          .function_name = callee_name,
          .value_name = prepared.names.value_names.intern("%p.x"),
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a0"},
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = main_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = main_name,
                  .value_name = spill_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{2},
                  .offset_bytes = 16,
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = main_name,
                  .value_name = byte_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 1,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"sink"},
          .arguments = {prepare::PreparedCallArgumentPlan{
              .instruction_index = 1,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .source_value_id = prepare::PreparedValueId{1},
              .source_slot_id = prepare::PreparedFrameSlotId{2},
              .source_stack_offset_bytes = 16,
              .destination_register_name = std::string{"a0"},
              .destination_contiguous_width = 1,
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
              .source_selection = prepare::PreparedCallArgumentSourceSelection{
                  .kind =
                      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
                  .source_value_id = prepare::PreparedValueId{1},
                  .source_value_name = spill_name,
                  .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
                  .source_slot_id = prepare::PreparedFrameSlotId{2},
                  .source_stack_offset_bytes = 16,
                  .source_size_bytes = 8,
                  .source_align_bytes = 8,
              },
          }},
      }},
  });
  prepared.frame_plan.functions = {
      prepare::PreparedFramePlanFunction{
          .function_name = callee_name,
          .frame_size_bytes = 0,
          .frame_alignment_bytes = 1,
      },
      prepare::PreparedFramePlanFunction{
          .function_name = main_name,
          .frame_size_bytes = 24,
          .frame_alignment_bytes = 8,
          .frame_slot_order = {prepare::PreparedFrameSlotId{2}},
      },
  };
  prepared.stack_layout.frame_size_bytes = 1;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{2},
          .function_name = main_name,
          .offset_bytes = 16,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  };
  return prepared;
}

prepare::PreparedBirModule make_prepared_frame_slot_address_arg_call_module() {
  prepare::PreparedBirModule prepared;
  const auto callee_name = prepared.names.function_names.intern("sink");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%lv.src");
  const auto x_name = prepared.names.value_names.intern("%lv.x");
  const auto y_name = prepared.names.value_names.intern("%lv.y");
  const auto x_slot_name = prepared.names.slot_names.intern("%lv.x");
  const auto y_slot_name = prepared.names.slot_names.intern("%lv.y");

  bir::Block callee_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };

  bir::CallInst call;
  call.callee = "sink";
  call.args = {bir::Value::named(bir::TypeKind::Ptr, "%lv.x"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.y")};
  call.arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr};
  call.return_type = bir::TypeKind::Void;
  bir::Block main_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "sink",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
                     .type = bir::TypeKind::Ptr,
                     .name = "%p.x",
                     .size_bytes = 8,
                     .align_bytes = 8,
                 },
                 bir::Param{
                     .type = bir::TypeKind::Ptr,
                     .name = "%p.y",
                     .size_bytes = 8,
                     .align_bytes = 8,
                 }},
      .blocks = {std::move(callee_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
                          .name = "%lv.x",
                          .slot_id = x_slot_name,
                          .type = bir::TypeKind::I16,
                          .size_bytes = 2,
                          .align_bytes = 2,
                      },
                      bir::LocalSlot{
                          .name = "%lv.y",
                          .slot_id = y_slot_name,
                          .type = bir::TypeKind::I16,
                          .size_bytes = 2,
                          .align_bytes = 2,
                      }},
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
                  .value_name = prepared.names.value_names.intern("%p.x"),
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = callee_name,
                  .value_name = prepared.names.value_names.intern("%p.y"),
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a1"},
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = main_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 13,
                  .function_name = main_name,
                  .value_name = source_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              prepare::PreparedValueHome{
                  .value_id = 14,
                  .function_name = main_name,
                  .value_name = x_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{9},
                  .offset_bytes = 48,
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              prepare::PreparedValueHome{
                  .value_id = 15,
                  .function_name = main_name,
                  .value_name = y_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{10},
                  .offset_bytes = 56,
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"sink"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::FrameSlot,
                      .source_value_id = prepare::PreparedValueId{14},
                      .source_slot_id = prepare::PreparedFrameSlotId{9},
                      .source_stack_offset_bytes = 48,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::
                                  PreparedCallArgumentSourceSelectionKind::
                                      FrameSlotAddress,
                              .source_value_id = prepare::PreparedValueId{14},
                              .source_value_name = x_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::StackSlot,
                              .source_slot_id = prepare::PreparedFrameSlotId{7},
                              .source_stack_offset_bytes = 24,
                              .source_size_bytes = 8,
                              .source_align_bytes = 8,
                              .address_materialization_block_label = block_label,
                              .address_materialization_inst_index = 0,
                              .address_materialization_frame_slot_id =
                                  prepare::PreparedFrameSlotId{7},
                              .address_materialization_byte_offset = 24,
                          },
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::FrameSlot,
                      .source_value_id = prepare::PreparedValueId{15},
                      .source_slot_id = prepare::PreparedFrameSlotId{10},
                      .source_stack_offset_bytes = 56,
                      .destination_register_name = std::string{"a1"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::
                                  PreparedCallArgumentSourceSelectionKind::
                                      FrameSlotAddress,
                              .source_value_id = prepare::PreparedValueId{15},
                              .source_value_name = y_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::StackSlot,
                              .source_slot_id = prepare::PreparedFrameSlotId{8},
                              .source_stack_offset_bytes = 32,
                              .source_size_bytes = 8,
                              .source_align_bytes = 8,
                              .address_materialization_block_label = block_label,
                              .address_materialization_inst_index = 0,
                              .address_materialization_frame_slot_id =
                                  prepare::PreparedFrameSlotId{8},
                              .address_materialization_byte_offset = 32,
                          },
                  },
              },
          .preserved_values =
              {
                  prepare::PreparedCallPreservedValue{
                      .value_id = prepare::PreparedValueId{14},
                      .value_name = x_name,
                      .route = prepare::PreparedCallPreservationRoute::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{9},
                      .stack_offset_bytes = 48,
                      .stack_size_bytes = 8,
                      .stack_align_bytes = 8,
                      .preservation_source =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::FrameSlot,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::StackSlot,
                              .value_id = prepare::PreparedValueId{14},
                              .value_name = x_name,
                              .slot_id = prepare::PreparedFrameSlotId{9},
                              .stack_offset_bytes = 48,
                              .stack_size_bytes = 8,
                              .stack_align_bytes = 8,
                          },
                      .preservation_destination =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::FrameSlot,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::StackSlot,
                              .value_id = prepare::PreparedValueId{14},
                              .value_name = x_name,
                              .slot_id = prepare::PreparedFrameSlotId{9},
                              .stack_offset_bytes = 48,
                              .stack_size_bytes = 8,
                              .stack_align_bytes = 8,
                          },
                      .preservation_reason = "stack_slot_preservation",
                  },
              },
      }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = main_name,
      .frame_size_bytes = 12,
      .frame_alignment_bytes = 4,
      .address_materializations =
          {
              prepare::PreparedAddressMaterialization{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
                  .result_value_name = x_name,
                  .result_value_id = prepare::PreparedValueId{14},
                  .result_home_kind = prepare::PreparedValueHomeKind::Register,
                  .frame_slot_id = prepare::PreparedFrameSlotId{7},
                  .byte_offset = 24,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
                  .result_value_name = y_name,
                  .result_value_id = prepare::PreparedValueId{15},
                  .result_home_kind = prepare::PreparedValueHomeKind::Register,
                  .frame_slot_id = prepare::PreparedFrameSlotId{8},
                  .byte_offset = 32,
              },
          },
  });
  prepared.frame_plan.functions = {
      prepare::PreparedFramePlanFunction{
          .function_name = callee_name,
          .frame_size_bytes = 0,
          .frame_alignment_bytes = 1,
      },
      prepare::PreparedFramePlanFunction{
          .function_name = main_name,
          .frame_size_bytes = 64,
          .frame_alignment_bytes = 8,
          .frame_slot_order = {prepare::PreparedFrameSlotId{7},
                               prepare::PreparedFrameSlotId{8},
                               prepare::PreparedFrameSlotId{9},
                               prepare::PreparedFrameSlotId{10}},
      },
  };
  prepared.stack_layout.frame_size_bytes = 64;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{7},
          .function_name = main_name,
          .offset_bytes = 24,
          .size_bytes = 8,
          .align_bytes = 8,
          .fixed_location = true,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{8},
          .function_name = main_name,
          .offset_bytes = 32,
          .size_bytes = 8,
          .align_bytes = 8,
          .fixed_location = true,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{9},
          .function_name = main_name,
          .offset_bytes = 48,
          .size_bytes = 8,
          .align_bytes = 8,
          .fixed_location = true,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{10},
          .function_name = main_name,
          .offset_bytes = 56,
          .size_bytes = 8,
          .align_bytes = 8,
          .fixed_location = true,
      },
  };
  prepared.store_source_publications.records = {
      prepare::PreparedStoreSourcePublicationRecord{
          .function_name = main_name,
          .block_label = block_label,
          .instruction_index = 0,
          .plan =
              prepare::PreparedStoreSourcePublicationPlan{
                  .status =
                      prepare::PreparedStoreSourcePublicationStatus::Available,
                  .intent = prepare::PreparedStoreSourcePublicationIntent::
                      StoreLocalPublication,
                  .source_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.src"),
                  .source_value_id = prepare::PreparedValueId{13},
                  .source_value_name = source_name,
                  .destination_frame_slot_id = prepare::PreparedFrameSlotId{7},
                  .destination_size_bytes = 8,
                  .destination_align_bytes = 8,
                  .destination_stack_offset_bytes = 24,
                  .destination_stack_size_bytes = 8,
                  .destination_stack_align_bytes = 8,
              },
      },
      prepare::PreparedStoreSourcePublicationRecord{
          .function_name = main_name,
          .block_label = block_label,
          .instruction_index = 0,
          .plan =
              prepare::PreparedStoreSourcePublicationPlan{
                  .status =
                      prepare::PreparedStoreSourcePublicationStatus::Available,
                  .intent = prepare::PreparedStoreSourcePublicationIntent::
                      StoreLocalPublication,
                  .source_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.src"),
                  .source_value_id = prepare::PreparedValueId{13},
                  .source_value_name = source_name,
                  .destination_frame_slot_id = prepare::PreparedFrameSlotId{8},
                  .destination_size_bytes = 8,
                  .destination_align_bytes = 8,
                  .destination_stack_offset_bytes = 32,
                  .destination_stack_size_bytes = 8,
                  .destination_stack_align_bytes = 8,
              },
      },
  };
  prepared.call_argument_value_publications.facts = {
      prepare::PreparedCallArgumentValuePublicationFact{
          .function_name = main_name,
          .call_block_label = block_label,
          .call_instruction_index = 0,
          .arg_index = 0,
          .argument_value_id = prepare::PreparedValueId{14},
          .argument_value_name = x_name,
          .argument_object_slot_id = prepare::PreparedFrameSlotId{7},
          .argument_object_stack_offset_bytes = 24,
          .argument_object_size_bytes = 8,
          .source_store_block_label = block_label,
          .source_store_instruction_index = 0,
          .payload_value_id = prepare::PreparedValueId{13},
          .payload_value_name = source_name,
          .payload_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.src"),
          .destination_frame_slot_id = prepare::PreparedFrameSlotId{7},
          .destination_stack_offset_bytes = 24,
          .destination_size_bytes = 8,
      },
      prepare::PreparedCallArgumentValuePublicationFact{
          .function_name = main_name,
          .call_block_label = block_label,
          .call_instruction_index = 0,
          .arg_index = 1,
          .argument_value_id = prepare::PreparedValueId{15},
          .argument_value_name = y_name,
          .argument_object_slot_id = prepare::PreparedFrameSlotId{8},
          .argument_object_stack_offset_bytes = 32,
          .argument_object_size_bytes = 8,
          .source_store_block_label = block_label,
          .source_store_instruction_index = 0,
          .payload_value_id = prepare::PreparedValueId{13},
          .payload_value_name = source_name,
          .payload_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.src"),
          .destination_frame_slot_id = prepare::PreparedFrameSlotId{8},
          .destination_stack_offset_bytes = 32,
          .destination_size_bytes = 8,
      },
  };
  return prepared;
}

prepare::PreparedBirModule
make_prepared_frame_slot_address_arg_call_load_local_payload_module() {
  auto prepared = make_prepared_frame_slot_address_arg_call_module();
  const auto main_name = prepared.names.function_names.find("main");
  const auto block_label = prepared.names.block_labels.find("entry");
  const auto loaded_x_name =
      prepared.names.value_names.intern("%lv.loaded.x");
  const auto loaded_y_name =
      prepared.names.value_names.intern("%lv.loaded.y");
  const auto source_slot_name = prepared.names.slot_names.intern("%lv.src");

  static bir::LoadLocalInst x_load{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.x"),
      .slot_name = "%lv.src",
      .align_bytes = 8,
  };
  static bir::LoadLocalInst y_load{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.y"),
      .slot_name = "%lv.src",
      .align_bytes = 8,
  };
  x_load.slot_id = source_slot_name;
  y_load.slot_id = source_slot_name;

  prepared.value_locations.functions[1].value_homes.push_back(
      prepare::PreparedValueHome{
          .value_id = 16,
          .function_name = main_name,
          .value_name = loaded_x_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"s2"},
          .size_bytes = 8,
          .align_bytes = 8,
      });
  prepared.value_locations.functions[1].value_homes.push_back(
      prepare::PreparedValueHome{
          .value_id = 17,
          .function_name = main_name,
          .value_name = loaded_y_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"s2"},
          .size_bytes = 8,
          .align_bytes = 8,
      });

  auto& x_record = prepared.store_source_publications.records[0];
  x_record.instruction_index = 0;
  x_record.plan.source_value =
      bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.x");
  x_record.plan.source_value_id = prepare::PreparedValueId{16};
  x_record.plan.source_value_name = loaded_x_name;
  x_record.plan.source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal;
  x_record.plan.source_producer_block_label = block_label;
  x_record.plan.source_producer_instruction_index = 0;
  x_record.plan.source_load_local = &x_load;

  auto& y_record = prepared.store_source_publications.records[1];
  y_record.instruction_index = 0;
  y_record.plan.source_value =
      bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.y");
  y_record.plan.source_value_id = prepare::PreparedValueId{17};
  y_record.plan.source_value_name = loaded_y_name;
  y_record.plan.source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal;
  y_record.plan.source_producer_block_label = block_label;
  y_record.plan.source_producer_instruction_index = 0;
  y_record.plan.source_load_local = &y_load;

  auto& x_fact = prepared.call_argument_value_publications.facts[0];
  x_fact.source_store_instruction_index = 0;
  x_fact.payload_value_id = prepare::PreparedValueId{16};
  x_fact.payload_value_name = loaded_x_name;
  x_fact.payload_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.x");

  auto& y_fact = prepared.call_argument_value_publications.facts[1];
  y_fact.source_store_instruction_index = 0;
  y_fact.payload_value_id = prepare::PreparedValueId{17};
  y_fact.payload_value_name = loaded_y_name;
  y_fact.payload_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.y");

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

int rejects_prepared_critical_edge_parallel_copy_with_shared_diagnostic() {
  const auto prepared = make_prepared_critical_edge_parallel_copy_module();
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected RV64 object emission to reject critical-edge copies");
  }

  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("expected diagnostic RV64 object emission result to reject");
  }
  if (result.prepared_consumer_category !=
      prepare::PreparedObjectConsumerDiagnosticCategory::
          UnsupportedParallelCopyExecutionSite) {
    return fail("expected shared unsupported parallel-copy execution-site category");
  }
  if (result.diagnostic !=
      "prepared critical-edge parallel-copy obligation has no target-consumable block event") {
    return fail("expected shared critical-edge parallel-copy diagnostic message");
  }
  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (image.ok() || image.image.has_value()) {
    return fail("expected diagnostic RV64 object image result to reject");
  }
  if (image.prepared_consumer_category != result.prepared_consumer_category ||
      image.diagnostic != result.diagnostic) {
    return fail("expected RV64 prepared object writer to preserve shared diagnostic");
  }
  return 0;
}

int builds_prepared_successor_entry_copy_from_shared_traversal() {
  const auto prepared = make_prepared_successor_entry_copy_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected RV64 object emission to accept successor-entry copy");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "entry_copy");
  if (text == nullptr || function == nullptr) {
    return fail("expected successor-entry copy object to publish text/function");
  }
  if (text->bytes.size() != 12 || function->size_bytes != 12) {
    return fail("expected branch, traversal copy, and return fragments");
  }
  if (text->bytes[4] != 0x13 || text->bytes[5] != 0x85 ||
      text->bytes[6] != 0x02 || text->bytes[7] != 0x00) {
    return fail("expected traversal successor-entry copy to emit mv a0, t0");
  }
  return 0;
}

int builds_prepared_fused_sgt_i32_compare_branch_object() {
  const auto prepared =
      make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Sgt);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared fused sgt i32 compare branch RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "cmp_branch");
  const auto* true_label = object::find_symbol(*module, ".Lcmp_branch_is_true");
  const auto* false_label = object::find_symbol(*module, ".Lcmp_branch_is_false");
  if (text == nullptr || function == nullptr || true_label == nullptr ||
      false_label == nullptr) {
    return fail("expected fused compare branch object symbols and text");
  }
  if (text->bytes.size() != 32 || text->size_bytes != 32 ||
      function->value != 0 || function->size_bytes != 32 ||
      true_label->value != 16 || false_label->value != 24) {
    return fail("expected fused compare branch object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00030e13 ||
      read_u32(text->bytes, 4) != 0x00028e93 ||
      read_u32(text->bytes, 8) != 0x01de4063 ||
      read_u32(text->bytes, 12) != 0x0000006f ||
      read_u32(text->bytes, 16) != 0x00100513 ||
      read_u32(text->bytes, 20) != 0x00008067 ||
      read_u32(text->bytes, 24) != 0x00000513 ||
      read_u32(text->bytes, 28) != 0x00008067) {
    return fail("expected sgt i32 branch to lower as blt with swapped operands");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 8 ||
      module->relocations[0].type != R_RISCV_BRANCH ||
      module->relocations[0].symbol != true_label->id ||
      module->relocations[0].addend != 0 ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 12 ||
      module->relocations[1].type != R_RISCV_JAL ||
      module->relocations[1].symbol != false_label->id ||
      module->relocations[1].addend != 0) {
    return fail("expected fused compare branch local relocations");
  }
  return 0;
}

int builds_prepared_fused_sle_i32_compare_branch_object() {
  const auto prepared =
      make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Sle);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared fused sle i32 compare branch RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "cmp_branch");
  const auto* true_label = object::find_symbol(*module, ".Lcmp_branch_is_true");
  const auto* false_label = object::find_symbol(*module, ".Lcmp_branch_is_false");
  if (text == nullptr || function == nullptr || true_label == nullptr ||
      false_label == nullptr) {
    return fail("expected fused sle compare branch object symbols and text");
  }
  if (text->bytes.size() != 32 || text->size_bytes != 32 ||
      function->value != 0 || function->size_bytes != 32 ||
      true_label->value != 16 || false_label->value != 24) {
    return fail("expected fused sle compare branch object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00030e13 ||
      read_u32(text->bytes, 4) != 0x00028e93 ||
      read_u32(text->bytes, 8) != 0x01de5063 ||
      read_u32(text->bytes, 12) != 0x0000006f ||
      read_u32(text->bytes, 16) != 0x00100513 ||
      read_u32(text->bytes, 20) != 0x00008067 ||
      read_u32(text->bytes, 24) != 0x00000513 ||
      read_u32(text->bytes, 28) != 0x00008067) {
    return fail("expected sle i32 branch to lower as bge with swapped operands");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 8 ||
      module->relocations[0].type != R_RISCV_BRANCH ||
      module->relocations[0].symbol != true_label->id ||
      module->relocations[0].addend != 0 ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 12 ||
      module->relocations[1].type != R_RISCV_JAL ||
      module->relocations[1].symbol != false_label->id ||
      module->relocations[1].addend != 0) {
    return fail("expected fused sle compare branch local relocations");
  }
  return 0;
}

int builds_prepared_fused_ne_ptr_null_compare_branch_object() {
  const auto prepared = make_prepared_fused_compare_branch_module(
      bir::BinaryOpcode::Ne,
      bir::TypeKind::Ptr,
      null_pointer_value());
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared fused ne ptr null compare branch RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "cmp_branch");
  const auto* true_label = object::find_symbol(*module, ".Lcmp_branch_is_true");
  const auto* false_label = object::find_symbol(*module, ".Lcmp_branch_is_false");
  if (text == nullptr || function == nullptr || true_label == nullptr ||
      false_label == nullptr) {
    return fail("expected fused pointer-null compare branch object symbols and text");
  }
  if (text->bytes.size() != 32 || text->size_bytes != 32 ||
      function->value != 0 || function->size_bytes != 32 ||
      true_label->value != 16 || false_label->value != 24) {
    return fail("expected fused pointer-null compare branch object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028e13 ||
      read_u32(text->bytes, 4) != 0x00000e93 ||
      read_u32(text->bytes, 8) != 0x01de1063 ||
      read_u32(text->bytes, 12) != 0x0000006f ||
      read_u32(text->bytes, 16) != 0x00100513 ||
      read_u32(text->bytes, 20) != 0x00008067 ||
      read_u32(text->bytes, 24) != 0x00000513 ||
      read_u32(text->bytes, 28) != 0x00008067) {
    return fail("expected ne ptr null branch to lower as bne against zero");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 8 ||
      module->relocations[0].type != R_RISCV_BRANCH ||
      module->relocations[0].symbol != true_label->id ||
      module->relocations[0].addend != 0 ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 12 ||
      module->relocations[1].type != R_RISCV_JAL ||
      module->relocations[1].symbol != false_label->id ||
      module->relocations[1].addend != 0) {
    return fail("expected fused pointer-null compare branch local relocations");
  }
  return 0;
}

int rejects_prepared_fused_compare_branch_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering";

  if (expect_prepared_rejection_diagnostic(
          make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Sgt,
                                                    bir::TypeKind::I64),
          diagnostic) != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Eq,
                                                    bir::TypeKind::Ptr,
                                                    null_pointer_value()),
          diagnostic) != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fused_compare_branch_module(
              bir::BinaryOpcode::Ne,
              bir::TypeKind::Ptr,
              nonnull_pointer_immediate(8)),
          diagnostic) != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Ne,
                                                    bir::TypeKind::Ptr),
          diagnostic) != 0) {
    return 1;
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

int builds_prepared_traversed_wide_rematerialized_return_object() {
  const auto prepared = make_prepared_wide_rematerialized_return_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared wide immediate-return RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared wide immediate-return object to publish text/main");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 16 ||
      main_symbol->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared wide immediate-return object to contain one load-immediate return fragment");
  }
  if (read_u32(text->bytes, 0) != 0xf8000513 ||
      read_u32(text->bytes, 4) != 0x00c51513 ||
      read_u32(text->bytes, 8) != 0x00c51513 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail("expected wide rematerialized return to materialize INT_MIN then ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared wide immediate-return object to need no relocations");
  }
  return 0;
}

int rejects_prepared_rematerialized_return_without_typed_immediate_fact() {
  auto prepared = make_prepared_rematerialized_return_module();
  prepared.value_locations.functions.front().value_homes.at(1).value_name =
      c4c::kInvalidValueName;
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
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

int builds_prepared_byval_stack_copy_same_module_call_object() {
  const auto prepared = make_prepared_byval_stack_copy_same_module_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared RV64 byval stack-copy call object to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* callee = object::find_symbol(module, "consume_pair");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected prepared byval stack-copy call object to publish text/functions");
  }
  if (text->bytes.size() != 92 || text->size_bytes != 92 ||
      callee->value != 0 || callee->size_bytes != 16 ||
      main->value != 16 || main->size_bytes != 76) {
    return fail("expected prepared byval stack-copy call object text layout");
  }

  const std::size_t base = main->value + 8;
  const std::uint32_t expected_words[] = {
      0xfe010113,  // addi sp, sp, -32
      0x08013e03,  // ld t3, 128(sp)
      0x01c13023,  // sd t3, 0(sp)
      0x08813e03,  // ld t3, 136(sp)
      0x01c13423,  // sd t3, 8(sp)
      0x09013e03,  // ld t3, 144(sp)
      0x01c13823,  // sd t3, 16(sp)
      0x00010513,  // mv a0, sp
      0x01300593,  // li a1, 19
  };
  for (std::size_t index = 0;
       index < sizeof(expected_words) / sizeof(expected_words[0]);
       ++index) {
    if (read_u32(text->bytes, base + index * 4) != expected_words[index]) {
      return fail("expected prepared byval stack-copy call to consume explicit stack-copy chunks");
    }
  }
  if (module.relocations.size() != 1 ||
      module.relocations[0].section != text->id ||
      module.relocations[0].offset != base + 36 ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != callee->id) {
    return fail("expected byval stack-copy same-module call relocation");
  }
  if (read_u32(text->bytes, base + 44) != 0x02010113 ||
      read_u32(text->bytes, base + 48) != 0x00050293 ||
      read_u32(text->bytes, base + 52) != 0x00028513 ||
      read_u32(text->bytes, base + 56) != 0x08813083 ||
      read_u32(text->bytes, base + 60) != 0x09010113 ||
      read_u32(text->bytes, base + 64) != 0x00008067) {
    return fail("expected byval stack-copy call to restore stack and publish result");
  }
  return 0;
}

int expect_byval_stack_copy_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
}

int rejects_prepared_byval_stack_copy_call_fail_closed_shapes() {
  auto prepared = make_prepared_byval_stack_copy_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].aggregate_transport =
      std::nullopt;
  if (expect_byval_stack_copy_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_copy_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].aggregate_transport->kind =
      prepare::PreparedAggregateTransportKind::ByvalRegisterLanes;
  if (expect_byval_stack_copy_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_copy_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0]
      .aggregate_transport->source_stack_offset_bytes = std::nullopt;
  if (expect_byval_stack_copy_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_copy_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0]
      .aggregate_transport->chunks.front().kind =
      prepare::PreparedAggregateTransportChunkKind::FallbackOnly;
  if (expect_byval_stack_copy_call_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int builds_prepared_same_module_sret_call_object() {
  const auto prepared = make_prepared_same_module_sret_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared same-module sret call RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* callee = object::find_symbol(module, "fill_result");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected prepared same-module sret call object to publish text/functions");
  }
  if (text->bytes.size() != 40 || text->size_bytes != 40 ||
      callee->value != 0 || callee->size_bytes != 4 ||
      main->value != 4 || main->size_bytes != 36) {
    return fail("expected prepared same-module sret call object text layout");
  }

  const std::size_t base = main->value;
  if (read_u32(text->bytes, base + 0) != 0xfd010113 ||
      read_u32(text->bytes, base + 4) != 0x02113423 ||
      read_u32(text->bytes, base + 8) != 0x01010513 ||
      read_u32(text->bytes, base + 12) != 0x00200593) {
    return fail("expected same-module sret call to pass frame-slot address in a0 and ordinary arg in a1");
  }
  if (module.relocations.size() != 1 ||
      module.relocations[0].section != text->id ||
      module.relocations[0].offset != base + 16 ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != callee->id) {
    return fail("expected same-module sret call relocation");
  }
  if (read_u32(text->bytes, base + 24) != 0x02813083 ||
      read_u32(text->bytes, base + 28) != 0x03010113 ||
      read_u32(text->bytes, base + 32) != 0x00008067) {
    return fail("expected same-module sret call to restore stack and return");
  }
  return 0;
}

int expect_same_module_sret_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
}

int rejects_prepared_same_module_sret_call_fail_closed_shapes() {
  auto prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].wrapper_kind =
      prepare::PreparedCallWrapperKind::DirectExternFixedArity;
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].memory_return->encoding =
      prepare::PreparedStorageEncodingKind::Register;
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].memory_return->sret_arg_index =
      std::size_t{1};
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].memory_return->slot_id =
      prepare::PreparedFrameSlotId{99};
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].source_selection =
      std::nullopt;
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0]
      .source_selection->source_stack_offset_bytes = std::size_t{20};
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0]
      .source_selection->address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{99};
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.frame_plan.functions[1].has_dynamic_stack = true;
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int builds_prepared_scalar_stack_result_call_object() {
  const auto prepared = make_prepared_scalar_stack_result_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared scalar stack-result call RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* callee = object::find_symbol(module, "add_three");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected prepared scalar stack-result call object to publish text/functions");
  }
  if (module.relocations.size() != 1 ||
      module.relocations[0].section != text->id ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != callee->id ||
      module.relocations[0].offset < main->value ||
      module.relocations[0].offset + 8 >= text->bytes.size()) {
    return fail("expected scalar stack-result same-module call relocation");
  }
  if (read_u32(text->bytes, module.relocations[0].offset + 8) != 0x00a11223) {
    return fail("expected scalar i16 call result to publish from a0 into stack slot");
  }
  return 0;
}

int builds_prepared_scalar_stack_result_call_with_inferred_gpr_banks_object() {
  auto prepared = make_prepared_scalar_stack_result_call_module();
  auto& result_plan = *prepared.call_plans.functions[0].calls[0].result;
  result_plan.value_bank = prepare::PreparedRegisterBank::None;
  result_plan.source_register_bank = prepare::PreparedRegisterBank::None;

  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared scalar stack-result call with inferred GPR banks "
                "to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* callee = object::find_symbol(module, "add_three");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected inferred-bank scalar stack-result call object to publish "
                "text/functions");
  }
  if (module.relocations.size() != 1 ||
      module.relocations[0].section != text->id ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != callee->id ||
      module.relocations[0].offset < main->value ||
      module.relocations[0].offset + 8 >= text->bytes.size()) {
    return fail("expected inferred-bank scalar stack-result same-module call relocation");
  }
  if (read_u32(text->bytes, module.relocations[0].offset + 8) != 0x00a11223) {
    return fail("expected inferred-bank scalar i16 call result to publish from a0 "
                "into stack slot");
  }
  return 0;
}

int expect_scalar_stack_result_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
}

int rejects_prepared_scalar_stack_result_call_fail_closed_shapes() {
  auto prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->source_register_name =
      std::nullopt;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->source_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->source_register_bank =
      prepare::PreparedRegisterBank::Fpr;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->destination_contiguous_width = 2;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->destination_slot_id =
      std::nullopt;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->destination_stack_offset_bytes = 6;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.value_locations.functions[1].value_homes[0].slot_id =
      prepare::PreparedFrameSlotId{13};
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  if (auto* call = std::get_if<bir::CallInst>(
          &prepared.module.functions[1].blocks[0].insts[0])) {
    call->result->type = bir::TypeKind::F32;
  }
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  if (auto* call = std::get_if<bir::CallInst>(
          &prepared.module.functions[1].blocks[0].insts[0])) {
    call->result->type = bir::TypeKind::Ptr;
    call->return_type = bir::TypeKind::Ptr;
  }
  prepared.value_locations.functions[1].value_homes[0].offset_bytes = 8;
  prepared.value_locations.functions[1].value_homes[0].size_bytes = 8;
  prepared.value_locations.functions[1].value_homes[0].align_bytes = 8;
  prepared.call_plans.functions[0].calls[0].result->destination_stack_offset_bytes = 8;
  prepared.stack_layout.frame_slots[0].offset_bytes = 8;
  prepared.stack_layout.frame_slots[0].size_bytes = 8;
  prepared.stack_layout.frame_slots[0].align_bytes = 8;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_stack_result_call_module();
  prepared.call_plans.functions[0].calls[0].result->destination_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  if (expect_scalar_stack_result_call_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int builds_prepared_fpr_same_module_call_object() {
  const auto prepared = make_prepared_fpr_same_module_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared FPR same-module call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* caller = object::find_symbol(*module, "fpr_call");
  const auto* callee = object::find_symbol(*module, "sin");
  if (text == nullptr || caller == nullptr || callee == nullptr) {
    return fail("expected prepared FPR call object to publish text/call symbols");
  }
  if (text->bytes.size() != 36 || text->size_bytes != 36 ||
      caller->value != 0 || caller->size_bytes != 36 ||
      caller->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared FPR call object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00113423 ||
      read_u32(text->bytes, 8) != 0x22000553 ||
      read_u32(text->bytes, 12) != 0x00000097 ||
      read_u32(text->bytes, 16) != 0x000080e7 ||
      read_u32(text->bytes, 20) != 0x22a504d3 ||
      read_u32(text->bytes, 24) != 0x00813083 ||
      read_u32(text->bytes, 28) != 0x01010113 ||
      read_u32(text->bytes, 32) != 0x00008067) {
    return fail("expected framed FPR call sequence with ft0/fa0/fs1 moves");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 12 ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected FPR same-module call relocation at call pair");
  }
  return 0;
}

int preserves_missing_variadic_entry_plan_diagnostic() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_variadic_return_zero_module(),
      "unsupported_function_admission: variadic functions are not supported by the RV64 object route; missing variadic entry contract facts were not prepared");
}

int preserves_missing_variadic_required_facts_diagnostic() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_variadic_missing_required_facts_module(),
      "unsupported_function_admission: variadic functions are not supported by the RV64 object route; missing_required_facts=[target_abi.va_list_layout]");
}

int rejects_incomplete_helper_free_variadic_entry_contract() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_variadic_helper_free_incomplete_contract_module(),
      "unsupported_function_admission: RV64 helper-free variadic entry requires a complete one-field overflow-area va_list contract");
}

int builds_fact_complete_helper_free_variadic_entry_object() {
  const auto prepared = make_prepared_variadic_helper_free_complete_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected fact-complete helper-free variadic RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "rv64_variadic");
  if (text == nullptr || function == nullptr) {
    return fail("expected helper-free variadic object to publish text/function");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      function->value != 0 || function->size_bytes != 8 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected helper-free variadic object return-zero text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00000513 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected helper-free variadic object to return zero");
  }
  if (!module->relocations.empty()) {
    return fail("expected helper-free variadic object to need no relocations");
  }
  return 0;
}

int rejects_fact_complete_variadic_va_start_without_overflow_base_state() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_variadic_va_start_module(),
      "unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state");
}

int rejects_variadic_va_start_with_missing_saved_gpr_publication_fact() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_variadic_va_start_missing_saved_gpr_publication_module(),
      "unsupported_function_admission: variadic functions are not supported by the RV64 object route; missing_required_facts=[rv64.incoming_variadic_gpr_publications]");
}

int materializes_fact_complete_variadic_va_start_with_saved_gpr_publications() {
  const auto prepared =
      make_prepared_variadic_va_start_with_saved_gpr_publications_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared saved-GPR va_start RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "rv64_va_start");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared va_start object to publish text/function");
  }
  if (text->bytes.size() != 56 || text->size_bytes != 56 ||
      function->value != 0 || function->size_bytes != 56 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared saved-GPR va_start object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfb010113 ||
      read_u32(text->bytes, 4) != 0x00b13423 ||
      read_u32(text->bytes, 8) != 0x00c13823 ||
      read_u32(text->bytes, 12) != 0x00d13c23 ||
      read_u32(text->bytes, 16) != 0x02e13023 ||
      read_u32(text->bytes, 20) != 0x02f13423 ||
      read_u32(text->bytes, 24) != 0x03013823 ||
      read_u32(text->bytes, 28) != 0x03113c23 ||
      read_u32(text->bytes, 32) != 0x04810513 ||
      read_u32(text->bytes, 36) != 0x00810313 ||
      read_u32(text->bytes, 40) != 0x00653023 ||
      read_u32(text->bytes, 44) != 0x00000513 ||
      read_u32(text->bytes, 48) != 0x05010113 ||
      read_u32(text->bytes, 52) != 0x00008067) {
    return fail("expected va_start to store incoming post-named GPRs before exposing the overflow-area pointer");
  }
  if (!module->relocations.empty()) {
    return fail("expected materialized va_start helper to need no relocations");
  }
  return 0;
}

int loads_rv64_va_start_published_word_after_helper() {
  const auto prepared =
      make_prepared_variadic_va_start_then_load_published_word_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared va_start followed by va_list load RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "rv64_va_start");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared va_start load object to publish text/function");
  }
  if (text->bytes.size() != 60 || text->size_bytes != 60 ||
      function->value != 0 || function->size_bytes != 60 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared va_start load object text layout");
  }
  if (read_u32(text->bytes, 40) != 0x00653023 ||
      read_u32(text->bytes, 44) != 0x04813603 ||
      read_u32(text->bytes, 48) != 0x00000513 ||
      read_u32(text->bytes, 52) != 0x05010113 ||
      read_u32(text->bytes, 56) != 0x00008067) {
    return fail("expected load_local after va_start to read helper-published va_list word");
  }
  return 0;
}

int rejects_malformed_variadic_saved_gpr_publications() {
  {
    auto prepared =
        make_prepared_variadic_va_start_with_saved_gpr_publications_module();
    auto& publications = prepared.variadic_entry_plans.functions.front()
                             .rv64_incoming_variadic_gpr_publications;
    publications.back() = publications.front();
    if (expect_prepared_rejection_diagnostic(
            std::move(prepared),
            "unsupported_function_admission: RV64 variadic entry incoming GPR publications must not contain duplicate sources or destinations") != 0) {
      return 1;
    }
  }
  {
    auto prepared =
        make_prepared_variadic_va_start_with_saved_gpr_publications_module();
    prepared.variadic_entry_plans.functions.front()
        .rv64_incoming_variadic_gpr_publications.front()
        .source_register_name = "a2";
    if (expect_prepared_rejection_diagnostic(
            std::move(prepared),
            "unsupported_function_admission: RV64 variadic entry incoming GPR publication source register is malformed") != 0) {
      return 1;
    }
  }
  {
    auto prepared =
        make_prepared_variadic_va_start_with_saved_gpr_publications_module();
    prepared.variadic_entry_plans.functions.front()
        .rv64_incoming_variadic_gpr_publications.front()
        .destination_slot_id = prepare::PreparedFrameSlotId{99};
    if (expect_prepared_rejection_diagnostic(
            std::move(prepared),
            "unsupported_function_admission: RV64 variadic entry incoming GPR publication destination shape is malformed") != 0) {
      return 1;
    }
  }
  auto prepared =
      make_prepared_variadic_va_start_with_saved_gpr_publications_module();
  auto& backing_slot = prepared.stack_layout.frame_slots[1];
  backing_slot.size_bytes = 8;
  return expect_prepared_rejection_diagnostic(
      std::move(prepared),
      "unsupported_function_admission: RV64 variadic entry incoming GPR publications require a supported overflow-area backing slot");
}

int rejects_malformed_variadic_va_start_destination_homes() {
  if (expect_prepared_rejection_diagnostic(
          make_prepared_variadic_va_start_module(
              true /*include_overflow_area_initial_state*/,
              false /*destination_va_list_is_stack_slot*/,
              true /*destination_address_is_gpr*/),
          "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list in a supported prepared stack-slot home") != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_variadic_va_start_module(
              true /*include_overflow_area_initial_state*/,
              true /*destination_va_list_is_stack_slot*/,
              false /*destination_address_is_gpr*/),
          "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home") != 0) {
    return 1;
  }
  return expect_prepared_rejection_diagnostic(
      make_prepared_variadic_va_start_module(
          true /*include_overflow_area_initial_state*/,
          true /*destination_va_list_is_stack_slot*/,
          true /*destination_address_is_gpr*/,
          "t1"),
      "unsupported_variadic_helper_lowering: RV64 va_start helper destination va_list address aliases the overflow-area scratch register");
}

int lowers_fact_complete_variadic_va_end_as_noop() {
  const auto prepared = make_prepared_variadic_va_end_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared va_end RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "rv64_va_end");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared va_end object to publish text/function");
  }
  if (text->bytes.empty() || text->size_bytes != text->bytes.size() ||
      function->value != 0 || function->size_bytes != text->bytes.size() ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared va_end object to publish one text function");
  }
  if (!module->relocations.empty()) {
    return fail("expected va_end no-op lowering to need no relocations");
  }
  if (object::find_symbol(*module, "llvm.va_end.p0") != nullptr) {
    return fail("expected va_end no-op lowering to avoid an extern symbol");
  }
  return 0;
}

int rejects_malformed_variadic_va_end_direct_extern_shapes() {
  auto mismatched_callee =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(
          make_prepared_variadic_va_end_module("not.llvm.va_end.p0"));
  if (mismatched_callee.ok() || mismatched_callee.module.has_value()) {
    return fail("expected mismatched prepared va_end callee plan to reject");
  }
  if (mismatched_callee.diagnostic !=
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering") {
    return fail("expected mismatched va_end plan to fail closed before call relocation");
  }

  auto multiple_args =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(
          make_prepared_variadic_va_end_module("llvm.va_end.p0", 2));
  if (multiple_args.ok() || multiple_args.module.has_value()) {
    return fail("expected malformed two-argument va_end call to reject");
  }
  if (multiple_args.diagnostic !=
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering") {
    return fail("expected malformed va_end arity to fail closed before call relocation");
  }
  return 0;
}

int materializes_fact_complete_variadic_aggregate_va_arg_helper() {
  const auto prepared =
      make_prepared_variadic_aggregate_va_arg_module(true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared aggregate va_arg RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "rv64_aggregate_va_arg");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared aggregate va_arg object to publish text/function");
  }
  if (text->bytes.size() != 44 || text->size_bytes != 44 ||
      function->value != 0 || function->size_bytes != 44 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared aggregate va_arg object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfd010113 ||
      read_u32(text->bytes, 4) != 0x0004b303 ||
      read_u32(text->bytes, 8) != 0x00033383 ||
      read_u32(text->bytes, 12) != 0x02713023 ||
      read_u32(text->bytes, 16) != 0x00830383 ||
      read_u32(text->bytes, 20) != 0x02710423 ||
      read_u32(text->bytes, 24) != 0x00930313 ||
      read_u32(text->bytes, 28) != 0x0064b023 ||
      read_u32(text->bytes, 32) != 0x00000513 ||
      read_u32(text->bytes, 36) != 0x03010113 ||
      read_u32(text->bytes, 40) != 0x00008067) {
    return fail("expected aggregate va_arg to copy overflow payload and advance va_list");
  }
  if (!module->relocations.empty()) {
    return fail("expected materialized aggregate va_arg helper to need no relocations");
  }
  return 0;
}

int rejects_aggregate_va_arg_helper_without_access_plan_payload_write_address() {
  if (expect_prepared_rejection_diagnostic(
          make_prepared_variadic_aggregate_va_arg_module(false),
          "unsupported_variadic_helper_lowering: RV64 object route requires complete prepared va_arg_aggregate helper operand homes") !=
      0) {
    return 1;
  }

  auto malformed_slot_stride =
      make_prepared_variadic_aggregate_va_arg_module(true);
  auto& malformed_plan =
      *malformed_slot_stride.variadic_entry_plans.functions.front()
           .helper_operand_homes.front()
           .aggregate_access_plan;
  malformed_plan.source_slot_size_bytes = std::size_t{4};
  malformed_plan.progression_stride_bytes = std::size_t{8};
  malformed_plan.overflow_stride_bytes = std::size_t{8};
  if (expect_prepared_rejection_diagnostic(
          malformed_slot_stride,
          "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires a supported overflow-area aggregate access plan") !=
      0) {
    return 1;
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

int builds_prepared_prior_preserved_arg_call_object() {
  const auto prepared = make_prepared_prior_preserved_arg_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared prior-preserved arg call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main = object::find_symbol(*module, "reload_prior_preserved_arg");
  const auto* probe = object::find_symbol(*module, "probe");
  if (text == nullptr || main == nullptr || probe == nullptr) {
    return fail("expected prior-preserved arg call object to publish text/function/probe");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[1].section != text->id ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[1].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != probe->id ||
      module->relocations[1].symbol != probe->id ||
      module->relocations[0].offset >= module->relocations[1].offset ||
      module->relocations[0].offset < main->value + 20 ||
      module->relocations[1].offset < main->value + 36) {
    return fail("expected two direct probe call relocations");
  }
  if (read_u32(text->bytes, main->value + 0) != 0xfe010113 ||
      read_u32(text->bytes, main->value + 4) != 0x00113c23 ||
      read_u32(text->bytes, main->value + 8) != 0x00913023 ||
      read_u32(text->bytes, module->relocations[0].offset - 8) !=
          0x00050493 ||
      read_u32(text->bytes, module->relocations[0].offset - 4) !=
          0x00048513 ||
      read_u32(text->bytes, module->relocations[1].offset - 8) !=
          0x00048513 ||
      read_u32(text->bytes, module->relocations[1].offset - 4) !=
      0x00048513) {
    return fail("expected preservation population/republication and later reload");
  }
  const auto epilogue_offset = main->value + main->size_bytes - 16;
  if (read_u32(text->bytes, epilogue_offset + 0) != 0x00013483 ||
      read_u32(text->bytes, epilogue_offset + 4) != 0x01813083 ||
      read_u32(text->bytes, epilogue_offset + 8) != 0x02010113 ||
      read_u32(text->bytes, epilogue_offset + 12) != 0x00008067) {
    return fail("expected s1 restore before ra restore and return");
  }
  return 0;
}

int rejects_prepared_prior_preserved_arg_call_fail_closed_shapes() {
  auto prepared = make_prepared_prior_preserved_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[1]
      .arguments[0]
      .source_selection->preserved_register_name = std::nullopt;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering") !=
      0) {
    return 1;
  }

  prepared = make_prepared_prior_preserved_arg_call_module();
  auto& selection = *prepared.call_plans.functions[0]
                         .calls[1]
                         .arguments[0]
                         .source_selection;
  selection.preservation_route =
      prepare::PreparedCallPreservationRoute::StackSlot;
  selection.preserved_register_name = std::nullopt;
  selection.preserved_register_bank = std::nullopt;
  selection.preserved_register_contiguous_width = std::nullopt;
  selection.preserved_occupied_register_names.clear();
  selection.preserved_register_placement = std::nullopt;
  selection.preserved_stack_slot_id = prepare::PreparedFrameSlotId{7};
  selection.preserved_stack_offset_bytes = std::size_t{24};
  selection.preserved_stack_size_bytes = std::size_t{8};
  selection.preserved_stack_align_bytes = std::size_t{8};
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering") !=
      0) {
    return 1;
  }

  prepared = make_prepared_prior_preserved_arg_call_module();
  prepared.frame_plan.functions[0]
      .saved_callee_registers[0]
      .slot_placement
      ->size_bytes = std::size_t{4};
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved GPR save slots") !=
      0) {
    return 1;
  }

  prepared = make_prepared_prior_preserved_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .preserved_values[0]
      .preservation_source
      .storage_kind = prepare::PreparedMoveStorageKind::None;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering") !=
      0) {
    return 1;
  }

  return 0;
}

int expect_byval_stack_slot_param_home_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_byval_param_home: RV64 object route requires a prepared permanent byval frame-slot home with matching size and alignment");
}

int expect_byval_stack_slot_lane_load_object(std::int64_t byte_offset,
                                             std::uint32_t expected_load,
                                             const char* lane_description) {
  const auto prepared = make_prepared_byval_stack_slot_param_module(byte_offset);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail(std::string{"expected prepared byval stack-slot lane load to build for "} +
                lane_description);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "byval_stack_param");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared byval parameter object to publish text/function");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      function->value != 0 || function->size_bytes != 16 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail(std::string{"expected prepared byval parameter object text layout for "} +
                lane_description);
  }
  if (read_u32(text->bytes, 0) != 0xfb010113 ||
      read_u32(text->bytes, 4) != expected_load ||
      read_u32(text->bytes, 8) != 0x05010113 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail(std::string{"expected prepared byval frame-slot load and return sequence for "} +
                lane_description);
  }
  if (!module->relocations.empty()) {
    return fail(std::string{"expected prepared byval parameter object to need no relocations for "} +
                lane_description);
  }
  return 0;
}

int builds_byval_stack_slot_param_home_object() {
  if (expect_byval_stack_slot_lane_load_object(0, 0x00012503, "offset 0") != 0) {
    return 1;
  }
  if (expect_byval_stack_slot_lane_load_object(68, 0x04412503, "offset 68") != 0) {
    return 1;
  }
  return 0;
}

int rejects_byval_stack_slot_param_home_fail_closed_shapes() {
  auto prepared = make_prepared_byval_stack_slot_param_module();
  prepared.stack_layout.objects[0].source_kind = "local";
  if (expect_byval_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.stack_layout.objects[0].permanent_home_slot = false;
  if (expect_byval_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.stack_layout.frame_slots[0].size_bytes = 68;
  if (expect_byval_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.value_locations.functions[0].value_homes[0].align_bytes =
      std::size_t{8};
  if (expect_byval_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.value_locations.functions[0].value_homes[0].slot_id = std::nullopt;
  if (expect_byval_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int expect_scalar_gpr_stack_slot_param_home_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_param_home: RV64 object route requires scalar GPR formal stack-slot homes to match prepared frame-slot facts");
}

int builds_scalar_gpr_stack_slot_param_home_object() {
  const auto prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar GPR stack-slot formal home to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "scalar_gpr_stack_param_home");
  if (text == nullptr || function == nullptr) {
    return fail("expected scalar GPR stack-slot formal object to publish text/function");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      function->value != 0 || function->size_bytes != 16 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected scalar GPR stack-slot formal object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfb010113 ||
      read_u32(text->bytes, 4) != 0x04a12023 ||
      read_u32(text->bytes, 8) != 0x05010113 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail("expected scalar GPR formal to store incoming a0 into stack slot before return");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar GPR stack-slot formal object to need no relocations");
  }
  return 0;
}

int rejects_scalar_gpr_stack_slot_param_home_fail_closed_shapes() {
  auto prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.value_locations.functions[0].value_homes[0].offset_bytes =
      std::size_t{68};
  if (expect_scalar_gpr_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.stack_layout.frame_slots[0].size_bytes = 8;
  if (expect_scalar_gpr_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.stack_layout.objects[0].source_kind = "local";
  if (expect_scalar_gpr_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.stack_layout.objects[0].address_exposed = true;
  if (expect_scalar_gpr_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.module.functions[0].params[0].abi->primary_class =
      bir::AbiValueClass::Sse;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes") !=
      0) {
    return 1;
  }

  prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.stack_layout.frame_size_bytes = 64;
  if (expect_scalar_gpr_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_gpr_stack_slot_param_module();
  prepared.value_locations.functions[0].value_homes[0].offset_bytes =
      std::size_t{2048};
  prepared.stack_layout.frame_slots[0].offset_bytes = 2048;
  if (expect_scalar_gpr_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int expect_byval_pointer_access_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing");
}

int rejects_byval_stack_slot_pointer_access_fail_closed_shapes() {
  auto prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].address.pointer_value_name =
      std::nullopt;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.value_locations.functions[0].value_homes.clear();
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes") !=
      0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.value_locations.functions[0].value_homes[0].slot_id = std::nullopt;
  if (expect_byval_stack_slot_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].address.base_kind =
      prepare::PreparedAddressBaseKind::None;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].address.byte_offset = 69;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].address.can_use_base_plus_offset =
      false;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].address_space =
      bir::AddressSpace::Tls;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].is_volatile = true;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  prepared.addressing.functions[0].accesses[0].address.align_bytes = 8;
  if (expect_byval_pointer_access_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_byval_stack_slot_param_module();
  auto* load =
      std::get_if<bir::LoadLocalInst>(&prepared.module.functions[0].blocks[0].insts[0]);
  if (load == nullptr) {
    return fail("expected mutable byval pointer fixture load");
  }
  load->result = bir::Value::named(bir::TypeKind::I128, "%t0");
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses") !=
      0) {
    return 1;
  }

  return 0;
}

int builds_prepared_fpr_formal_param_home_with_target_identity_object() {
  const auto prepared = make_prepared_fpr_formal_param_home_module(true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail(
        "expected prepared FPR formal parameter home with target identity to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "fpr_formal_param_home");
  if (text == nullptr || function == nullptr) {
    return fail(
        "expected prepared FPR formal parameter object to publish text/function");
  }
  if (text->bytes.size() != 4 || text->size_bytes != 4 ||
      function->value != 0 || function->size_bytes != 4 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared FPR formal parameter object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00008067) {
    return fail("expected prepared FPR formal parameter object to emit ret");
  }
  return 0;
}

int rejects_raw_fpr_formal_param_home_without_target_identity() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_fpr_formal_param_home_module(false),
      "unsupported_param_home: RV64 object route requires all parameters in "
      "supported GPR or prepared FPR register homes");
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

int builds_prepared_frame_slot_address_local_store_object() {
  const auto prepared = make_prepared_frame_slot_address_local_store_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared frame-slot address local store object to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* main_symbol = object::find_symbol(*result.module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected frame-slot address local store object to publish text/main");
  }
  if (!contains_u32(text->bytes, 0x01810313) ||
      !contains_u32(text->bytes, 0x00613423)) {
    return fail("expected local pointer store to materialize addi t1, sp, 24 "
                "before storing the pointer");
  }
  if (contains_u32(text->bytes, 0x00048313)) {
    return fail("expected local pointer store not to move stale s1 into t1");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected frame-slot address local store object to need no relocations");
  }
  return 0;
}

int builds_prepared_f64_local_frame_object() {
  const auto prepared = make_prepared_f64_local_frame_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared F64 local frame RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "f64_local_frame");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared F64 local frame object to publish text/function");
  }
  if (text->bytes.size() < 20 || text->size_bytes != text->bytes.size() ||
      function->value != 0 || function->size_bytes != text->bytes.size() ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared F64 local frame object text layout");
  }
  if (!contains_u32(text->bytes, 0xf2030053) ||
      !contains_u32(text->bytes, 0x00013027) ||
      !contains_u32(text->bytes, 0x00013007)) {
    return fail("expected prepared F64 local frame to materialize immediate, fsd, and fld");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared F64 local frame object to need no relocations");
  }
  return 0;
}

int builds_prepared_f32_local_frame_object() {
  const auto prepared = make_prepared_f32_local_frame_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared F32 local frame RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "f32_local_frame");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared F32 local frame object to publish text/function");
  }
  if (text->bytes.size() < 20 || text->size_bytes != text->bytes.size() ||
      function->value != 0 || function->size_bytes != text->bytes.size() ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared F32 local frame object text layout");
  }
  if (!contains_u32(text->bytes, 0xf0030053) ||
      !contains_u32(text->bytes, 0x00012027) ||
      !contains_u32(text->bytes, 0x00012007)) {
    return fail("expected prepared F32 local frame to materialize immediate, fsw, and flw");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared F32 local frame object to need no relocations");
  }
  return 0;
}

int builds_prepared_f32_i32_local_overlay_object() {
  const auto prepared = make_prepared_f32_i32_local_overlay_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared F32/I32 overlay local RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "f32_i32_overlay");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared F32/I32 overlay object to publish text/function");
  }
  if (text->bytes.size() < 28 || text->size_bytes != text->bytes.size() ||
      function->value != 0 || function->size_bytes != text->bytes.size() ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared F32/I32 overlay object text layout");
  }
  if (!contains_u32(text->bytes, 0x00612023) ||
      !contains_u32(text->bytes, 0x00012087) ||
      !contains_u32(text->bytes, 0x00012227) ||
      !contains_u32(text->bytes, 0x00412283)) {
    return fail("expected prepared overlay to encode sw, flw, fsw, and lw");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared F32/I32 overlay object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_local_subobject_frame_object() {
  const auto prepared = make_prepared_scalar_local_subobject_frame_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar local subobject RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar local subobject object to publish text/main");
  }
  if (text->bytes.size() != 28 || text->size_bytes != 28 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 28) {
    return fail("expected prepared scalar local subobject object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00500313 ||
      read_u32(text->bytes, 8) != 0x00612623 ||
      read_u32(text->bytes, 12) != 0x00c12283 ||
      read_u32(text->bytes, 16) != 0x00028513 ||
      read_u32(text->bytes, 20) != 0x01010113 ||
      read_u32(text->bytes, 24) != 0x00008067) {
    return fail("expected stack-frame sw/lw scalar local subobject sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar local subobject object to need no relocations");
  }
  return 0;
}

int rejects_prepared_f64_local_frame_fail_closed_shapes() {
  auto prepared = make_prepared_f64_local_frame_module();
  prepared.stack_layout.frame_slots[0].size_bytes = 1;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing") !=
      0) {
    return 1;
  }

  prepared = make_prepared_f64_local_frame_module();
  prepared.addressing.functions[0].accesses[0].address.can_use_base_plus_offset =
      false;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing") !=
      0) {
    return 1;
  }

  return 0;
}

int expect_scalar_local_subobject_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing");
}

int rejects_prepared_scalar_local_subobject_fail_closed_shapes() {
  auto prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].address.frame_slot_id = std::nullopt;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].address.base_kind =
      prepare::PreparedAddressBaseKind::PointerValue;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].address.byte_offset = -1;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].address.byte_offset = 8;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].address.align_bytes = 8;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].is_volatile = true;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  prepared.addressing.functions[0].accesses[0].address_space = bir::AddressSpace::Tls;
  if (expect_scalar_local_subobject_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_local_subobject_frame_module();
  auto* store =
      std::get_if<bir::StoreLocalInst>(&prepared.module.functions[0].blocks[0].insts[0]);
  if (store == nullptr) {
    return fail("expected mutable subobject fixture store");
  }
  store->value = bir::Value::immediate_f128_bits(0, 0);
  prepared.addressing.functions[0].accesses[0].address.size_bytes = 16;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses") !=
      0) {
    return 1;
  }

  return 0;
}

int builds_prepared_pointer_value_scalar_local_object() {
  const auto prepared = make_prepared_pointer_value_scalar_local_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared pointer-value scalar local RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared pointer-value local object to publish text/main");
  }
  if (text->bytes.size() != 20 || text->size_bytes != 20 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 20) {
    return fail("expected prepared pointer-value local object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00900313 ||
      read_u32(text->bytes, 4) != 0x00639123 ||
      read_u32(text->bytes, 8) != 0x00239283 ||
      read_u32(text->bytes, 12) != 0x00028513 ||
      read_u32(text->bytes, 16) != 0x00008067) {
    return fail("expected pointer-value sh/lh scalar local sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected pointer-value scalar local object to need no relocations");
  }
  return 0;
}

int builds_prepared_pointer_value_scalar_local_store_with_t1_base_object() {
  const auto prepared = make_prepared_pointer_value_scalar_local_module("t1");
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared pointer-value local store with t1 base to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared pointer-value t1-base object to publish text/main");
  }
  if (text->bytes.size() != 20 || text->size_bytes != 20 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 20) {
    return fail("expected prepared pointer-value t1-base object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00900393 ||
      read_u32(text->bytes, 4) != 0x00731123 ||
      read_u32(text->bytes, 8) != 0x00231283 ||
      read_u32(text->bytes, 12) != 0x00028513 ||
      read_u32(text->bytes, 16) != 0x00008067) {
    return fail("expected pointer-value store to avoid clobbering t1 base");
  }
  if (!module->relocations.empty()) {
    return fail("expected pointer-value t1-base object to need no relocations");
  }
  return 0;
}

int builds_prepared_pointer_value_f64_local_object() {
  const auto prepared = make_prepared_pointer_value_f64_local_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared pointer-value F64 local RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "f64_pointer_local");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared pointer-value F64 local object to publish text/function");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      function->value != 0 || function->size_bytes != 12 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared pointer-value F64 local object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x0004b027 ||
      read_u32(text->bytes, 4) != 0x0004b087 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected pointer-value F64 fsd/fld local sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected pointer-value F64 local object to need no relocations");
  }
  return 0;
}

int builds_prepared_sret_stack_pointer_store_object() {
  const auto prepared = make_prepared_sret_stack_pointer_store_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared sret stack-homed pointer store to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "sret_stack_pointer_store");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared sret pointer store object to publish text/function");
  }
  if (text->bytes.size() != 28 || text->size_bytes != 28 ||
      function->value != 0 || function->size_bytes != 28 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared sret pointer store object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00a13023 ||
      read_u32(text->bytes, 8) != 0x00013383 ||
      read_u32(text->bytes, 12) != 0x02a00313 ||
      read_u32(text->bytes, 16) != 0x0063a223 ||
      read_u32(text->bytes, 20) != 0x01010113 ||
      read_u32(text->bytes, 24) != 0x00008067) {
    return fail("expected sret a0 home publication, pointer load, indirect store, and return sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared sret pointer store object to need no relocations");
  }
  return 0;
}

int expect_pointer_value_scalar_local_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing");
}

int expect_sret_stack_pointer_store_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing");
}

int expect_sret_home_publication_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_sret_param_home: RV64 object route requires a pointer-sized permanent sret frame-slot home matching the incoming a0 formal");
}

int rejects_prepared_sret_stack_pointer_store_fail_closed_shapes() {
  auto prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.stack_layout.objects[0].source_kind = "local_slot";
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.stack_layout.objects[0].permanent_home_slot = false;
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.stack_layout.objects[0].address_exposed = false;
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.value_locations.functions[0].value_homes[0].kind =
      prepare::PreparedValueHomeKind::Register;
  prepared.value_locations.functions[0].value_homes[0].register_name =
      std::nullopt;
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.stack_layout.frame_slots[0].offset_bytes = 8;
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.stack_layout.frame_slots[0].size_bytes = 4;
  prepared.stack_layout.objects[0].size_bytes = 4;
  prepared.value_locations.functions[0].value_homes[0].size_bytes = std::size_t{4};
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.module.functions[0].params[0].abi = std::nullopt;
  if (expect_sret_home_publication_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.addressing.functions[0].accesses[0].address.can_use_base_plus_offset =
      false;
  if (expect_sret_stack_pointer_store_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.addressing.functions[0].accesses[0].address_space =
      bir::AddressSpace::Tls;
  if (expect_sret_stack_pointer_store_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.addressing.functions[0].accesses[0].is_volatile = true;
  if (expect_sret_stack_pointer_store_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.addressing.functions[0].accesses[0].address.byte_offset = 4096;
  if (expect_sret_stack_pointer_store_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_sret_stack_pointer_store_module();
  prepared.addressing.functions[0].accesses[0].address.align_bytes = 8;
  if (expect_sret_stack_pointer_store_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_pointer_value_scalar_local_fail_closed_shapes() {
  auto prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].address.pointer_value_name =
      std::nullopt;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.value_locations.functions[0].value_homes[0].register_name =
      std::nullopt;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].address.byte_offset = 4096;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].address.align_bytes = 4;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].address_space =
      bir::AddressSpace::Tls;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].is_volatile = true;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].address.can_use_base_plus_offset =
      false;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  prepared.addressing.functions[0].accesses[0].address.size_bytes = 16;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_scalar_local_module();
  auto* store =
      std::get_if<bir::StoreLocalInst>(&prepared.module.functions[0].blocks[0].insts[0]);
  if (store == nullptr) {
    return fail("expected mutable pointer-value fixture store");
  }
  store->value = bir::Value::immediate_f128_bits(0, 0);
  prepared.addressing.functions[0].accesses[0].address.size_bytes = 16;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses") !=
      0) {
    return 1;
  }

  return 0;
}

int builds_prepared_stack_slot_scalar_flow_object() {
  const auto prepared = make_prepared_stack_slot_scalar_flow_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared stack-slot scalar flow RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared stack-slot scalar object to publish text/main");
  }
  if (text->bytes.size() != 48 || text->size_bytes != 48 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 48) {
    return fail("expected prepared stack-slot scalar object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00500313 ||
      read_u32(text->bytes, 8) != 0x00612023 ||
      read_u32(text->bytes, 12) != 0x00012303 ||
      read_u32(text->bytes, 16) != 0x00612223 ||
      read_u32(text->bytes, 20) != 0x00412e03 ||
      read_u32(text->bytes, 24) != 0x00700e93 ||
      read_u32(text->bytes, 28) != 0x01de0f33 ||
      read_u32(text->bytes, 32) != 0x01e12423 ||
      read_u32(text->bytes, 36) != 0x00812503 ||
      read_u32(text->bytes, 40) != 0x01010113 ||
      read_u32(text->bytes, 44) != 0x00008067) {
    return fail("expected load/store through prepared stack-slot value homes");
  }
  if (!module->relocations.empty()) {
    return fail("expected stack-slot scalar flow object to need no relocations");
  }
  return 0;
}

int builds_prepared_stack_slot_to_gpr_move_bundle_object() {
  const auto prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared stack-slot to GPR move-bundle RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "stack_move");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared stack-slot move object to publish text/function");
  }
  if (text->bytes.size() != 20 || text->size_bytes != 20 ||
      function->value != 0 || function->size_bytes != 20 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared stack-slot move object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00811483 ||
      read_u32(text->bytes, 8) != 0x00811503 ||
      read_u32(text->bytes, 12) != 0x01010113 ||
      read_u32(text->bytes, 16) != 0x00008067) {
    return fail("expected lh s1, 8(sp) move-bundle emission before stack return");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared stack-slot move object to need no relocations");
  }
  return 0;
}

int rejects_prepared_stack_slot_to_gpr_move_bundle_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_stack_offset_bytes = 8;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_contiguous_width = 2;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .uses_cycle_temp_source = true;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  prepared.value_locations.functions[0].value_homes[0].slot_id = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  prepared.module.functions[0].return_type = bir::TypeKind::F32;
  prepared.module.functions[0].return_size_bytes = 4;
  prepared.module.functions[0].return_align_bytes = 4;
  prepared.module.functions[0].blocks[0].terminator.value =
      bir::Value::named(bir::TypeKind::F32, "%src");
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  prepared.module.functions[0].return_type = bir::TypeKind::Ptr;
  prepared.module.functions[0].return_size_bytes = 8;
  prepared.module.functions[0].return_align_bytes = 8;
  prepared.module.functions[0].blocks[0].terminator.value =
      bir::Value::named(bir::TypeKind::Ptr, "%src");
  prepared.stack_layout.frame_slots[0].size_bytes = 8;
  prepared.stack_layout.frame_slots[0].align_bytes = 8;
  prepared.value_locations.functions[0].value_homes[0].size_bytes =
      std::size_t{8};
  prepared.value_locations.functions[0].value_homes[0].align_bytes =
      std::size_t{8};
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int builds_prepared_scalar_compare_trunc_object() {
  const auto prepared = make_prepared_scalar_compare_trunc_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar compare/trunc RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar compare/trunc object to publish text/main");
  }
  if (text->bytes.size() != 40 || text->size_bytes != 40 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 40) {
    return fail("expected prepared scalar compare/trunc object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfe010113 ||
      read_u32(text->bytes, 4) != 0x0082a493 ||
      read_u32(text->bytes, 8) != 0x0014c493 ||
      read_u32(text->bytes, 12) != 0x00048f13 ||
      read_u32(text->bytes, 16) != 0x030f1f13 ||
      read_u32(text->bytes, 20) != 0x030f5f13 ||
      read_u32(text->bytes, 24) != 0x01e11a23 ||
      read_u32(text->bytes, 28) != 0x01411503 ||
      read_u32(text->bytes, 32) != 0x02010113 ||
      read_u32(text->bytes, 36) != 0x00008067) {
    return fail("expected Sge i32 compare materialization feeding i16 trunc publication");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar compare/trunc object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_ordered_compare_return_object() {
  auto prepared = make_prepared_scalar_compare_trunc_module();
  auto& function = prepared.module.functions[0];
  auto& block = function.blocks[0];
  auto* compare = std::get_if<bir::BinaryInst>(&block.insts[0]);
  if (compare == nullptr) {
    return fail("expected mutable scalar compare fixture compare");
  }
  compare->opcode = bir::BinaryOpcode::Slt;
  block.insts.resize(1);
  block.terminator.value = bir::Value::named(bir::TypeKind::I32, "%cmp");
  function.return_type = bir::TypeKind::I32;
  function.return_size_bytes = 4;
  function.return_align_bytes = 4;

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar ordered compare return RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr || text->bytes.empty()) {
    return fail("expected prepared scalar ordered compare return object to publish text/main");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar ordered compare return object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_ashr_register_object() {
  const auto prepared = make_prepared_scalar_ashr_module(bir::TypeKind::I32, false);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar ashr register RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar ashr register object to publish text/main");
  }
  if (text->bytes.size() != 20 || text->size_bytes != 20 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 20) {
    return fail("expected prepared scalar ashr register object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028e13 ||
      read_u32(text->bytes, 4) != 0x00048e93 ||
      read_u32(text->bytes, 8) != 0x41de593b ||
      read_u32(text->bytes, 12) != 0x00090513 ||
      read_u32(text->bytes, 16) != 0x00008067) {
    return fail("expected prepared scalar ashr register lowering sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar ashr register object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_ashr_immediate_object() {
  const auto prepared = make_prepared_scalar_ashr_module(bir::TypeKind::I32, true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar ashr immediate RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar ashr immediate object to publish text/main");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 16) {
    return fail("expected prepared scalar ashr immediate object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028e13 ||
      read_u32(text->bytes, 4) != 0x41fe591b ||
      read_u32(text->bytes, 8) != 0x00090513 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail("expected prepared scalar ashr immediate lowering sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar ashr immediate object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_ashr_i64_register_object() {
  const auto prepared = make_prepared_scalar_ashr_module(bir::TypeKind::I64, false);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar ashr i64 register RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar ashr i64 register object to publish text/main");
  }
  if (text->bytes.size() != 20 || text->size_bytes != 20 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 20) {
    return fail("expected prepared scalar ashr i64 register object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028e13 ||
      read_u32(text->bytes, 4) != 0x00048e93 ||
      read_u32(text->bytes, 8) != 0x41de5933 ||
      read_u32(text->bytes, 12) != 0x00090513 ||
      read_u32(text->bytes, 16) != 0x00008067) {
    return fail("expected prepared scalar ashr i64 register lowering sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar ashr i64 register object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_ashr_i64_immediate_object() {
  const auto prepared = make_prepared_scalar_ashr_module(bir::TypeKind::I64, true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared scalar ashr i64 immediate RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared scalar ashr i64 immediate object to publish text/main");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 16) {
    return fail("expected prepared scalar ashr i64 immediate object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028e13 ||
      read_u32(text->bytes, 4) != 0x41fe5913 ||
      read_u32(text->bytes, 8) != 0x00090513 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail("expected prepared scalar ashr i64 immediate lowering sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected scalar ashr i64 immediate object to need no relocations");
  }
  return 0;
}

int rejects_prepared_scalar_ashr_invalid_immediate_object() {
  constexpr const char* diagnostic =
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";

  auto prepared = make_prepared_scalar_ashr_module(bir::TypeKind::I32, true);
  auto* shift = std::get_if<bir::BinaryInst>(
      &prepared.module.functions.front().blocks.front().insts.front());
  if (shift == nullptr) {
    return fail("expected prepared scalar ashr fixture to contain a binary instruction");
  }
  shift->rhs = bir::Value::immediate_i32(32);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }
  return 0;
}

int builds_prepared_scalar_divrem_object() {
  struct Case {
    bir::BinaryOpcode opcode;
    bir::TypeKind type;
    std::uint32_t instruction;
    const char* name;
  };
  constexpr Case cases[] = {
      {bir::BinaryOpcode::SDiv, bir::TypeKind::I32, 0x03de493b, "divw"},
      {bir::BinaryOpcode::SDiv, bir::TypeKind::I64, 0x03de4933, "div"},
      {bir::BinaryOpcode::SRem, bir::TypeKind::I32, 0x03de693b, "remw"},
      {bir::BinaryOpcode::SRem, bir::TypeKind::I64, 0x03de6933, "rem"},
      {bir::BinaryOpcode::URem, bir::TypeKind::I32, 0x03de793b, "remuw"},
      {bir::BinaryOpcode::URem, bir::TypeKind::I64, 0x03de7933, "remu"},
  };
  for (const auto& test_case : cases) {
    const auto prepared =
        make_prepared_scalar_binary_module(test_case.opcode, test_case.type);
    const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
    if (!module.has_value()) {
      return fail(std::string{"expected prepared scalar "} + test_case.name +
                  " RV64 object module to build");
    }
    const auto* text = object::find_section(*module, ".text");
    const auto* main_symbol = object::find_symbol(*module, "main");
    if (text == nullptr || main_symbol == nullptr) {
      return fail(std::string{"expected prepared scalar "} + test_case.name +
                  " object to publish text/main");
    }
    if (text->bytes.size() != 20 || text->size_bytes != 20 ||
        main_symbol->value != 0 || main_symbol->size_bytes != 20) {
      return fail(std::string{"expected prepared scalar "} + test_case.name +
                  " object text layout");
    }
    if (read_u32(text->bytes, 0) != 0x00028e13 ||
        read_u32(text->bytes, 4) != 0x00048e93 ||
        read_u32(text->bytes, 8) != test_case.instruction ||
        read_u32(text->bytes, 12) != 0x00090513 ||
        read_u32(text->bytes, 16) != 0x00008067) {
      return fail(std::string{"expected prepared scalar "} + test_case.name +
                  " lowering sequence");
    }
    if (!module->relocations.empty()) {
      return fail(std::string{"expected scalar "} + test_case.name +
                  " object to need no relocations");
    }
  }
  return 0;
}

int rejects_prepared_scalar_compare_publication_missing_home() {
  constexpr const char* diagnostic =
      "unsupported_scalar_compare_publication: RV64 object route requires prepared scalar compare result homes and materializable operands";

  auto prepared = make_prepared_scalar_compare_trunc_module();
  prepared.value_locations.functions[0].value_homes.erase(
      prepared.value_locations.functions[0].value_homes.begin() + 1);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int builds_prepared_join_transfer_select_materialization_object() {
  const auto prepared = make_prepared_join_transfer_select_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared join-transfer select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* true_label = object::find_symbol(*module, ".Lmain_entry_select_0_true");
  const auto* end_label = object::find_symbol(*module, ".Lmain_entry_select_0_end");
  if (text == nullptr || main_symbol == nullptr || true_label == nullptr ||
      end_label == nullptr) {
    return fail("expected prepared select object to publish text/main/select labels");
  }
  if (text->bytes.size() != 44 || text->size_bytes != 44 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 44 ||
      true_label->value != 24 || end_label->value != 28) {
    return fail("expected prepared select materialization object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00028e13 ||
      read_u32(text->bytes, 8) != 0x00100e93 ||
      read_u32(text->bytes, 12) != 0x01de1063 ||
      read_u32(text->bytes, 16) != 0x00030f13 ||
      read_u32(text->bytes, 20) != 0x0000006f ||
      read_u32(text->bytes, 24) != 0x00100f13 ||
      read_u32(text->bytes, 28) != 0x01e12023 ||
      read_u32(text->bytes, 32) != 0x00012503 ||
      read_u32(text->bytes, 36) != 0x01010113 ||
      read_u32(text->bytes, 40) != 0x00008067) {
    return fail("expected prepared select compare/branch/materialize/store sequence");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 12 ||
      module->relocations[0].type != R_RISCV_BRANCH ||
      module->relocations[0].symbol != true_label->id ||
      module->relocations[0].addend != 0 ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 20 ||
      module->relocations[1].type != R_RISCV_JAL ||
      module->relocations[1].symbol != end_label->id ||
      module->relocations[1].addend != 0) {
    return fail("expected prepared select local branch/jump relocations");
  }
  return 0;
}

int builds_prepared_normalized_sle_select_materialization_object() {
  auto prepared = make_prepared_join_transfer_select_module();
  auto* select = std::get_if<bir::SelectInst>(
      &prepared.module.functions.front().blocks.front().insts.front());
  if (select == nullptr) {
    return fail("expected prepared select fixture to contain a select");
  }
  select->predicate = bir::BinaryOpcode::Sle;

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared sle select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* true_label = object::find_symbol(*module, ".Lmain_entry_select_0_true");
  const auto* end_label = object::find_symbol(*module, ".Lmain_entry_select_0_end");
  if (text == nullptr || true_label == nullptr || end_label == nullptr) {
    return fail("expected prepared sle select object to publish select labels");
  }
  if (text->bytes.size() != 44 || text->size_bytes != 44 ||
      true_label->value != 24 || end_label->value != 28) {
    return fail("expected normalized sle select materialization object text layout");
  }
  if (read_u32(text->bytes, 4) != 0x00100e13 ||
      read_u32(text->bytes, 8) != 0x00028e93 ||
      read_u32(text->bytes, 12) != 0x01de5063) {
    return fail("expected sle select to branch as bge with swapped operands");
  }
  return 0;
}

int skips_published_prepared_join_transfer_select_carrier_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_published_copies_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected published-copy prepared join-transfer select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* true_copy_label = object::find_symbol(*module, ".Lmain_pred_true");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  const auto* select_true_label = object::find_symbol(*module, ".Lmain_join_select_0_true");
  const auto* select_end_label = object::find_symbol(*module, ".Lmain_join_select_0_end");
  if (text == nullptr || main_symbol == nullptr || true_copy_label == nullptr ||
      false_copy_label == nullptr || join_label == nullptr) {
    return fail("expected published-copy join object to publish text/main/block labels");
  }
  if (select_true_label != nullptr || select_end_label != nullptr) {
    return fail("expected published-copy join select carrier to avoid local select labels");
  }
  if (text->bytes.size() != 24 || text->size_bytes != 24 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 24 ||
      true_copy_label->value != 4 || false_copy_label->value != 12 ||
      join_label->value != 20) {
    return fail("expected published-copy join object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x0000006f ||
      read_u32(text->bytes, 4) != 0x00100513 ||
      read_u32(text->bytes, 8) != 0x0000006f ||
      read_u32(text->bytes, 12) != 0x00030513 ||
      read_u32(text->bytes, 16) != 0x0000006f ||
      read_u32(text->bytes, 20) != 0x00008067) {
    return fail("expected predecessor terminators to publish select edge copies before jumps");
  }
  bool saw_select_local_relocation = false;
  for (const auto& relocation : module->relocations) {
    if (relocation.type != R_RISCV_JAL && relocation.type != R_RISCV_BRANCH) {
      continue;
    }
    const auto symbol_it =
        std::find_if(module->symbols.begin(),
                     module->symbols.end(),
                     [&](const object::SymbolRecord& symbol) {
                       return symbol.id == relocation.symbol;
                     });
    if (symbol_it == module->symbols.end()) {
      continue;
    }
    saw_select_local_relocation =
        saw_select_local_relocation ||
        symbol_it->name.find("_select_") != std::string::npos;
  }
  if (saw_select_local_relocation) {
    return fail("expected published-copy join select carrier to need no select relocations");
  }
  return 0;
}

int materializes_published_prepared_join_transfer_select_edge_compare_source_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_edge_compare_source_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected edge-compare source prepared join-transfer select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* true_copy_label = object::find_symbol(*module, ".Lmain_pred_true");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  const auto* select_true_label = object::find_symbol(*module, ".Lmain_join_select_1_true");
  const auto* select_end_label = object::find_symbol(*module, ".Lmain_join_select_1_end");
  if (text == nullptr || main_symbol == nullptr || true_copy_label == nullptr ||
      false_copy_label == nullptr || join_label == nullptr) {
    return fail("expected edge-compare join object to publish text/main/block labels");
  }
  if (select_true_label != nullptr || select_end_label != nullptr) {
    return fail("expected edge-compare join select carrier to avoid local select labels");
  }
  if (text->bytes.size() != 36 || text->size_bytes != 36 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 36 ||
      true_copy_label->value != 4 || false_copy_label->value != 12 ||
      join_label->value != 32) {
    return fail("expected edge-compare join object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x0000006f ||
      read_u32(text->bytes, 4) != 0x00100513 ||
      read_u32(text->bytes, 8) != 0x0000006f ||
      read_u32(text->bytes, 12) != 0x00030e13 ||
      read_u32(text->bytes, 16) != 0x00000e93 ||
      read_u32(text->bytes, 20) != 0x01de4533 ||
      read_u32(text->bytes, 24) != 0x00a03533 ||
      read_u32(text->bytes, 28) != 0x0000006f ||
      read_u32(text->bytes, 32) != 0x00008067) {
    return fail("expected RHS predecessor edge to materialize compare directly into select result");
  }
  return 0;
}

int materializes_published_prepared_join_transfer_select_dependent_edge_compare_source_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_dependent_edge_compare_source_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected dependent edge-compare source prepared join-transfer select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  if (text == nullptr || main_symbol == nullptr || false_copy_label == nullptr ||
      join_label == nullptr) {
    return fail("expected dependent edge-compare join object to publish text/main/block labels");
  }
  if (text->bytes.size() != 76 || text->size_bytes != 76 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 76 ||
      false_copy_label->value != 12 || join_label->value != 52) {
    return fail("expected dependent edge-compare join object text layout");
  }
  if (read_u32(text->bytes, 12) != 0x00030e13 ||
      read_u32(text->bytes, 16) != 0x00300e93 ||
      read_u32(text->bytes, 20) != 0x01de5933 ||
      read_u32(text->bytes, 24) != 0x00797493 ||
      read_u32(text->bytes, 28) != 0x00048913 ||
      read_u32(text->bytes, 32) != 0x00090e13 ||
      read_u32(text->bytes, 36) != 0x00500e93 ||
      read_u32(text->bytes, 40) != 0x01de4533 ||
      read_u32(text->bytes, 44) != 0x00a03533 ||
      read_u32(text->bytes, 48) != 0x0000006f) {
    return fail("expected dependent RHS predecessor edge to materialize producer chain before compare");
  }
  return 0;
}

int rejects_published_prepared_join_transfer_select_ambiguous_publications_object() {
  auto stack_source = make_prepared_join_transfer_select_with_published_copies_module();
  auto& false_source_home = stack_source.value_locations.functions.front().value_homes.at(1);
  false_source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  false_source_home.register_name.reset();
  false_source_home.target_register_identity.reset();
  false_source_home.slot_id = prepare::PreparedFrameSlotId{7};
  false_source_home.offset_bytes = 0;
  false_source_home.size_bytes = std::size_t{4};
  false_source_home.align_bytes = std::size_t{4};
  stack_source.stack_layout.frame_size_bytes = 4;
  stack_source.stack_layout.frame_alignment_bytes = 4;
  stack_source.stack_layout.frame_slots = {prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{7},
      .function_name = stack_source.names.function_names.find("main"),
      .offset_bytes = 0,
      .size_bytes = 4,
      .align_bytes = 4,
  }};
  if (rv64::build_rv64_prepared_text_object_module(stack_source).has_value()) {
    return fail("expected published-copy select object path to reject stack-source publication");
  }

  auto non_select_carrier =
      make_prepared_join_transfer_select_with_published_copies_module();
  non_select_carrier.control_flow.functions.front()
      .parallel_copy_bundles.front()
      .moves.front()
      .carrier_kind = prepare::PreparedJoinTransferCarrierKind::None;
  if (rv64::build_rv64_prepared_text_object_module(non_select_carrier).has_value()) {
    return fail("expected published-copy select object path to reject non-select carrier publication");
  }

  auto stack_edge_source =
      make_prepared_join_transfer_select_with_edge_compare_source_module();
  auto& compare_operand_home =
      stack_edge_source.value_locations.functions.front().value_homes.at(1);
  compare_operand_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  compare_operand_home.register_name.reset();
  compare_operand_home.target_register_identity.reset();
  compare_operand_home.slot_id = prepare::PreparedFrameSlotId{8};
  compare_operand_home.offset_bytes = 0;
  compare_operand_home.size_bytes = std::size_t{4};
  compare_operand_home.align_bytes = std::size_t{4};
  stack_edge_source.stack_layout.frame_size_bytes = 4;
  stack_edge_source.stack_layout.frame_alignment_bytes = 4;
  stack_edge_source.stack_layout.frame_slots = {prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{8},
      .function_name = stack_edge_source.names.function_names.find("main"),
      .offset_bytes = 0,
      .size_bytes = 4,
      .align_bytes = 4,
  }};
  if (rv64::build_rv64_prepared_text_object_module(stack_edge_source).has_value()) {
    return fail("expected edge-compare select object path to reject stack operand source");
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

int builds_prepared_frame_slot_value_arg_call_object() {
  const auto prepared = make_prepared_frame_slot_value_arg_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared frame-slot-value arg call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* sink = object::find_symbol(*module, "sink");
  const auto* main = object::find_symbol(*module, "main");
  if (text == nullptr || sink == nullptr || main == nullptr) {
    return fail("expected frame-slot-value arg call object to publish text/functions");
  }
  if (sink->section != main->section || sink->section != text->id ||
      sink->size_bytes == 0 || main->size_bytes == 0 ||
      main->value < sink->value + sink->size_bytes) {
    return fail("expected frame-slot-value arg call object text layout");
  }
  const std::size_t main_offset = main->value;
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset < main_offset + 4 ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != sink->id) {
    return fail("expected frame-slot-value same-module call relocation");
  }
  const auto store = read_u32(text->bytes, module->relocations[0].offset - 8);
  if (read_u32(text->bytes, main_offset + 0) != 0xfd010113 ||
      read_u32(text->bytes, main_offset + 4) != 0x02113423 ||
      (store & 0x7fU) != 0x23U || ((store >> 12) & 0x7U) != 3U ||
      ((store >> 15) & 0x1fU) != 2U || ((store >> 20) & 0x1fU) != 30U ||
      read_u32(text->bytes, module->relocations[0].offset - 4) != 0x01013503) {
    return fail("expected frame-slot payload store and reload into a0 before call");
  }
  return 0;
}

int expect_frame_slot_value_arg_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
}

int rejects_prepared_frame_slot_value_arg_call_fail_closed_shapes() {
  auto prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].source_selection->kind =
      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].source_slot_id =
      std::nullopt;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.value_locations.functions[1].value_homes[0].slot_id = std::nullopt;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].destination_register_bank =
      prepare::PreparedRegisterBank::Fpr;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  if (auto* call = std::get_if<bir::CallInst>(
          &prepared.module.functions[1].blocks[0].insts[1])) {
    call->arg_types[0] = bir::TypeKind::F32;
  }
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_align_bytes = 16;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.value_locations.functions[1].value_homes[0].offset_bytes = 32;
  prepared.call_plans.functions[0].calls[0].arguments[0].source_stack_offset_bytes =
      32;
  prepared.stack_layout.frame_slots[0].offset_bytes = 32;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.value_locations.functions[1].value_homes[0].offset_bytes = 2048;
  prepared.call_plans.functions[0].calls[0].arguments[0].source_stack_offset_bytes =
      2048;
  prepared.stack_layout.frame_slots[0].offset_bytes = 2048;
  prepared.frame_plan.functions[1].frame_size_bytes = 2056;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame") !=
      0) {
    return 1;
  }

  return 0;
}

int builds_prepared_frame_slot_address_arg_call_object() {
  const auto prepared = make_prepared_frame_slot_address_arg_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared frame-slot-address arg call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* sink = object::find_symbol(*module, "sink");
  const auto* main = object::find_symbol(*module, "main");
  if (text == nullptr || sink == nullptr || main == nullptr) {
    return fail("expected frame-slot-address arg call object to publish text/functions");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != sink->id ||
      module->relocations[0].offset < main->value + 24) {
    return fail("expected frame-slot-address same-module call relocation");
  }
  if (read_u32(text->bytes, module->relocations[0].offset - 24) !=
          0x00048313 ||
      read_u32(text->bytes, module->relocations[0].offset - 20) !=
          0x00613c23 ||
      read_u32(text->bytes, module->relocations[0].offset - 16) !=
          0x01810513 ||
      read_u32(text->bytes, module->relocations[0].offset - 12) !=
          0x00048313 ||
      read_u32(text->bytes, module->relocations[0].offset - 8) !=
          0x02613023 ||
      read_u32(text->bytes, module->relocations[0].offset - 4) !=
          0x02010593) {
    return fail("expected initialized va_list payload publication before frame-slot-address args");
  }
  return 0;
}

int builds_prepared_frame_slot_address_arg_call_load_local_payload_object() {
  const auto prepared =
      make_prepared_frame_slot_address_arg_call_load_local_payload_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared frame-slot-address load-local payload arg call RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* sink = object::find_symbol(*module, "sink");
  const auto* main = object::find_symbol(*module, "main");
  if (text == nullptr || sink == nullptr || main == nullptr) {
    return fail("expected frame-slot-address load-local payload object to publish text/functions");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != sink->id ||
      module->relocations[0].offset < main->value + 24) {
    return fail("expected frame-slot-address load-local payload same-module call relocation");
  }
  if (read_u32(text->bytes, module->relocations[0].offset - 24) !=
          0x00090313 ||
      read_u32(text->bytes, module->relocations[0].offset - 20) !=
          0x00613c23 ||
      read_u32(text->bytes, module->relocations[0].offset - 16) !=
          0x01810513 ||
      read_u32(text->bytes, module->relocations[0].offset - 12) !=
          0x00090313 ||
      read_u32(text->bytes, module->relocations[0].offset - 8) !=
          0x02613023 ||
      read_u32(text->bytes, module->relocations[0].offset - 4) !=
          0x02010593) {
    return fail("expected load-local publication payload, not storage address, before frame-slot-address args");
  }
  return 0;
}

int records_prepared_frame_slot_address_arg_missing_publication_need() {
  const auto prepared = make_prepared_frame_slot_address_arg_call_module();
  const auto& call = prepared.call_plans.functions[0].calls[0];
  const auto x_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          call.arguments[0]);
  const auto y_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          call.arguments[1]);
  if (!x_need.available || !y_need.available ||
      x_need.kind != prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
                         FrameSlotAddress ||
      y_need.kind != prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
                         FrameSlotAddress ||
      x_need.source_value_id != prepare::PreparedValueId{14} ||
      y_need.source_value_id != prepare::PreparedValueId{15} ||
      x_need.source_selection == nullptr || y_need.source_selection == nullptr ||
      !x_need.source_materializes_address || !y_need.source_materializes_address ||
      x_need.may_emit_local_aggregate_address_payload ||
      y_need.may_emit_local_aggregate_address_payload) {
    return fail("expected frame-slot-address args to expose missing-publication need without claiming payload authority");
  }
  return 0;
}

void route_frame_slot_address_args_through_prepared_call_emit(
    prepare::PreparedBirModule& prepared) {
  auto& call_plan = prepared.call_plans.functions[0].calls[0];
  call_plan.preserved_values.clear();
  auto& args = call_plan.arguments;
  args[0].source_encoding = prepare::PreparedStorageEncodingKind::Register;
  args[0].source_register_name = std::string{"s1"};
  args[0].source_register_bank = prepare::PreparedRegisterBank::Gpr;
  args[1].source_encoding = prepare::PreparedStorageEncodingKind::Register;
  args[1].source_register_name = std::string{"s2"};
  args[1].source_register_bank = prepare::PreparedRegisterBank::Gpr;
}

std::optional<std::string> emit_prepared_frame_slot_address_arg_call_text(
    const prepare::PreparedBirModule& prepared) {
  const auto main_name = prepared.names.function_names.find("main");
  const auto block_label = prepared.names.block_labels.find("entry");
  const auto& call = std::get<bir::CallInst>(
      prepared.module.functions[1].blocks[0].insts[0]);
  const auto lookups =
      prepare::make_prepared_function_lookups(prepared,
                                              prepared.control_flow.functions[1]);
  const rv64::PreparedCurrentInstructionContext context{
      .names = prepared.names,
      .lookups = &lookups,
      .block_label = block_label,
      .instruction_index = 0,
  };
  return rv64::emit_riscv_simple_call(prepared, main_name, call, 0, context);
}

int emits_prepared_frame_slot_address_arg_call_from_selected_storage_facts() {
  auto prepared = make_prepared_frame_slot_address_arg_call_module();
  route_frame_slot_address_args_through_prepared_call_emit(prepared);

  const auto emitted = emit_prepared_frame_slot_address_arg_call_text(prepared);
  if (!emitted.has_value()) {
    return fail("expected prepared call emitter to lower verified frame-slot-address args");
  }
  if (emitted->find("    addi a0, sp, 24\n") == std::string::npos ||
      emitted->find("    addi a1, sp, 32\n") == std::string::npos ||
      emitted->find("    call sink\n") == std::string::npos) {
    return fail("expected prepared call emitter to use verified selected local storage offsets");
  }
  return 0;
}

int expect_prepared_frame_slot_address_arg_emit_rejection(
    const prepare::PreparedBirModule& prepared) {
  const auto emitted = emit_prepared_frame_slot_address_arg_call_text(prepared);
  if (emitted.has_value()) {
    return fail("expected prepared call emitter to fail closed on selected local storage facts");
  }
  return 0;
}

int rejects_prepared_frame_slot_address_arg_emit_selected_storage_fail_closed() {
  auto prepared = make_prepared_frame_slot_address_arg_call_module();
  route_frame_slot_address_args_through_prepared_call_emit(prepared);
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_size_bytes = std::nullopt;
  if (expect_prepared_frame_slot_address_arg_emit_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  route_frame_slot_address_args_through_prepared_call_emit(prepared);
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_align_bytes = std::nullopt;
  if (expect_prepared_frame_slot_address_arg_emit_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  route_frame_slot_address_args_through_prepared_call_emit(prepared);
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_size_bytes = 4;
  if (expect_prepared_frame_slot_address_arg_emit_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  route_frame_slot_address_args_through_prepared_call_emit(prepared);
  prepared.stack_layout.frame_slots[0].offset_bytes = 28;
  if (expect_prepared_frame_slot_address_arg_emit_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
}

int expect_frame_slot_address_arg_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
}

int rejects_prepared_frame_slot_address_arg_call_fail_closed_shapes() {
  auto prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts.clear();
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.store_source_publications.records.clear();
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts.push_back(
      prepared.call_argument_value_publications.facts[0]);
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  auto ambiguous = prepared.call_argument_value_publications.facts[0];
  ambiguous.payload_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.y");
  ambiguous.payload_value_id = prepare::PreparedValueId{15};
  ambiguous.payload_value_name = prepared.names.value_names.find("%lv.y");
  prepared.call_argument_value_publications.facts.push_back(ambiguous);
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts[0]
      .destination_stack_offset_bytes = 40;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts[0]
      .source_store_instruction_index = 1;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts[0].payload_value =
      bir::Value::named(bir::TypeKind::I64, "%lv.src");
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts[0].payload_value =
      bir::Value::named(bir::TypeKind::Ptr, "%missing.payload");
  prepared.call_argument_value_publications.facts[0].payload_value_name =
      prepared.names.value_names.intern("%missing.payload");
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_argument_value_publications.facts[0].payload_value =
      bir::Value::named(bir::TypeKind::Ptr, "%lv.x");
  prepared.call_argument_value_publications.facts[0].payload_value_id =
      prepare::PreparedValueId{14};
  prepared.call_argument_value_publications.facts[0].payload_value_name =
      prepared.names.value_names.find("%lv.x");
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_load_local_payload_module();
  static const bir::LoadLocalInst mismatched_load_local_payload{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.y"),
      .slot_name = "%lv.src",
      .align_bytes = 8,
  };
  prepared.store_source_publications.records[0].plan.source_load_local =
      &mismatched_load_local_payload;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->kind =
      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].source_value_id =
      std::nullopt;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].source_slot_id =
      std::nullopt;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .preserved_values[0]
      .preservation_destination.stack_offset_bytes = 56;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_slot_id = std::nullopt;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->address_materialization_frame_slot_id = std::nullopt;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_value_id = prepare::PreparedValueId{99};
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_home_kind = prepare::PreparedValueHomeKind::Register;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{8};
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.addressing.functions[0].address_materializations.clear();
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  auto duplicate = prepared.addressing.functions[0].address_materializations[0];
  duplicate.byte_offset = 32;
  prepared.addressing.functions[0].address_materializations.push_back(duplicate);
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  duplicate = prepared.addressing.functions[0].address_materializations[0];
  prepared.addressing.functions[0].address_materializations.push_back(duplicate);
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.addressing.functions[0].address_materializations[0].kind =
      prepare::PreparedAddressMaterializationKind::DirectGlobal;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.addressing.functions[0].address_materializations[0].address_space =
      bir::AddressSpace::Tls;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.addressing.functions[0].address_materializations[0].is_thread_local = true;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].destination_contiguous_width =
      2;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0].calls[0].arguments[0].destination_register_bank =
      prepare::PreparedRegisterBank::Fpr;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.frame_plan.functions[1].has_dynamic_stack = true;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.frame_plan.functions[1].uses_frame_pointer_for_fixed_slots = true;
  if (expect_frame_slot_address_arg_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->address_materialization_byte_offset = 2048;
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->source_stack_offset_bytes = 2048;
  prepared.addressing.functions[0].address_materializations[0].byte_offset = 2048;
  prepared.stack_layout.frame_slots[0].offset_bytes = 2048;
  prepared.frame_plan.functions[1].frame_size_bytes = 2050;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame") !=
      0) {
    return 1;
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
  if (bytes.size() != 16 || read_u64(bytes, 0) != 0x0003040b1420033full ||
      read_u32(bytes, 8) != 0x00000513 ||
      read_u32(bytes, 12) != 0x00008067) {
    return fail("expected RV64 line encoder to preserve canonical object bytes");
  }
  return 0;
}

int rejects_rv64_line_core_out_of_range_jal_immediate() {
  const rv64::Rv64AsmRegister ra{
      .bank = rv64::Rv64AsmRegisterBank::Gpr,
      .physical_index = 1,
  };
  for (const std::int64_t immediate : {
           static_cast<std::int64_t>(1 << 20),
           static_cast<std::int64_t>(-(1 << 20) - 2),
       }) {
    const auto line = rv64::Rv64AsmLine{rv64::Rv64JumpLine{
        .destination = ra,
        .immediate = immediate,
    }};
    if (rv64::encode_rv64_asm_line(line).has_value()) {
      return fail("expected RV64 line encoder to reject out-of-range jal immediate");
    }
  }
  return 0;
}

int parses_substituted_prepared_inline_asm_insn_d_with_line_core() {
  const auto carrier = make_prepared_insn_d_carrier();
  const auto substituted = rv64::substitute_prepared_riscv_inline_asm_operands(carrier);
  if (!substituted.has_value() ||
      *substituted != ".insn.d 10, 11, v20, v4, v6, v8, 3") {
    return fail("expected prepared .insn.d carrier to substitute to canonical text");
  }

  const auto parsed = rv64::parse_rv64_asm_line(*substituted);
  if (!parsed.has_value() ||
      !std::holds_alternative<rv64::Rv64InsnDLine>(*parsed)) {
    return fail("expected line core to parse substituted prepared .insn.d text");
  }
  const auto encoded = rv64::encode_rv64_asm_line(*parsed);
  if (!encoded.has_value() || encoded->size() != 8 ||
      read_u64(*encoded, 0) != 0x0003080b14620a3full) {
    return fail("expected line core to encode substituted prepared .insn.d text");
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
  if (*encoded != 0x0003080b14620a3full) {
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
  if (read_u64(text->bytes, 0) != 0x0003080b14620a3full ||
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
  if (read_u64(text->bytes, 0) != 0x0003080b14620a3full ||
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
  if (read_u64(text->bytes, 0) != 0x0003080b14620a3full ||
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
      read_u64(text->bytes, 4) != 0x0003080b14620a3full ||
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

int emits_prepared_string_constant_object_storage() {
  auto prepared = make_prepared_direct_call_module();
  const auto text_name = prepared.module.names.texts.intern(".LC0");
  prepared.module.string_constants.push_back(bir::StringConstant{
      .name = ".LC0",
      .name_id = text_name,
      .bytes = "hello",
      .align_bytes = 1,
  });

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit string constants");
  }
  const auto* rodata = object::find_section(*module, ".rodata");
  if (rodata == nullptr || rodata->writable || rodata->executable ||
      rodata->align_bytes != 1 ||
      rodata->bytes !=
          std::vector<std::uint8_t>{'h', 'e', 'l', 'l', 'o', 0}) {
    return fail("expected prepared string bytes in read-only data");
  }
  const auto* symbol = object::find_symbol(*module, ".LC0");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Local ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{rodata->id} ||
      symbol->value != 0 || symbol->size_bytes != 6) {
    return fail("expected prepared string object symbol in rodata");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize prepared string object");
  }
  return 0;
}

int rejects_prepared_global_memory_without_prepared_access() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_global_load_module(false),
      "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing");
}

int emits_prepared_global_aggregate_lane_load_from_explicit_facts() {
  const auto prepared = make_prepared_global_aggregate_lane_load_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit aggregate global lane load");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* global_symbol = object::find_symbol(*module, "aggregate_lanes");
  const auto* auipc_label =
      object::find_symbol(*module, ".Lpcrel_hi_global_load_1_1_0");
  if (text == nullptr || rodata == nullptr || global_symbol == nullptr ||
      auipc_label == nullptr) {
    return fail("expected text, rodata, aggregate symbol, and lane AUIPC label");
  }
  if (rodata->bytes.size() != 72 || rodata->bytes[68] != 17 ||
      rodata->bytes[69] != 0 || rodata->bytes[70] != 0 ||
      rodata->bytes[71] != 0) {
    return fail("expected aggregate global storage to publish lane bytes");
  }
  if (text->bytes.size() < 12) {
    return fail("expected PC-relative address materialization and lane load");
  }
  const auto load = read_u32(text->bytes, 8);
  if ((load & 0x7fU) != 0x03U || ((load >> 12) & 0x7U) != 2U ||
      ((load >> 20) & 0xfffU) != 68U) {
    return fail("expected prepared aggregate lane load to encode lw offset 68");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{rodata->id} ||
      global_symbol->value != 0 || global_symbol->size_bytes != 72) {
    return fail("expected aggregate lane relocation target to be a defined object");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != global_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id) {
    return fail("expected prepared aggregate lane global relocation pair");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize aggregate lane load");
  }
  return 0;
}

int rejects_raw_load_local_global_address_lane_without_prepared_access() {
  return expect_prepared_rejection_diagnostic(
      make_raw_global_address_load_local_lane_module(),
      "unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes");
}

int builds_prepared_i16_local_store_object() {
  const auto prepared = make_prepared_i16_local_store_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared i16 local-store RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared i16 local-store object to publish text/main");
  }
  if (text->bytes.size() != 24 || text->size_bytes != 24 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 24) {
    return fail("expected prepared i16 local-store object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00700313 ||
      read_u32(text->bytes, 8) != 0x00611023 ||
      read_u32(text->bytes, 12) != 0x00000513 ||
      read_u32(text->bytes, 16) != 0x01010113 ||
      read_u32(text->bytes, 20) != 0x00008067) {
    return fail("expected stack-frame sh i16 local-store object sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected i16 local-store object to need no relocations");
  }
  return 0;
}

int builds_prepared_fpr_fpext_object() {
  const auto prepared = make_prepared_fpr_fpext_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared FPR fpext RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "fpr_fpext");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared FPR fpext object to publish text/function");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      function->value != 0 || function->size_bytes != 8 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared FPR fpext object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x420505d3 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fcvt.d.s fa1, fa0, rne followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared FPR fpext object to need no relocations");
  }
  return 0;
}

int builds_prepared_fpr_fptrunc_object() {
  const auto prepared = make_prepared_fpr_fptrunc_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared FPR fptrunc RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "fpr_fptrunc");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared FPR fptrunc object to publish text/function");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      function->value != 0 || function->size_bytes != 8 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared FPR fptrunc object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x401505d3 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fcvt.s.d fa1, fa0, rne followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared FPR fptrunc object to need no relocations");
  }
  return 0;
}

int builds_prepared_formal_fpr_fpext_to_ft0_object() {
  const auto prepared = make_prepared_formal_fpr_fpext_to_ft0_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared formal FPR fpext to ft0 RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "formal_fpr_fpext_to_ft0");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared formal FPR fpext object to publish text/function");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      function->value != 0 || function->size_bytes != 8 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared formal FPR fpext object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x42050053 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fcvt.d.s ft0, fa0, rne followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared formal FPR fpext object to need no relocations");
  }
  return 0;
}

int builds_prepared_sitofp_i32_immediate_to_f64_object() {
  const auto prepared = make_prepared_sitofp_i32_immediate_to_f64_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared SIToFP i32 immediate to F64 RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function =
      object::find_symbol(*module, "sitofp_i32_immediate_to_f64");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared SIToFP object to publish text/function");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      function->value != 0 || function->size_bytes != 12 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared SIToFP object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00000293 ||
      read_u32(text->bytes, 4) != 0xd2028053 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected addi t0, zero, 0; fcvt.d.w ft0, t0, rne; ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared SIToFP object to need no relocations");
  }
  return 0;
}

int builds_prepared_before_return_fpr_f32_abi_move_object() {
  const auto prepared = make_prepared_before_return_fpr_f32_abi_move_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared before-return F32 FPR ABI move object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "fpr_return_move");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared before-return FPR move object to publish text/function");
  }
  if (text->bytes.size() != 8 || function->size_bytes != 8) {
    return fail("expected prepared before-return FPR move text layout");
  }
  if (read_u32(text->bytes, 0) != 0x20000553 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fmv.s fa0, ft0 followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared before-return FPR move object to need no relocations");
  }
  return 0;
}

int builds_prepared_before_return_fpr_f64_abi_move_object() {
  const auto prepared = make_prepared_before_return_fpr_f64_abi_move_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared before-return F64 FPR ABI move object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "fpr_return_move");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared before-return FPR move object to publish text/function");
  }
  if (text->bytes.size() != 8 || function->size_bytes != 8) {
    return fail("expected prepared before-return FPR move text layout");
  }
  if (read_u32(text->bytes, 0) != 0x22000553 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fmv.d fa0, ft0 followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared before-return FPR move object to need no relocations");
  }
  return 0;
}

int rejects_prepared_before_return_fpr_abi_move_fail_closed_shapes() {
  auto prepared = make_prepared_before_return_fpr_f32_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .uses_cycle_temp_source = true;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
      0) {
    return 1;
  }

  prepared = make_prepared_before_return_fpr_f32_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_contiguous_width = 2;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
      0) {
    return 1;
  }

  prepared = make_prepared_before_return_fpr_f32_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_stack_offset_bytes = 0;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
      0) {
    return 1;
  }

  prepared = make_prepared_before_return_fpr_f128_abi_move_module();
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
      0) {
    return 1;
  }

  prepared = make_prepared_before_return_fpr_f32_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_register_placement = std::nullopt;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
      0) {
    return 1;
  }

  return 0;
}

int builds_prepared_fpr_immediate_return_objects() {
  auto prepared =
      make_prepared_fpr_immediate_return_module(bir::TypeKind::F32, 1);
  auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared F32 immediate-return RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "fpr_immediate_return");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared F32 immediate-return object to publish text/function");
  }
  if (text->bytes.size() != 12 || function->size_bytes != 12 ||
      read_u32(text->bytes, 0) != 0x00100293 ||
      read_u32(text->bytes, 4) != 0xf0028553 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected addi t0, zero, 1; fmv.w.x fa0, t0; ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared F32 immediate-return object to need no relocations");
  }

  prepared = make_prepared_fpr_immediate_return_module(bir::TypeKind::F64, 7);
  module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared F64 immediate-return RV64 object to build");
  }
  text = object::find_section(*module, ".text");
  function = object::find_symbol(*module, "fpr_immediate_return");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared F64 immediate-return object to publish text/function");
  }
  if (text->bytes.size() != 12 || function->size_bytes != 12 ||
      read_u32(text->bytes, 0) != 0x00700293 ||
      read_u32(text->bytes, 4) != 0xf2028553 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected addi t0, zero, 7; fmv.d.x fa0, t0; ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared F64 immediate-return object to need no relocations");
  }

  return 0;
}

int rejects_prepared_fpr_immediate_return_fail_closed_shapes() {
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fpr_immediate_return_module(bir::TypeKind::F128, 0),
          "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering") !=
      0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fpr_immediate_return_module(bir::TypeKind::F32, 0x3f800000u),
          "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering") !=
      0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fpr_immediate_return_module(
              bir::TypeKind::F64,
              0x3ff0000000000000ull),
          "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering") !=
      0) {
    return 1;
  }
  return 0;
}

int rejects_unsupported_prepared_floating_cast_with_precise_diagnostic() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_unsupported_floating_cast_module(),
      "unsupported_floating_cast: RV64 object route supports only prepared FPR width casts and I32/I64-to-F32/F64 integer-to-floating casts");
}

int rejects_prepared_data_without_asm_fallback() {
  auto prepared = make_prepared_direct_call_module();
  const auto target = prepared.module.names.link_names.intern("target");
  const auto global_name = prepared.module.names.link_names.intern("g");
  prepared.module.globals.push_back(bir::Global{
      .name = "g",
      .link_name_id = global_name,
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::named_symbol_pointer("@target", target),
  });
  publish_prepared_object_data(prepared);
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=" +
          std::to_string(global_name) +
          " object_size_bytes=8 emitted_byte_count=0 zero_fill_byte_count=0");
}

int emits_prepared_writable_i32_global_object_storage() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("counter");
  prepared.module.globals.push_back(bir::Global{
      .name = "counter",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(7),
  });
  publish_prepared_object_data(prepared);

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit writable I32 globals");
  }
  const auto* data = object::find_section(*module, ".data");
  if (data == nullptr || !data->writable || data->executable ||
      data->align_bytes != 4 ||
      data->bytes != std::vector<std::uint8_t>{7, 0, 0, 0}) {
    return fail("expected mutable I32 global bytes in writable data");
  }
  const auto* symbol = object::find_symbol(*module, "counter");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Global ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{data->id} ||
      symbol->value != 0 || symbol->size_bytes != 4) {
    return fail("expected mutable I32 global object symbol in data");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize mutable I32 global object");
  }
  return 0;
}

int rejects_prepared_global_object_storage_without_prepared_data_facts() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("counter");
  prepared.module.globals.push_back(bir::Global{
      .name = "counter",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(7),
  });

  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_global_data: prepared selected object-data contract status=missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0");
}

int rejects_prepared_global_object_storage_incoherent_prepared_data_facts() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("counter");
  prepared.module.globals.push_back(bir::Global{
      .name = "counter",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(7),
  });
  publish_prepared_object_data(prepared);
  prepared.object_data.globals.front().conflicting_emitted_bytes = true;

  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_global_data: prepared selected object-data contract status=conflicting_emitted_bytes object_label_id=" +
          std::to_string(link_name) +
          " object_size_bytes=4 emitted_byte_count=4 zero_fill_byte_count=0");
}

int emits_prepared_linear_i8_global_object_storage() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("bytes");
  prepared.module.globals.push_back(bir::Global{
      .name = "bytes",
      .link_name_id = link_name,
      .type = bir::TypeKind::I8,
      .is_constant = true,
      .size_bytes = 3,
      .align_bytes = 1,
      .initializer_elements =
          {
              bir::Value::immediate_i8(1),
              bir::Value::immediate_i8(2),
              bir::Value::immediate_i8(3),
          },
  });
  publish_prepared_object_data(prepared);

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit linear I8 globals");
  }
  const auto* rodata = object::find_section(*module, ".rodata");
  if (rodata == nullptr || rodata->writable || rodata->executable ||
      rodata->align_bytes != 1 ||
      rodata->bytes != std::vector<std::uint8_t>{1, 2, 3}) {
    return fail("expected constant I8 element bytes in read-only data");
  }
  const auto* symbol = object::find_symbol(*module, "bytes");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Global ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{rodata->id} ||
      symbol->value != 0 || symbol->size_bytes != 3) {
    return fail("expected linear I8 global object symbol in rodata");
  }
  return 0;
}

int emits_prepared_zero_global_bss_storage() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("zeros");
  prepared.module.globals.push_back(bir::Global{
      .name = "zeros",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements =
          {
              bir::Value::immediate_i32(0),
              bir::Value::immediate_i32(0),
          },
  });
  publish_prepared_object_data(prepared);

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit BSS globals");
  }
  const auto* bss = object::find_section(*module, ".bss");
  if (bss == nullptr || !bss->writable || bss->executable ||
      bss->align_bytes != 4 || !bss->bytes.empty() || bss->size_bytes != 8) {
    return fail("expected zero global reservation in BSS");
  }
  const auto* symbol = object::find_symbol(*module, "zeros");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Global ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{bss->id} ||
      symbol->value != 0 || symbol->size_bytes != 8) {
    return fail("expected zero global object symbol in BSS");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize BSS global object");
  }
  return 0;
}

int emits_prepared_constant_f64_global_object_storage() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("one");
  prepared.module.globals.push_back(bir::Global{
      .name = "one",
      .link_name_id = link_name,
      .type = bir::TypeKind::F64,
      .is_constant = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::immediate_f64_bits(0x3ff0000000000000ull),
  });
  publish_prepared_object_data(prepared);

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit constant F64 globals");
  }
  const auto* rodata = object::find_section(*module, ".rodata");
  if (rodata == nullptr || rodata->writable || rodata->executable ||
      rodata->align_bytes != 8 ||
      rodata->bytes != std::vector<std::uint8_t>{0, 0, 0, 0, 0, 0, 0xf0, 0x3f}) {
    return fail("expected constant F64 global bytes in read-only data");
  }
  const auto* symbol = object::find_symbol(*module, "one");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Global ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{rodata->id} ||
      symbol->value != 0 || symbol->size_bytes != 8) {
    return fail("expected constant F64 global object symbol in rodata");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize constant F64 global object");
  }
  return 0;
}

int emits_prepared_string_address_relocations_to_object_symbol() {
  const auto prepared = make_prepared_string_address_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit string address materialization");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* string_symbol = object::find_symbol(*module, ".LC0");
  const auto* auipc_label = object::find_symbol(*module, ".Lpcrel_hi_1_1_0");
  if (text == nullptr || rodata == nullptr || string_symbol == nullptr ||
      auipc_label == nullptr) {
    return fail("expected text, rodata, string symbol, and AUIPC-site label");
  }
  if (text->bytes.size() < 8 || text->bytes[0] != 0x17 ||
      text->bytes[1] != 0x05 || text->bytes[4] != 0x13 ||
      text->bytes[5] != 0x05) {
    return fail("expected prepared string address materialization into a0");
  }
  if (string_symbol->binding != object::SymbolBinding::Local ||
      string_symbol->kind != object::SymbolKind::Object ||
      string_symbol->section != std::optional<object::SectionId>{rodata->id} ||
      string_symbol->value != 0 || string_symbol->size_bytes != 4) {
    return fail("expected string address relocation target to be a defined object");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != string_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id) {
    return fail("expected prepared string address PC-relative relocation pair");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize string address relocations");
  }
  return 0;
}

int emits_prepared_string_call_argument_relocation_to_object_symbol() {
  const auto prepared = make_prepared_string_call_argument_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit string call argument");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* string_symbol = object::find_symbol(*module, ".LC0");
  const auto* auipc_label =
      object::find_symbol(*module, ".Lpcrel_call_arg_main_0_0_0");
  const auto* sink_symbol = object::find_symbol(*module, "sink");
  if (text == nullptr || rodata == nullptr || string_symbol == nullptr ||
      auipc_label == nullptr || sink_symbol == nullptr) {
    return fail("expected string call argument symbols and sections");
  }
  if (string_symbol->binding != object::SymbolBinding::Local ||
      string_symbol->kind != object::SymbolKind::Object ||
      string_symbol->section != std::optional<object::SectionId>{rodata->id}) {
    return fail("expected string call argument relocation target to be defined object");
  }
  if (sink_symbol->section.has_value()) {
    return fail("expected direct extern call target to remain undefined");
  }
  if (module->relocations.size() != 3 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != string_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id ||
      module->relocations[2].section != text->id ||
      module->relocations[2].type != R_RISCV_CALL_PLT ||
      module->relocations[2].symbol != sink_symbol->id) {
    return fail("expected string call argument relocation pair before call relocation");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize string call argument");
  }
  return 0;
}

int emits_prepared_global_address_relocations_to_object_symbol() {
  const auto prepared = make_prepared_global_address_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit global address materialization");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* global_symbol = object::find_symbol(*module, "global_i32");
  const auto* auipc_label = object::find_symbol(*module, ".Lpcrel_hi_1_1_0");
  if (text == nullptr || rodata == nullptr || global_symbol == nullptr ||
      auipc_label == nullptr) {
    return fail("expected text, rodata, global symbol, and AUIPC-site label");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{rodata->id} ||
      global_symbol->value != 0 || global_symbol->size_bytes != 4) {
    return fail("expected global address relocation target to be a defined object");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != global_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id) {
    return fail("expected prepared global address PC-relative relocation pair");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize global address relocations");
  }
  return 0;
}

int emits_prepared_global_load_relocations_and_instruction() {
  const auto prepared = make_prepared_global_load_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit global load");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* global_symbol = object::find_symbol(*module, "g");
  const auto* auipc_label = object::find_symbol(*module, ".Lpcrel_hi_global_load_1_1_0");
  if (text == nullptr || rodata == nullptr || global_symbol == nullptr ||
      auipc_label == nullptr) {
    return fail("expected text, rodata, global symbol, and load AUIPC-site label");
  }
  if (text->bytes.size() < 12 ||
      text->bytes[0] != 0x17 || text->bytes[1] != 0x05 ||
      text->bytes[4] != 0x13 || text->bytes[5] != 0x05 ||
      text->bytes[8] != 0x03 || text->bytes[9] != 0x25) {
    return fail("expected PC-relative address materialization followed by lw");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{rodata->id}) {
    return fail("expected global load relocation target to be a defined object");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != global_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id) {
    return fail("expected prepared global load PC-relative relocation pair");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize global load relocations");
  }
  return 0;
}

int emits_prepared_global_i8_load_and_zext_instruction() {
  const auto prepared = make_prepared_global_i8_zext_load_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit byte global load and zext");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* global_symbol = object::find_symbol(*module, "bytes");
  if (text == nullptr || rodata == nullptr || global_symbol == nullptr) {
    return fail("expected text, rodata, and byte global symbol");
  }
  if (text->bytes.size() < 20) {
    return fail("expected PC-relative address materialization, byte load, and zext");
  }
  const auto load = read_u32(text->bytes, 8);
  const auto zext = read_u32(text->bytes, 16);
  if ((load & 0x7fU) != 0x03U || ((load >> 12) & 0x7U) != 0U) {
    return fail("expected prepared byte global load to encode lb");
  }
  if ((zext & 0x7fU) != 0x13U || ((zext >> 12) & 0x7U) != 7U ||
      ((zext >> 20) & 0xfffU) != 0xffU) {
    return fail("expected prepared byte global load consumer to encode zext mask");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{rodata->id}) {
    return fail("expected byte global load relocation target to be a defined object");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize byte global load");
  }
  return 0;
}

int emits_prepared_same_width_i32_zext_gpr_copy() {
  const auto prepared = make_prepared_same_width_integer_zext_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared same-width i32 zext GPR cast to build");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() < 8) {
    return fail("expected same-width zext copy and return text");
  }
  const auto copy = read_u32(text->bytes, 0);
  if ((copy & 0x7fU) != 0x13U || ((copy >> 7) & 0x1fU) != 18U ||
      ((copy >> 12) & 0x7U) != 0U || ((copy >> 15) & 0x1fU) != 5U ||
      ((copy >> 20) & 0xfffU) != 0U) {
    return fail("expected same-width zext to publish prepared GPR copy");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize same-width zext object");
  }
  return 0;
}

int rejects_prepared_same_width_zext_fail_closed_shapes() {
  const std::string unsupported_instruction =
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";
  if (expect_prepared_rejection_diagnostic(
          make_prepared_same_width_integer_zext_module(bir::CastOpcode::SExt),
          unsupported_instruction) != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_same_width_integer_zext_module(bir::CastOpcode::ZExt,
                                                       bir::TypeKind::Ptr,
                                                       bir::TypeKind::Ptr),
          unsupported_instruction) != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_same_width_integer_zext_module(
              bir::CastOpcode::ZExt,
              bir::TypeKind::I32,
              bir::TypeKind::I32,
              prepare::PreparedValueHomeKind::StackSlot),
          "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes") !=
      0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_same_width_integer_zext_module(
              bir::CastOpcode::ZExt,
              bir::TypeKind::I32,
              bir::TypeKind::I32,
              prepare::PreparedValueHomeKind::Register,
              prepare::PreparedValueHomeKind::StackSlot),
          unsupported_instruction) != 0) {
    return 1;
  }
  return 0;
}

int emits_prepared_global_store_relocations_and_instruction() {
  const auto prepared = make_prepared_global_store_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit global store");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* data = object::find_section(*module, ".data");
  const auto* global_symbol = object::find_symbol(*module, "g");
  const auto* auipc_label = object::find_symbol(*module, ".Lpcrel_hi_global_store_1_1_0");
  if (text == nullptr || data == nullptr || global_symbol == nullptr ||
      auipc_label == nullptr) {
    return fail("expected text, data, global symbol, and store AUIPC-site label");
  }
  if (text->bytes.size() < 16 ||
      text->bytes[0] != 0x97 || text->bytes[1] != 0x02 ||
      text->bytes[4] != 0x93 || text->bytes[5] != 0x82 ||
      text->bytes[8] != 0x13 || text->bytes[9] != 0x03 ||
      text->bytes[12] != 0x23 || text->bytes[13] != 0xa0) {
    return fail("expected PC-relative address materialization, value move, and sw");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{data->id}) {
    return fail("expected global store relocation target to be a defined object");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != global_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_RISCV_PCREL_LO12_I ||
      module->relocations[1].symbol != auipc_label->id) {
    return fail("expected prepared global store PC-relative relocation pair");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize global store relocations");
  }
  return 0;
}

int emits_prepared_global_i16_store_instruction() {
  const auto prepared = make_prepared_global_i16_store_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit halfword global store");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* data = object::find_section(*module, ".data");
  const auto* global_symbol = object::find_symbol(*module, "h");
  if (text == nullptr || data == nullptr || global_symbol == nullptr) {
    return fail("expected text, data, and halfword global symbol");
  }
  if (text->bytes.size() < 16) {
    return fail("expected PC-relative address materialization, value move, and sh");
  }
  const auto store = read_u32(text->bytes, 12);
  if ((store & 0x7fU) != 0x23U || ((store >> 12) & 0x7U) != 1U) {
    return fail("expected prepared halfword global store to encode sh");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{data->id}) {
    return fail("expected halfword global store relocation target to be a defined object");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize halfword global store");
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
  status |= rejects_prepared_critical_edge_parallel_copy_with_shared_diagnostic();
  status |= builds_prepared_successor_entry_copy_from_shared_traversal();
  status |= builds_prepared_fused_sgt_i32_compare_branch_object();
  status |= builds_prepared_fused_sle_i32_compare_branch_object();
  status |= builds_prepared_fused_ne_ptr_null_compare_branch_object();
  status |= rejects_prepared_fused_compare_branch_fail_closed_shapes();
  status |= builds_prepared_rematerialized_nonzero_return_object();
  status |= builds_prepared_traversed_wide_rematerialized_return_object();
  status |= rejects_prepared_rematerialized_return_without_typed_immediate_fact();
  status |= builds_prepared_scalar_same_module_call_object();
  status |= builds_prepared_byval_stack_copy_same_module_call_object();
  status |= rejects_prepared_byval_stack_copy_call_fail_closed_shapes();
  status |= builds_prepared_same_module_sret_call_object();
  status |= rejects_prepared_same_module_sret_call_fail_closed_shapes();
  status |= builds_prepared_scalar_stack_result_call_object();
  status |= builds_prepared_scalar_stack_result_call_with_inferred_gpr_banks_object();
  status |= rejects_prepared_scalar_stack_result_call_fail_closed_shapes();
  status |= builds_prepared_fpr_same_module_call_object();
  status |= preserves_missing_variadic_entry_plan_diagnostic();
  status |= preserves_missing_variadic_required_facts_diagnostic();
  status |= rejects_incomplete_helper_free_variadic_entry_contract();
  status |= builds_fact_complete_helper_free_variadic_entry_object();
  status |= rejects_fact_complete_variadic_va_start_without_overflow_base_state();
  status |= rejects_variadic_va_start_with_missing_saved_gpr_publication_fact();
  status |=
      materializes_fact_complete_variadic_va_start_with_saved_gpr_publications();
  status |= loads_rv64_va_start_published_word_after_helper();
  status |= rejects_malformed_variadic_saved_gpr_publications();
  status |= rejects_malformed_variadic_va_start_destination_homes();
  status |= lowers_fact_complete_variadic_va_end_as_noop();
  status |= rejects_malformed_variadic_va_end_direct_extern_shapes();
  status |=
      materializes_fact_complete_variadic_aggregate_va_arg_helper();
  status |=
      rejects_aggregate_va_arg_helper_without_access_plan_payload_write_address();
  status |= builds_prepared_two_arg_scalar_call_object();
  status |= builds_prepared_prior_preserved_arg_call_object();
  status |= rejects_prepared_prior_preserved_arg_call_fail_closed_shapes();
  status |= builds_byval_stack_slot_param_home_object();
  status |= rejects_byval_stack_slot_param_home_fail_closed_shapes();
  status |= builds_scalar_gpr_stack_slot_param_home_object();
  status |= rejects_scalar_gpr_stack_slot_param_home_fail_closed_shapes();
  status |= rejects_byval_stack_slot_pointer_access_fail_closed_shapes();
  status |= builds_prepared_fpr_formal_param_home_with_target_identity_object();
  status |= rejects_raw_fpr_formal_param_home_without_target_identity();
  status |= builds_prepared_scalar_local_frame_object();
  status |= builds_prepared_frame_slot_address_local_store_object();
  status |= builds_prepared_f64_local_frame_object();
  status |= builds_prepared_f32_local_frame_object();
  status |= builds_prepared_f32_i32_local_overlay_object();
  status |= builds_prepared_scalar_local_subobject_frame_object();
  status |= rejects_prepared_f64_local_frame_fail_closed_shapes();
  status |= rejects_prepared_scalar_local_subobject_fail_closed_shapes();
  status |= builds_prepared_pointer_value_scalar_local_object();
  status |= builds_prepared_pointer_value_scalar_local_store_with_t1_base_object();
  status |= builds_prepared_pointer_value_f64_local_object();
  status |= builds_prepared_sret_stack_pointer_store_object();
  status |= rejects_prepared_sret_stack_pointer_store_fail_closed_shapes();
  status |= rejects_prepared_pointer_value_scalar_local_fail_closed_shapes();
  status |= builds_prepared_stack_slot_scalar_flow_object();
  status |= builds_prepared_stack_slot_to_gpr_move_bundle_object();
  status |= rejects_prepared_stack_slot_to_gpr_move_bundle_fail_closed_shapes();
  status |= builds_prepared_scalar_compare_trunc_object();
  status |= builds_prepared_scalar_ordered_compare_return_object();
  status |= builds_prepared_scalar_ashr_register_object();
  status |= builds_prepared_scalar_ashr_immediate_object();
  status |= builds_prepared_scalar_ashr_i64_register_object();
  status |= builds_prepared_scalar_ashr_i64_immediate_object();
  status |= rejects_prepared_scalar_ashr_invalid_immediate_object();
  status |= builds_prepared_scalar_divrem_object();
  status |= rejects_prepared_scalar_compare_publication_missing_home();
  status |= builds_prepared_join_transfer_select_materialization_object();
  status |= builds_prepared_normalized_sle_select_materialization_object();
  status |= skips_published_prepared_join_transfer_select_carrier_object();
  status |= materializes_published_prepared_join_transfer_select_edge_compare_source_object();
  status |=
      materializes_published_prepared_join_transfer_select_dependent_edge_compare_source_object();
  status |= rejects_published_prepared_join_transfer_select_ambiguous_publications_object();
  status |= builds_prepared_local_register_arg_call_object();
  status |= builds_prepared_frame_slot_value_arg_call_object();
  status |= rejects_prepared_frame_slot_value_arg_call_fail_closed_shapes();
  status |= builds_prepared_frame_slot_address_arg_call_object();
  status |= builds_prepared_frame_slot_address_arg_call_load_local_payload_object();
  status |= records_prepared_frame_slot_address_arg_missing_publication_need();
  status |= emits_prepared_frame_slot_address_arg_call_from_selected_storage_facts();
  status |= rejects_prepared_frame_slot_address_arg_emit_selected_storage_fail_closed();
  status |= rejects_prepared_frame_slot_address_arg_call_fail_closed_shapes();
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
  status |= rejects_rv64_line_core_out_of_range_jal_immediate();
  status |= parses_substituted_prepared_inline_asm_insn_d_with_line_core();
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
  status |= emits_prepared_string_constant_object_storage();
  status |= rejects_prepared_global_memory_without_prepared_access();
  status |= emits_prepared_global_aggregate_lane_load_from_explicit_facts();
  status |= rejects_raw_load_local_global_address_lane_without_prepared_access();
  status |= builds_prepared_i16_local_store_object();
  status |= builds_prepared_fpr_fpext_object();
  status |= builds_prepared_fpr_fptrunc_object();
  status |= builds_prepared_formal_fpr_fpext_to_ft0_object();
  status |= builds_prepared_sitofp_i32_immediate_to_f64_object();
  status |= builds_prepared_before_return_fpr_f32_abi_move_object();
  status |= builds_prepared_before_return_fpr_f64_abi_move_object();
  status |= rejects_prepared_before_return_fpr_abi_move_fail_closed_shapes();
  status |= builds_prepared_fpr_immediate_return_objects();
  status |= rejects_prepared_fpr_immediate_return_fail_closed_shapes();
  status |= rejects_unsupported_prepared_floating_cast_with_precise_diagnostic();
  status |= rejects_prepared_data_without_asm_fallback();
  status |= emits_prepared_writable_i32_global_object_storage();
  status |= rejects_prepared_global_object_storage_without_prepared_data_facts();
  status |=
      rejects_prepared_global_object_storage_incoherent_prepared_data_facts();
  status |= emits_prepared_linear_i8_global_object_storage();
  status |= emits_prepared_zero_global_bss_storage();
  status |= emits_prepared_constant_f64_global_object_storage();
  status |= emits_prepared_string_address_relocations_to_object_symbol();
  status |= emits_prepared_string_call_argument_relocation_to_object_symbol();
  status |= emits_prepared_global_address_relocations_to_object_symbol();
  status |= emits_prepared_global_load_relocations_and_instruction();
  status |= emits_prepared_global_i8_load_and_zext_instruction();
  status |= emits_prepared_same_width_i32_zext_gpr_copy();
  status |= rejects_prepared_same_width_zext_fail_closed_shapes();
  status |= emits_prepared_global_store_relocations_and_instruction();
  status |= emits_prepared_global_i16_store_instruction();
  status |= serializes_rv64_relocatable_elf_contract();
  status |= serializes_pcrel_hi_lo_relocations_with_auipc_label_symbol();
  status |= writes_prepared_rv64_relocatable_elf_object_file();
  return status;
}
