#include "src/backend/mir/object/model.hpp"
#include "src/backend/mir/riscv/codegen/object_emission.hpp"
#include "src/backend/mir/riscv/codegen/rv64_line_assembler.hpp"
#include "src/backend/prealloc/control_flow.hpp"
#include "src/backend/prealloc/module.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    bool include_overflow_area_initial_state = false) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("rv64_va_start");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto ap_name = prepared.names.value_names.intern("%ap");
  const auto ap_addr_name = prepared.names.value_names.intern("%ap.addr");

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
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a0"},
  };
  const prepare::PreparedValueHome ap_addr_home{
      .value_id = 2,
      .function_name = function_name,
      .value_name = ap_addr_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a1"},
  };
  prepared.variadic_entry_plans.functions.push_back(
      prepare::PreparedVariadicEntryPlanFunction{
          .function_name = function_name,
          .overflow_area =
              prepare::PreparedVariadicEntryOverflowArea{
                  .required = true,
                  .base_slot_id = include_overflow_area_initial_state
                                      ? std::optional<prepare::PreparedFrameSlotId>{7}
                                      : std::nullopt,
                  .base_stack_offset_bytes = include_overflow_area_initial_state
                                                 ? std::optional<std::size_t>{64}
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
          .align_bytes = 4,
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

prepare::PreparedBirModule make_prepared_byval_stack_slot_param_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name =
      prepared.names.function_names.intern("byval_stack_param");
  const auto param_name = prepared.names.value_names.intern("%p.pa");
  const auto object_id = prepare::PreparedObjectId{18};
  const auto slot_id = prepare::PreparedFrameSlotId{0};

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::immediate_i32(0);

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
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = function_name,
          .value_name = param_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = slot_id,
          .offset_bytes = std::size_t{0},
          .size_bytes = std::size_t{72},
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
                  .value_id = 14,
                  .function_name = main_name,
                  .value_name = x_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              prepare::PreparedValueHome{
                  .value_id = 15,
                  .function_name = main_name,
                  .value_name = y_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s2"},
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
                          prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{14},
                      .source_register_name = std::string{"s1"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::
                                  PreparedCallArgumentSourceSelectionKind::
                                      LocalFrameAddressMaterialization,
                              .source_value_id = prepare::PreparedValueId{14},
                              .source_value_name = x_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::Register,
                              .source_slot_id = prepare::PreparedFrameSlotId{4},
                              .source_stack_offset_bytes = 0,
                              .source_size_bytes = 8,
                              .source_align_bytes = 8,
                              .address_materialization_block_label = block_label,
                              .address_materialization_inst_index = 0,
                              .address_materialization_frame_slot_id =
                                  prepare::PreparedFrameSlotId{4},
                              .address_materialization_byte_offset = 0,
                          },
                  },
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{15},
                      .source_register_name = std::string{"s2"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a1"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::
                                  PreparedCallArgumentSourceSelectionKind::
                                      LocalFrameAddressMaterialization,
                              .source_value_id = prepare::PreparedValueId{15},
                              .source_value_name = y_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::Register,
                              .source_slot_id = prepare::PreparedFrameSlotId{5},
                              .source_stack_offset_bytes = 2,
                              .source_size_bytes = 8,
                              .source_align_bytes = 8,
                              .address_materialization_block_label = block_label,
                              .address_materialization_inst_index = 0,
                              .address_materialization_frame_slot_id =
                                  prepare::PreparedFrameSlotId{5},
                              .address_materialization_byte_offset = 2,
                          },
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
                  .frame_slot_id = prepare::PreparedFrameSlotId{4},
                  .byte_offset = 0,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = main_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
                  .result_value_name = y_name,
                  .result_value_id = prepare::PreparedValueId{15},
                  .result_home_kind = prepare::PreparedValueHomeKind::Register,
                  .frame_slot_id = prepare::PreparedFrameSlotId{5},
                  .byte_offset = 2,
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
          .frame_size_bytes = 12,
          .frame_alignment_bytes = 4,
          .frame_slot_order = {prepare::PreparedFrameSlotId{4},
                               prepare::PreparedFrameSlotId{5}},
      },
  };
  prepared.stack_layout.frame_size_bytes = 4;
  prepared.stack_layout.frame_alignment_bytes = 2;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{4},
          .function_name = main_name,
          .offset_bytes = 0,
          .size_bytes = 2,
          .align_bytes = 2,
          .fixed_location = true,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{5},
          .function_name = main_name,
          .offset_bytes = 2,
          .size_bytes = 2,
          .align_bytes = 2,
          .fixed_location = true,
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
          make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Sle),
          diagnostic) != 0) {
    return 1;
  }
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

int materializes_fact_complete_variadic_va_start_with_overflow_base_state() {
  const auto prepared = make_prepared_variadic_va_start_module(
      true /*include_overflow_area_initial_state*/);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared va_start RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "rv64_va_start");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared va_start object to publish text/function");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      function->value != 0 || function->size_bytes != 16 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared va_start object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x04010313 ||
      read_u32(text->bytes, 4) != 0x0065b023 ||
      read_u32(text->bytes, 8) != 0x00000513 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail("expected va_start to store sp+64 into the overflow-area va_list field");
  }
  if (!module->relocations.empty()) {
    return fail("expected materialized va_start helper to need no relocations");
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

int rejects_byval_stack_slot_param_home_with_precise_diagnostic() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_byval_stack_slot_param_module(),
      "unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots");
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
  store->value = bir::Value::immediate_f32_bits(0);
  prepared.addressing.functions[0].accesses[0].address.size_bytes = 4;
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

int expect_pointer_value_scalar_local_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing");
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
  store->value = bir::Value::immediate_f32_bits(0);
  prepared.addressing.functions[0].accesses[0].address.size_bytes = 4;
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

int rejects_prepared_scalar_compare_trunc_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_scalar_compare_trunc: RV64 object route supports only prepared named Sge i32 compare results feeding one i16 integer trunc publication";

  auto prepared = make_prepared_scalar_compare_trunc_module();
  auto* compare =
      std::get_if<bir::BinaryInst>(&prepared.module.functions[0].blocks[0].insts[0]);
  if (compare == nullptr) {
    return fail("expected mutable scalar compare/trunc fixture compare");
  }
  compare->opcode = bir::BinaryOpcode::Slt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_compare_trunc_module();
  compare =
      std::get_if<bir::BinaryInst>(&prepared.module.functions[0].blocks[0].insts[0]);
  if (compare == nullptr) {
    return fail("expected mutable scalar compare/trunc fixture compare");
  }
  compare->operand_type = bir::TypeKind::I64;
  compare->lhs = bir::Value::named(bir::TypeKind::I64, "%lhs");
  compare->rhs = bir::Value::immediate_i64(8);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_compare_trunc_module();
  auto* trunc =
      std::get_if<bir::CastInst>(&prepared.module.functions[0].blocks[0].insts[1]);
  if (trunc == nullptr) {
    return fail("expected mutable scalar compare/trunc fixture trunc");
  }
  trunc->result = bir::Value::named(bir::TypeKind::I32, "%trunc");
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_compare_trunc_module();
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{10},
      .function_name = prepared.names.function_names.find("main"),
      .offset_bytes = 0,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.value_locations.functions[0].value_homes[1] =
      rv64_stack_slot_home(2,
                           prepared.names.function_names.find("main"),
                           prepared.names.value_names.find("%cmp"),
                           prepare::PreparedFrameSlotId{10},
                           0);
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
      module->relocations[0].offset < main->value + 8) {
    return fail("expected frame-slot-address same-module call relocation");
  }
  if (read_u32(text->bytes, module->relocations[0].offset - 8) !=
          0x00010513 ||
      read_u32(text->bytes, module->relocations[0].offset - 4) !=
          0x00210593) {
    return fail("expected frame-slot addresses materialized into a0/a1 before call");
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
  prepared.call_plans.functions[0]
      .calls[0]
      .arguments[0]
      .source_selection->kind =
      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue;
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
  prepared.addressing.functions[0].address_materializations.clear();
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
  if (bytes.size() != 16 || read_u64(bytes, 0) != 0x0000030b0820030aull ||
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
      read_u64(*encoded, 0) != 0x0000030b10620a0aull) {
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
      "unsupported_floating_cast: RV64 object route supports only prepared F32-to-F64 and F64-to-F32 FPR register casts");
}

int rejects_prepared_data_without_asm_fallback() {
  auto prepared = make_prepared_direct_call_module();
  const auto target = prepared.module.names.link_names.intern("target");
  prepared.module.globals.push_back(bir::Global{
      .name = "g",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::named_symbol_pointer("@target", target),
  });
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_global_data: RV64 object route supports only immediate scalar and immediate linear global storage");
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
  status |= builds_prepared_fused_ne_ptr_null_compare_branch_object();
  status |= rejects_prepared_fused_compare_branch_fail_closed_shapes();
  status |= builds_prepared_rematerialized_nonzero_return_object();
  status |= builds_prepared_scalar_same_module_call_object();
  status |= builds_prepared_scalar_stack_result_call_object();
  status |= builds_prepared_scalar_stack_result_call_with_inferred_gpr_banks_object();
  status |= rejects_prepared_scalar_stack_result_call_fail_closed_shapes();
  status |= builds_prepared_fpr_same_module_call_object();
  status |= preserves_missing_variadic_entry_plan_diagnostic();
  status |= preserves_missing_variadic_required_facts_diagnostic();
  status |= rejects_incomplete_helper_free_variadic_entry_contract();
  status |= builds_fact_complete_helper_free_variadic_entry_object();
  status |= rejects_fact_complete_variadic_va_start_without_overflow_base_state();
  status |=
      materializes_fact_complete_variadic_va_start_with_overflow_base_state();
  status |= builds_prepared_two_arg_scalar_call_object();
  status |= rejects_byval_stack_slot_param_home_with_precise_diagnostic();
  status |= builds_prepared_fpr_formal_param_home_with_target_identity_object();
  status |= rejects_raw_fpr_formal_param_home_without_target_identity();
  status |= builds_prepared_scalar_local_frame_object();
  status |= builds_prepared_scalar_local_subobject_frame_object();
  status |= rejects_prepared_scalar_local_subobject_fail_closed_shapes();
  status |= builds_prepared_pointer_value_scalar_local_object();
  status |= builds_prepared_pointer_value_scalar_local_store_with_t1_base_object();
  status |= rejects_prepared_pointer_value_scalar_local_fail_closed_shapes();
  status |= builds_prepared_stack_slot_scalar_flow_object();
  status |= builds_prepared_stack_slot_to_gpr_move_bundle_object();
  status |= rejects_prepared_stack_slot_to_gpr_move_bundle_fail_closed_shapes();
  status |= builds_prepared_scalar_compare_trunc_object();
  status |= rejects_prepared_scalar_compare_trunc_fail_closed_shapes();
  status |= builds_prepared_join_transfer_select_materialization_object();
  status |= skips_published_prepared_join_transfer_select_carrier_object();
  status |= rejects_published_prepared_join_transfer_select_ambiguous_publications_object();
  status |= builds_prepared_local_register_arg_call_object();
  status |= builds_prepared_frame_slot_value_arg_call_object();
  status |= rejects_prepared_frame_slot_value_arg_call_fail_closed_shapes();
  status |= builds_prepared_frame_slot_address_arg_call_object();
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
  status |= builds_prepared_i16_local_store_object();
  status |= builds_prepared_fpr_fpext_object();
  status |= builds_prepared_fpr_fptrunc_object();
  status |= builds_prepared_formal_fpr_fpext_to_ft0_object();
  status |= builds_prepared_before_return_fpr_f32_abi_move_object();
  status |= builds_prepared_before_return_fpr_f64_abi_move_object();
  status |= rejects_prepared_before_return_fpr_abi_move_fail_closed_shapes();
  status |= builds_prepared_fpr_immediate_return_objects();
  status |= rejects_prepared_fpr_immediate_return_fail_closed_shapes();
  status |= rejects_unsupported_prepared_floating_cast_with_precise_diagnostic();
  status |= rejects_prepared_data_without_asm_fallback();
  status |= emits_prepared_writable_i32_global_object_storage();
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
