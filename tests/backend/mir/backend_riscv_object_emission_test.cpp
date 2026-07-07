#include "src/backend/mir/object/model.hpp"
#include "src/backend/mir/riscv/codegen/emit.hpp"
#include "src/backend/mir/riscv/codegen/object_emission.hpp"
#include "src/backend/mir/riscv/codegen/prepared_call_emit.hpp"
#include "src/backend/mir/riscv/codegen/prepared_emit_context.hpp"
#include "src/backend/mir/riscv/codegen/prepared_frame_emit.hpp"
#include "src/backend/mir/riscv/codegen/prepared_module_emit.hpp"
#include "src/backend/mir/riscv/codegen/rv64_line_assembler.hpp"
#include "src/backend/prealloc/control_flow.hpp"
#include "src/backend/prealloc/module.hpp"
#include "src/backend/prealloc/publication_plans.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
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
constexpr std::uint32_t R_RISCV_64 = 2;
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

bool contains_adjacent_u32_pair(const std::vector<std::uint8_t>& bytes,
                                std::uint32_t first,
                                std::uint32_t second) {
  for (std::size_t offset = 0; offset + 8 <= bytes.size(); offset += 4) {
    if (read_u32(bytes, offset) == first &&
        read_u32(bytes, offset + 4) == second) {
      return true;
    }
  }
  return false;
}

bool contains_u32_sequence(const std::vector<std::uint8_t>& bytes,
                           const std::vector<std::uint32_t>& words) {
  if (words.empty()) {
    return true;
  }
  for (std::size_t offset = 0; offset + words.size() * 4 <= bytes.size();
       offset += 4) {
    bool matched = true;
    for (std::size_t index = 0; index < words.size(); ++index) {
      if (read_u32(bytes, offset + index * 4) != words[index]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      return true;
    }
  }
  return false;
}

std::int32_t sign_extend_bits(std::uint32_t value, unsigned bits) {
  const std::uint32_t sign_bit = std::uint32_t{1} << (bits - 1);
  return static_cast<std::int32_t>((value ^ sign_bit) - sign_bit);
}

unsigned riscv_rd(std::uint32_t word) {
  return (word >> 7) & 0x1fU;
}

unsigned riscv_rs1(std::uint32_t word) {
  return (word >> 15) & 0x1fU;
}

unsigned riscv_rs2(std::uint32_t word) {
  return (word >> 20) & 0x1fU;
}

std::int32_t riscv_i_imm(std::uint32_t word) {
  return sign_extend_bits(word >> 20, 12);
}

std::int32_t riscv_s_imm(std::uint32_t word) {
  return sign_extend_bits(((word >> 7) & 0x1fU) | ((word >> 20) & 0xfe0U), 12);
}

bool is_rv64_load_from_sp(std::uint32_t word,
                          unsigned funct3,
                          std::int32_t offset) {
  return (word & 0x7fU) == 0x03U && ((word >> 12) & 0x7U) == funct3 &&
         riscv_rs1(word) == 2U && riscv_i_imm(word) == offset;
}

bool is_rv64_store_to_sp(std::uint32_t word,
                         unsigned funct3,
                         unsigned stored_register,
                         std::int32_t offset) {
  return (word & 0x7fU) == 0x23U && ((word >> 12) & 0x7U) == funct3 &&
         riscv_rs1(word) == 2U && riscv_rs2(word) == stored_register &&
         riscv_s_imm(word) == offset;
}

bool is_rv64_add(std::uint32_t word) {
  return (word & 0x7fU) == 0x33U && ((word >> 12) & 0x7U) == 0U &&
         ((word >> 25) & 0x7fU) == 0U;
}

bool contains_frame_slot_dynamic_pointer_result_publication(
    const std::vector<std::uint8_t>& bytes,
    unsigned base_register,
    std::int32_t offset_stack_offset,
    std::int32_t result_stack_offset) {
  for (std::size_t load_offset = 0; load_offset + 4 <= bytes.size();
       load_offset += 4) {
    const std::uint32_t load_word = read_u32(bytes, load_offset);
    if (!is_rv64_load_from_sp(load_word, 3U, offset_stack_offset)) {
      continue;
    }
    const unsigned offset_register = riscv_rd(load_word);
    for (std::size_t add_offset = load_offset + 4; add_offset + 4 <= bytes.size();
         add_offset += 4) {
      const std::uint32_t add_word = read_u32(bytes, add_offset);
      if (!is_rv64_add(add_word)) {
        continue;
      }
      const bool base_plus_offset =
          (riscv_rs1(add_word) == base_register &&
           riscv_rs2(add_word) == offset_register) ||
          (riscv_rs1(add_word) == offset_register &&
           riscv_rs2(add_word) == base_register);
      if (!base_plus_offset) {
        continue;
      }
      const unsigned result_register = riscv_rd(add_word);
      for (std::size_t store_offset = add_offset + 4;
           store_offset + 4 <= bytes.size();
           store_offset += 4) {
        if (is_rv64_store_to_sp(read_u32(bytes, store_offset), 3U,
                                result_register, result_stack_offset)) {
          return true;
        }
      }
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

constexpr const char* kGenericPreparedMoveBundleDiagnostic =
    "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";
constexpr const char* kGenericUnsupportedInstructionFragmentDiagnostic =
    "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";
constexpr const char* kUnsupportedSameModuleCallAbiDiagnostic =
    "unsupported_call_abi: RV64 object route requires supported ordinary same-module call ABI/result lowering";

bool prepared_rejection_diagnostic_matches(const std::string& actual,
                                           const std::string& expected) {
  if (actual == expected) {
    return true;
  }
  return (expected == kGenericPreparedMoveBundleDiagnostic ||
          expected == kGenericUnsupportedInstructionFragmentDiagnostic ||
          expected == kUnsupportedSameModuleCallAbiDiagnostic) &&
         actual.rfind(expected, 0) == 0;
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
  if (!prepared_rejection_diagnostic_matches(result.diagnostic,
                                             expected_diagnostic)) {
    return fail("expected prepared RV64 object diagnostic `" +
                expected_diagnostic + "`, got `" + result.diagnostic + "`");
  }
  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (image.ok() || image.image.has_value() ||
      image.prepared_consumer_category.has_value() ||
      !prepared_rejection_diagnostic_matches(image.diagnostic,
                                             expected_diagnostic)) {
    return fail("expected prepared RV64 ELF writer to preserve RV64-local diagnostic");
  }
  return 0;
}

int expect_prepared_rejection_diagnostic_contains(
    const prepare::PreparedBirModule& prepared,
    const std::vector<std::string>& expected_fragments) {
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("expected prepared RV64 object path to reject");
  }
  if (result.prepared_consumer_category.has_value()) {
    return fail("expected RV64-local diagnostic rather than shared category");
  }
  for (const auto& fragment : expected_fragments) {
    if (result.diagnostic.find(fragment) == std::string::npos) {
      return fail("expected prepared RV64 object diagnostic to contain `" +
                  fragment + "`, got `" + result.diagnostic + "`");
    }
  }
  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (image.ok() || image.image.has_value() ||
      image.prepared_consumer_category.has_value()) {
    return fail("expected prepared RV64 ELF writer to reject");
  }
  for (const auto& fragment : expected_fragments) {
    if (image.diagnostic.find(fragment) == std::string::npos) {
      return fail("expected prepared RV64 ELF diagnostic to contain `" +
                  fragment + "`, got `" + image.diagnostic + "`");
    }
  }
  return 0;
}

int expect_prepared_consumer_rejection_diagnostic(
    const prepare::PreparedBirModule& prepared,
    prepare::PreparedObjectConsumerDiagnosticCategory expected_category,
    const std::string& expected_diagnostic) {
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("expected prepared RV64 object path to reject");
  }
  if (result.prepared_consumer_category != expected_category) {
    return fail("expected shared prepared-consumer diagnostic category");
  }
  if (result.diagnostic != expected_diagnostic) {
    return fail("expected prepared-consumer diagnostic `" +
                expected_diagnostic + "`, got `" + result.diagnostic + "`");
  }
  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (image.ok() || image.image.has_value() ||
      image.prepared_consumer_category != expected_category ||
      image.diagnostic != expected_diagnostic) {
    return fail("expected prepared RV64 ELF writer to preserve shared diagnostic");
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

prepare::PreparedBirModule make_prepared_runtime_external_call_module(
    std::string callee, bool with_exit_arg) {
  prepare::PreparedBirModule prepared;
  const auto caller_name =
      prepared.names.function_names.intern(with_exit_arg ? "calls_exit" : "calls_abort");
  const auto entry_label = prepared.names.block_labels.intern("entry");

  bir::CallInst call;
  call.callee = callee;
  call.return_type = bir::TypeKind::Void;
  if (with_exit_arg) {
    call.args = {bir::Value::immediate_i32(0)};
    call.arg_types = {bir::TypeKind::I32};
  }
  bir::Block caller_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
      .label_id = entry_label,
  };
  caller_entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = with_exit_arg ? "calls_exit" : "calls_abort",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(caller_entry)},
  });

  prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 0,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::move(callee),
  };
  if (with_exit_arg) {
    call_plan.arguments = {prepare::PreparedCallArgumentPlan{
        .instruction_index = 0,
        .arg_index = 0,
        .value_bank = prepare::PreparedRegisterBank::Gpr,
        .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
        .source_literal = bir::Value::immediate_i32(0),
        .destination_register_name = std::string{"a0"},
        .destination_contiguous_width = 1,
        .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
    }};
  }
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = caller_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = caller_name,
      .calls = {std::move(call_plan)},
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
  if (!destination_address_is_gpr) {
    prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
        .slot_id = prepare::PreparedFrameSlotId{6},
        .object_id = 3,
        .function_name = function_name,
        .offset_bytes = 64,
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
                           prepare::PreparedFrameSlotId{6}},
      .offset_bytes = destination_address_is_gpr
                          ? std::nullopt
                          : std::optional<std::size_t>{64},
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

prepare::PreparedBirModule
make_prepared_global_symbol_address_prior_preserved_call_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("symbol_address_survives_call");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto symbol_value_name = prepared.names.value_names.intern("@global_state");
  const auto result_name = prepared.names.value_names.intern("%status");
  const auto global_name = prepared.names.link_names.intern("global_state");

  bir::CallInst first_call;
  first_call.result = bir::Value::named(bir::TypeKind::I32, "%status");
  first_call.callee = "capture";
  first_call.args = {bir::Value::named_symbol_pointer("@global_state", global_name)};
  first_call.arg_types = {bir::TypeKind::Ptr};
  first_call.return_type = bir::TypeKind::I32;

  bir::CallInst second_call;
  second_call.callee = "consume";
  second_call.args = {bir::Value::named_symbol_pointer("@global_state", global_name)};
  second_call.arg_types = {bir::TypeKind::Ptr};
  second_call.return_type = bir::TypeKind::Void;

  bir::Block entry{
      .label = "entry",
      .insts = {first_call, second_call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%status");

  prepared.module.functions.push_back(bir::Function{
      .name = "symbol_address_survives_call",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  });
  prepared.module.globals.push_back(bir::Global{
      .name = "global_state",
      .link_name_id = global_name,
      .type = bir::TypeKind::I64,
      .is_constant = false,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::immediate_i64(0),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  publish_prepared_object_data(prepared);
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
                  .value_name = symbol_value_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
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
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 0,
                  .wrapper_kind =
                      prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .direct_callee_name = std::string{"capture"},
                  .arguments = {prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::SymbolAddress,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_symbol_name = std::string{"@global_state"},
                      .source_symbol_name_id = global_name,
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
                      .value_name = symbol_value_name,
                      .route = prepare::PreparedCallPreservationRoute::
                          CalleeSavedRegister,
                      .callee_saved_save_index = std::size_t{0},
                      .contiguous_width = 1,
                      .register_name = std::string{"s1"},
                      .register_bank = prepare::PreparedRegisterBank::Gpr,
                      .occupied_register_names = {std::string{"s1"}},
                      .register_placement = s1_placement,
                      .preservation_source =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::Register,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .value_id = prepare::PreparedValueId{1},
                              .value_name = symbol_value_name,
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
                              .value_name = symbol_value_name,
                              .register_name = std::string{"s1"},
                              .register_bank = prepare::PreparedRegisterBank::Gpr,
                              .contiguous_width = 1,
                              .occupied_register_names = {std::string{"s1"}},
                              .callee_saved_save_index = std::size_t{0},
                              .register_placement = s1_placement,
                          },
                  }},
              },
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 1,
                  .wrapper_kind =
                      prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .direct_callee_name = std::string{"consume"},
                  .arguments = {prepare::PreparedCallArgumentPlan{
                      .instruction_index = 1,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::SymbolAddress,
                      .source_value_id = prepare::PreparedValueId{1},
                      .source_symbol_name = std::string{"@global_state"},
                      .source_symbol_name_id = global_name,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          prepare::PreparedCallArgumentSourceSelection{
                              .kind = prepare::
                                  PreparedCallArgumentSourceSelectionKind::
                                      PriorPreservation,
                              .source_value_id = prepare::PreparedValueId{1},
                              .source_value_name = symbol_value_name,
                              .source_home_kind =
                                  prepare::PreparedValueHomeKind::Register,
                              .source_size_bytes = std::size_t{8},
                              .source_align_bytes = std::size_t{8},
                              .preserved_call_block_index = std::size_t{0},
                              .preserved_call_instruction_index = std::size_t{0},
                              .preservation_route = prepare::
                                  PreparedCallPreservationRoute::
                                      CalleeSavedRegister,
                              .preserved_register_name = std::string{"s1"},
                              .preserved_register_bank =
                                  prepare::PreparedRegisterBank::Gpr,
                              .preserved_register_contiguous_width =
                                  std::size_t{1},
                              .preserved_occupied_register_names =
                                  {std::string{"s1"}},
                              .preserved_register_placement = s1_placement,
                              .preserved_callee_saved_save_index =
                                  std::size_t{0},
                          },
                  }},
              },
          },
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
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

prepare::PreparedBirModule make_prepared_immediate_null_same_module_call_module() {
  prepare::PreparedBirModule prepared;
  const auto callee_name = prepared.names.function_names.intern("mix");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto param_x_name = prepared.names.value_names.intern("%p.x");
  const auto param_p_name = prepared.names.value_names.intern("%p.p");
  const auto main_result_name = prepared.names.value_names.intern("%main.t0");

  bir::Block callee_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  callee_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%p.x");

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%main.t0");
  call.callee = "mix";
  call.args = {bir::Value::immediate_i32(100), null_pointer_value()};
  call.arg_types = {bir::TypeKind::I32, bir::TypeKind::Ptr};
  call.return_type = bir::TypeKind::I32;
  bir::Block main_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%main.t0");

  prepared.module.functions.push_back(bir::Function{
      .name = "mix",
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
                  .type = bir::TypeKind::Ptr,
                  .name = "%p.p",
                  .size_bytes = 8,
                  .align_bytes = 8,
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
                  .value_name = param_p_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a1"},
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
          .direct_callee_name = std::string{"mix"},
          .arguments =
              {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 0,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Immediate,
                      .source_literal = bir::Value::immediate_i32(100),
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
                      .source_literal = null_pointer_value(),
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

prepare::PreparedBirModule
make_prepared_prior_result_multi_gpr_same_module_call_module() {
  prepare::PreparedBirModule prepared;
  const auto seed_name = prepared.names.function_names.intern("seed");
  const auto combine_name = prepared.names.function_names.intern("combine");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto combine_x_name = prepared.names.value_names.intern("%p.x");
  const auto combine_y_name = prepared.names.value_names.intern("%p.y");
  const auto combine_p_name = prepared.names.value_names.intern("%p.p");
  const auto first_result_name = prepared.names.value_names.intern("%first");
  const auto second_result_name = prepared.names.value_names.intern("%second");

  bir::Block seed_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  seed_entry.terminator.value = bir::Value::immediate_i64(11);

  bir::Block combine_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  combine_entry.terminator.value = bir::Value::named(bir::TypeKind::I64, "%p.x");

  bir::CallInst first_call;
  first_call.result = bir::Value::named(bir::TypeKind::I64, "%first");
  first_call.callee = "seed";
  first_call.return_type = bir::TypeKind::I64;

  bir::CallInst second_call;
  second_call.result = bir::Value::named(bir::TypeKind::I64, "%second");
  second_call.callee = "combine";
  second_call.args = {bir::Value::named(bir::TypeKind::I64, "%first"),
                      bir::Value::immediate_i64(7),
                      null_pointer_value()};
  second_call.arg_types = {bir::TypeKind::I64,
                           bir::TypeKind::I64,
                           bir::TypeKind::Ptr};
  second_call.return_type = bir::TypeKind::I64;

  bir::Block main_entry{
      .label = "entry",
      .insts = {first_call, second_call},
      .terminator = bir::Terminator{},
  };
  main_entry.terminator.value = bir::Value::named(bir::TypeKind::I64, "%second");

  prepared.module.functions.push_back(bir::Function{
      .name = "seed",
      .return_type = bir::TypeKind::I64,
      .return_size_bytes = 8,
      .return_align_bytes = 8,
      .blocks = {std::move(seed_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "combine",
      .return_type = bir::TypeKind::I64,
      .return_size_bytes = 8,
      .return_align_bytes = 8,
      .params =
          {
              bir::Param{
                  .type = bir::TypeKind::I64,
                  .name = "%p.x",
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              bir::Param{
                  .type = bir::TypeKind::I64,
                  .name = "%p.y",
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              bir::Param{
                  .type = bir::TypeKind::Ptr,
                  .name = "%p.p",
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
      .blocks = {std::move(combine_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
      .return_type = bir::TypeKind::I64,
      .return_size_bytes = 8,
      .return_align_bytes = 8,
      .blocks = {std::move(main_entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = seed_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = combine_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = main_name,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = combine_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = combine_name,
                  .value_name = combine_x_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = combine_name,
                  .value_name = combine_y_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a1"},
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = combine_name,
                  .value_name = combine_p_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a2"},
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
                  .value_name = first_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 5,
                  .function_name = main_name,
                  .value_name = second_result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t1"},
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = main_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 0,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
                  .direct_callee_name = std::string{"seed"},
                  .result = prepare::PreparedCallResultPlan{
                      .instruction_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_value_id = 4,
                      .source_register_name = std::string{"a0"},
                      .source_contiguous_width = 1,
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"t0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  },
              },
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 1,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
                  .direct_callee_name = std::string{"combine"},
                  .arguments =
                      {
                          prepare::PreparedCallArgumentPlan{
                              .instruction_index = 1,
                              .arg_index = 0,
                              .value_bank = prepare::PreparedRegisterBank::Gpr,
                              .source_encoding =
                                  prepare::PreparedStorageEncodingKind::Register,
                              .source_value_id = prepare::PreparedValueId{4},
                              .source_register_name = std::string{"t0"},
                              .source_register_bank =
                                  prepare::PreparedRegisterBank::Gpr,
                              .destination_register_name = std::string{"a0"},
                              .destination_contiguous_width = 1,
                              .destination_register_bank =
                                  prepare::PreparedRegisterBank::Gpr,
                          },
                          prepare::PreparedCallArgumentPlan{
                              .instruction_index = 1,
                              .arg_index = 1,
                              .value_bank = prepare::PreparedRegisterBank::Gpr,
                              .source_encoding =
                                  prepare::PreparedStorageEncodingKind::Immediate,
                              .source_literal = bir::Value::immediate_i64(7),
                              .destination_register_name = std::string{"a1"},
                              .destination_contiguous_width = 1,
                              .destination_register_bank =
                                  prepare::PreparedRegisterBank::Gpr,
                          },
                          prepare::PreparedCallArgumentPlan{
                              .instruction_index = 1,
                              .arg_index = 2,
                              .value_bank = prepare::PreparedRegisterBank::Gpr,
                              .source_encoding =
                                  prepare::PreparedStorageEncodingKind::Immediate,
                              .source_literal = null_pointer_value(),
                              .destination_register_name = std::string{"a2"},
                              .destination_contiguous_width = 1,
                              .destination_register_bank =
                                  prepare::PreparedRegisterBank::Gpr,
                          },
                      },
                  .result = prepare::PreparedCallResultPlan{
                      .instruction_index = 1,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_value_id = 5,
                      .source_register_name = std::string{"a0"},
                      .source_contiguous_width = 1,
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"t1"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                  },
              },
          },
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

prepare::PreparedCallArgumentSourceSelection prior_preserved_s2_selection(
    c4c::ValueNameId source_name, prepare::PreparedValueId source_id) {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation,
      .source_value_id = source_id,
      .source_value_name = source_name,
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
      .source_size_bytes = std::size_t{8},
      .source_align_bytes = std::size_t{8},
      .preserved_call_block_index = std::size_t{0},
      .preserved_call_instruction_index = std::size_t{1},
      .preservation_route =
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
      .preserved_register_name = std::string{"s2"},
      .preserved_register_bank = prepare::PreparedRegisterBank::Gpr,
      .preserved_register_contiguous_width = std::size_t{1},
      .preserved_occupied_register_names = {std::string{"s2"}},
      .preserved_register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
              .slot_index = 2,
              .contiguous_width = 1,
          },
      .preserved_callee_saved_save_index = std::size_t{0},
  };
}

prepare::PreparedCallArgumentSourceSelection ptrtoint_d_preserved_s1_selection(
    c4c::ValueNameId source_name) {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation,
      .source_value_id = prepare::PreparedValueId{4},
      .source_value_name = source_name,
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
      .source_size_bytes = std::size_t{8},
      .source_align_bytes = std::size_t{8},
      .preserved_call_block_index = std::size_t{0},
      .preserved_call_instruction_index = std::size_t{1},
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

prepare::PreparedBirModule
make_prepared_ptrtoint_param_survives_nested_same_module_call_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto consume_name = prepared.names.function_names.intern("consume_bits");
  const auto keep_name = prepared.names.function_names.intern("nested_keep");
  const auto function_name =
      prepared.names.function_names.intern("ptrtoint_param_survives_call");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto param_a_name = prepared.names.value_names.intern("%p.a");
  const auto param_b_name = prepared.names.value_names.intern("%p.b");
  const auto param_c_name = prepared.names.value_names.intern("%p.c");
  const auto int_name = prepared.names.value_names.intern("%d");

  bir::Block consume_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  bir::Block keep_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };

  bir::CastInst cast;
  cast.opcode = bir::CastOpcode::PtrToInt;
  cast.result = bir::Value::named(bir::TypeKind::I64, "%d");
  cast.operand = bir::Value::named(bir::TypeKind::Ptr, "%p.c");

  bir::CallInst keep_call;
  keep_call.callee = "nested_keep";
  keep_call.return_type = bir::TypeKind::Void;

  bir::CallInst consume_call;
  consume_call.callee = "consume_bits";
  consume_call.args = {bir::Value::named(bir::TypeKind::I64, "%d")};
  consume_call.arg_types = {bir::TypeKind::I64};
  consume_call.return_type = bir::TypeKind::Void;

  bir::Block entry{
      .label = "entry",
      .insts = {cast, keep_call, consume_call},
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "consume_bits",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = bir::TypeKind::I64,
          .name = "%p.bits",
          .size_bytes = 8,
          .align_bytes = 8,
      }},
      .blocks = {std::move(consume_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "nested_keep",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .blocks = {std::move(keep_entry)},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "ptrtoint_param_survives_call",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params =
          {
              bir::Param{
                  .type = bir::TypeKind::I64,
                  .name = "%p.a",
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              bir::Param{
                  .type = bir::TypeKind::I64,
                  .name = "%p.b",
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              bir::Param{
                  .type = bir::TypeKind::Ptr,
                  .name = "%p.c",
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
      .blocks = {std::move(entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = consume_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = keep_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = consume_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 20,
          .function_name = consume_name,
          .value_name = prepared.names.value_names.intern("%p.bits"),
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a0"},
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 1,
                  .function_name = function_name,
                  .value_name = param_a_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = param_b_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a1"},
              },
              prepare::PreparedValueHome{
                  .value_id = 3,
                  .function_name = function_name,
                  .value_name = param_c_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"a2"},
              },
              prepare::PreparedValueHome{
                  .value_id = 4,
                  .function_name = function_name,
                  .value_name = int_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
              },
          },
  });

  const prepare::PreparedRegisterPlacement s1_placement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
      .slot_index = 1,
      .contiguous_width = 1,
  };
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 1,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
                  .direct_callee_name = std::string{"nested_keep"},
                  .preserved_values = {prepare::PreparedCallPreservedValue{
                      .value_id = prepare::PreparedValueId{4},
                      .value_name = int_name,
                      .route = prepare::PreparedCallPreservationRoute::
                          CalleeSavedRegister,
                      .callee_saved_save_index = std::size_t{0},
                      .contiguous_width = 1,
                      .register_name = std::string{"s1"},
                      .register_bank = prepare::PreparedRegisterBank::Gpr,
                      .occupied_register_names = {std::string{"s1"}},
                      .register_placement = s1_placement,
                      .preservation_source =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::Register,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .value_id = prepare::PreparedValueId{4},
                              .value_name = int_name,
                              .register_name = std::string{"s1"},
                              .register_bank = prepare::PreparedRegisterBank::Gpr,
                              .contiguous_width = 1,
                              .occupied_register_names = {std::string{"s1"}},
                          },
                      .preservation_destination =
                          prepare::PreparedCallBoundaryEffectEndpoint{
                              .encoding =
                                  prepare::PreparedStorageEncodingKind::Register,
                              .storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .value_id = prepare::PreparedValueId{4},
                              .value_name = int_name,
                              .register_name = std::string{"s1"},
                              .register_bank = prepare::PreparedRegisterBank::Gpr,
                              .contiguous_width = 1,
                              .occupied_register_names = {std::string{"s1"}},
                              .callee_saved_save_index = std::size_t{0},
                              .register_placement = s1_placement,
                          },
                  }},
              },
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 2,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
                  .direct_callee_name = std::string{"consume_bits"},
                  .arguments = {prepare::PreparedCallArgumentPlan{
                      .instruction_index = 2,
                      .arg_index = 0,
                      .value_bank = prepare::PreparedRegisterBank::Gpr,
                      .source_encoding =
                          prepare::PreparedStorageEncodingKind::Register,
                      .source_value_id = prepare::PreparedValueId{4},
                      .source_register_name = std::string{"s1"},
                      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                      .destination_register_name = std::string{"a0"},
                      .destination_contiguous_width = 1,
                      .destination_register_bank =
                          prepare::PreparedRegisterBank::Gpr,
                      .source_selection =
                          ptrtoint_d_preserved_s1_selection(int_name),
                  }},
              },
          },
  });

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
      .function_name = consume_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 1,
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = keep_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 1,
  });
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

prepare::PreparedSavedRegister make_prepared_fpr_callee_saved_fs1() {
  const prepare::PreparedRegisterPlacement fs1_placement{
      .bank = prepare::PreparedRegisterBank::Fpr,
      .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
      .slot_index = 1,
      .contiguous_width = 1,
  };
  const prepare::PreparedSavedRegisterSlotPlacement fs1_slot{
      .bank = prepare::PreparedRegisterBank::Fpr,
      .register_name = "fs1",
      .contiguous_width = 1,
      .occupied_register_names = {"fs1"},
      .save_index = 0,
      .register_placement = fs1_placement,
      .slot_id = prepare::PreparedFrameSlotId{11},
      .stack_offset_bytes = std::size_t{24},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
      .fixed_location = true,
  };
  return prepare::PreparedSavedRegister{
      .bank = prepare::PreparedRegisterBank::Fpr,
      .register_name = "fs1",
      .contiguous_width = 1,
      .occupied_register_names = {"fs1"},
      .save_index = 0,
      .placement = fs1_placement,
      .slot_placement = fs1_slot,
  };
}

prepare::PreparedSavedRegister make_prepared_gpr_callee_saved_s1(
    std::size_t stack_offset_bytes) {
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
      .slot_id = prepare::PreparedFrameSlotId{10000},
      .stack_offset_bytes = stack_offset_bytes,
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
      .fixed_location = true,
  };
  return prepare::PreparedSavedRegister{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "s1",
      .contiguous_width = 1,
      .occupied_register_names = {"s1"},
      .save_index = 0,
      .placement = s1_placement,
      .slot_placement = s1_slot,
  };
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

prepare::PreparedBirModule make_prepared_large_fixed_stack_frame_module();

prepare::PreparedBirModule make_prepared_gpr_callee_saved_frame_module(
    std::size_t frame_size_bytes,
    std::size_t saved_stack_offset_bytes) {
  auto prepared = make_prepared_large_fixed_stack_frame_module();
  const auto function_name = prepared.names.function_names.intern("main");
  prepared.stack_layout.frame_size_bytes = frame_size_bytes;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.frame_plan.functions[0] = prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = frame_size_bytes,
      .frame_alignment_bytes = 16,
      .saved_callee_registers =
          {make_prepared_gpr_callee_saved_s1(saved_stack_offset_bytes)},
  };
  return prepared;
}

prepare::PreparedBirModule make_prepared_fpr_callee_saved_frame_module() {
  auto prepared = make_prepared_scalar_local_frame_module();
  const auto function_name = prepared.names.function_names.intern("main");
  prepared.control_flow.functions[0].blocks[0].terminator_kind =
      bir::TerminatorKind::Return;
  prepared.addressing.functions[0].frame_size_bytes = 48;
  prepared.addressing.functions[0].frame_alignment_bytes = 16;

  auto fs1 = make_prepared_fpr_callee_saved_fs1();
  auto fs2 = make_prepared_fpr_callee_saved_fs1();
  fs2.register_name = "fs2";
  fs2.occupied_register_names = {"fs2"};
  fs2.save_index = 1;
  fs2.placement->slot_index = 2;
  fs2.slot_placement->register_name = "fs2";
  fs2.slot_placement->occupied_register_names = {"fs2"};
  fs2.slot_placement->save_index = 1;
  fs2.slot_placement->register_placement = fs2.placement;
  fs2.slot_placement->slot_id = prepare::PreparedFrameSlotId{12};
  fs2.slot_placement->stack_offset_bytes = std::size_t{32};

  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 48,
      .frame_alignment_bytes = 16,
      .saved_callee_registers = {std::move(fs1), std::move(fs2)},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_large_fixed_stack_frame_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");

  bir::Block entry{
      .label = "entry",
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
      }},
  });
  prepared.stack_layout.frame_size_bytes = 4096;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 4096,
      .frame_alignment_bytes = 16,
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_large_fixed_slot_addressing_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto ptr_slot_name = prepared.names.slot_names.intern("%lv.ptr");
  const auto target_slot_name = prepared.names.slot_names.intern("%lv.target");
  const auto scalar_slot_name = prepared.names.slot_names.intern("%lv.x");
  const auto address_name = prepared.names.value_names.intern("%addr");
  const auto result_name = prepared.names.value_names.intern("%t0");

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
              bir::StoreLocalInst{
                  .slot_name = "%lv.x",
                  .slot_id = scalar_slot_name,
                  .value = bir::Value::immediate_i32(5),
                  .align_bytes = 4,
              },
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
                  .slot_name = "%lv.x",
                  .slot_id = scalar_slot_name,
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
                          .name = "%lv.ptr",
                          .slot_id = ptr_slot_name,
                          .type = bir::TypeKind::Ptr,
                          .size_bytes = 8,
                          .align_bytes = 8,
                      },
                      bir::LocalSlot{
                          .name = "%lv.target",
                          .slot_id = target_slot_name,
                          .type = bir::TypeKind::I32,
                          .size_bytes = 4,
                          .align_bytes = 4,
                      },
                      bir::LocalSlot{
                          .name = "%lv.x",
                          .slot_id = scalar_slot_name,
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
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = result_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 8192,
      .frame_alignment_bytes = 16,
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
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{2},
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
                  .result_value_name = result_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{2},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
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
                  .byte_offset = 4104,
              },
          },
  });
  prepared.stack_layout.frame_size_bytes = 8192;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{0},
          .function_name = function_name,
          .offset_bytes = 4104,
          .size_bytes = 4,
          .align_bytes = 4,
          .fixed_location = true,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{1},
          .function_name = function_name,
          .offset_bytes = 4096,
          .size_bytes = 8,
          .align_bytes = 8,
          .fixed_location = true,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{2},
          .function_name = function_name,
          .offset_bytes = 4112,
          .size_bytes = 4,
          .align_bytes = 4,
          .fixed_location = true,
      },
  };
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 8192,
      .frame_alignment_bytes = 16,
      .frame_slot_order = {prepare::PreparedFrameSlotId{0},
                           prepare::PreparedFrameSlotId{1},
                           prepare::PreparedFrameSlotId{2}},
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

prepare::PreparedBirModule
make_prepared_pointer_result_frame_slot_address_materialization_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name =
      prepared.names.function_names.intern("dynamic_pointer_result");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto target_slot_name = prepared.names.slot_names.intern("%target.object");
  const auto offset_slot_name = prepared.names.slot_names.intern("%offset.home");
  const auto result_slot_name = prepared.names.slot_names.intern("%result.home");
  const auto base_name = prepared.names.value_names.intern("%frame.base");
  const auto offset_name = prepared.names.value_names.intern("%byte.offset");
  const auto result_name = prepared.names.value_names.intern("%pointer.result");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::Ptr,
                                              "%pointer.result"),
                  .operand_type = bir::TypeKind::Ptr,
                  .lhs = bir::Value::named(bir::TypeKind::Ptr, "%frame.base"),
                  .rhs = bir::Value::named(bir::TypeKind::I64, "%byte.offset"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "dynamic_pointer_result",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
                          .name = "%target.object",
                          .slot_id = target_slot_name,
                          .type = bir::TypeKind::I32,
                          .size_bytes = 4,
                          .align_bytes = 4,
                      },
                      bir::LocalSlot{
                          .name = "%offset.home",
                          .slot_id = offset_slot_name,
                          .type = bir::TypeKind::I64,
                          .size_bytes = 8,
                          .align_bytes = 8,
                      },
                      bir::LocalSlot{
                          .name = "%result.home",
                          .slot_id = result_slot_name,
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
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 14,
                  .function_name = function_name,
                  .value_name = base_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"s1"},
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              prepare::PreparedValueHome{
                  .value_id = 12,
                  .function_name = function_name,
                  .value_name = offset_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{22},
                  .offset_bytes = 80,
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
              prepare::PreparedValueHome{
                  .value_id = 13,
                  .function_name = function_name,
                  .value_name = result_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{23},
                  .offset_bytes = 88,
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 96,
      .frame_alignment_bytes = 16,
      .address_materializations =
          {
              prepare::PreparedAddressMaterialization{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
                  .result_value_name = base_name,
                  .result_value_id = prepare::PreparedValueId{14},
                  .result_home_kind = prepare::PreparedValueHomeKind::Register,
                  .frame_slot_id = prepare::PreparedFrameSlotId{21},
                  .byte_offset = 24,
              },
          },
  });
  prepared.stack_layout.frame_size_bytes = 96;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{21},
          .function_name = function_name,
          .offset_bytes = 24,
          .size_bytes = 4,
          .align_bytes = 4,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{22},
          .function_name = function_name,
          .offset_bytes = 80,
          .size_bytes = 8,
          .align_bytes = 8,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{23},
          .function_name = function_name,
          .offset_bytes = 88,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  };
  prepared.frame_plan.functions = {
      prepare::PreparedFramePlanFunction{
          .function_name = function_name,
          .frame_size_bytes = 96,
          .frame_alignment_bytes = 16,
          .frame_slot_order = {prepare::PreparedFrameSlotId{21},
                               prepare::PreparedFrameSlotId{22},
                               prepare::PreparedFrameSlotId{23}},
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

prepare::PreparedBirModule
make_prepared_pointer_value_scalar_stack_home_local_module() {
  auto prepared = make_prepared_pointer_value_scalar_local_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto pointer_name = prepared.names.value_names.find("%p");
  const auto object_id = prepare::PreparedObjectId{17};
  const auto slot_id = prepare::PreparedFrameSlotId{17};
  if (function_name == c4c::kInvalidFunctionName ||
      pointer_name == c4c::kInvalidValueName ||
      prepared.value_locations.functions.empty() ||
      prepared.value_locations.functions[0].value_homes.empty() ||
      prepared.addressing.functions.empty()) {
    return prepared;
  }

  auto& pointer_home = prepared.value_locations.functions[0].value_homes[0];
  pointer_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  pointer_home.register_name = std::nullopt;
  pointer_home.slot_id = slot_id;
  pointer_home.offset_bytes = std::size_t{0};
  pointer_home.size_bytes = std::size_t{8};
  pointer_home.align_bytes = std::size_t{8};

  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = object_id,
      .function_name = function_name,
      .value_name = pointer_name,
      .source_kind = "local_slot",
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
  prepared.addressing.functions[0].frame_size_bytes = 16;
  prepared.addressing.functions[0].frame_alignment_bytes = 8;
  return prepared;
}

prepare::PreparedBirModule make_prepared_pointer_value_i8_local_store_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name =
      prepared.names.function_names.intern("pointer_value_i8_store");
  const auto block_label = prepared.names.block_labels.intern("block_12");
  const auto slot_name = prepared.names.slot_names.intern("%store.addr");
  const auto pointer_name = prepared.names.value_names.intern("%p");
  const auto stored_name = prepared.names.value_names.intern("%byte");

  bir::Block block{
      .label = "block_12",
      .insts =
          {
              bir::StoreLocalInst{
                  .slot_name = "%store.addr",
                  .slot_id = slot_name,
                  .value = bir::Value::named(bir::TypeKind::I8, "%byte"),
                  .align_bytes = 1,
                  .address = bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                      .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p"),
                      .byte_offset = 0,
                      .size_bytes = 1,
                      .align_bytes = 1,
                  },
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "pointer_value_i8_store",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .local_slots = {bir::LocalSlot{
          .name = "%store.addr",
          .slot_id = slot_name,
          .type = bir::TypeKind::I8,
          .size_bytes = 1,
          .align_bytes = 1,
      }},
      .blocks = {std::move(block)},
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
              prepare::PreparedValueHome{
                  .value_id = 2,
                  .function_name = function_name,
                  .value_name = stored_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
          },
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 6,
      .function_name = function_name,
      .value_name = prepared.names.value_names.intern("%store.addr"),
      .source_kind = "lowering_scratch",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .address_exposed = false,
      .requires_home_slot = true,
      .permanent_home_slot = false,
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
                  .stored_value_name = stored_name,
                  .address = prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                      .pointer_value_name = pointer_name,
                      .byte_offset = 0,
                      .size_bytes = 1,
                      .align_bytes = 1,
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

prepare::PreparedValueHome rv64_gpr_home(prepare::PreparedValueId value_id,
                                         c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         std::string register_name,
                                         std::size_t physical_index);

prepare::PreparedBirModule make_prepared_uitofp_i32_to_f32_then_fpext_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("uitofp_i32_to_f32_then_fpext");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto f32_name = prepared.names.value_names.intern("%f32");
  const auto f64_name = prepared.names.value_names.intern("%f64");

  bir::CastInst to_f32;
  to_f32.opcode = bir::CastOpcode::UIToFP;
  to_f32.operand = bir::Value::named(bir::TypeKind::I32, "%src");
  to_f32.result = bir::Value::named(bir::TypeKind::F32, "%f32");

  bir::CastInst to_f64;
  to_f64.opcode = bir::CastOpcode::FPExt;
  to_f64.operand = bir::Value::named(bir::TypeKind::F32, "%f32");
  to_f64.result = bir::Value::named(bir::TypeKind::F64, "%f64");

  bir::Block entry{
      .label = "entry",
      .insts = {to_f32, to_f64},
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "uitofp_i32_to_f32_then_fpext",
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
              rv64_gpr_home(1, function_name, source_name, "t0", 5),
              make_fpr_home(function_name, f32_name, 2, "ft0", 0),
              make_fpr_home(function_name, f64_name, 3, "fs1", 9),
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

prepare::PreparedBirModule make_prepared_direct_global_return_authority_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("direct_global_return");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto value_name = prepared.names.value_names.intern("@global");
  const auto global_name = prepared.names.link_names.intern("global");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value =
      bir::Value::named_symbol_pointer("@global", global_name);

  prepared.module.functions.push_back(bir::Function{
      .name = "direct_global_return",
      .return_type = bir::TypeKind::Ptr,
      .return_size_bytes = 8,
      .return_align_bytes = 8,
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
                  .value_name = value_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"t0"},
              },
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
                  .destination_register_name = std::string{"a0"},
                  .destination_contiguous_width = 1,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .reason = "return_stack_to_register",
                  .destination_register_placement =
                      prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CallResult,
                          .slot_index = 0,
                          .contiguous_width = 1,
                      },
              }},
          }},
  });
  return prepared;
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

prepare::PreparedBirModule make_prepared_global_f64_load_module(
    bool publish_access = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%d");
  const auto global_name = prepared.names.link_names.intern("d");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadGlobalInst{
                  .result = bir::Value::named(bir::TypeKind::F64, "%d"),
                  .global_name = "d",
                  .global_name_id = global_name,
                  .align_bytes = 8,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.globals.push_back(bir::Global{
      .name = "d",
      .link_name_id = global_name,
      .type = bir::TypeKind::F64,
      .is_constant = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::immediate_f64_bits(0x3ff0000000000000ULL),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "main",
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
      .value_homes = {make_fpr_home(function_name, result_name, 1, "ft0", 0)},
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
                .size_bytes = 8,
                .align_bytes = 8,
                .can_use_base_plus_offset = true,
                .provenance = bir::MemoryAccessProvenance{
                    .base_identity = bir::MemoryProvenanceBaseIdentity{
                        .kind =
                            bir::MemoryProvenanceBaseIdentityKind::GlobalSymbol,
                        .spelling = "d",
                        .link_name_id = global_name,
                    },
                    .object_extent = bir::MemoryObjectExtent{
                        .completeness =
                            bir::MemoryObjectExtentCompleteness::Complete,
                        .size_bytes = 8,
                        .size_known = true,
                    },
                    .requested_range = bir::make_memory_byte_range(0, 8),
                    .layout_authority = bir::MemoryLayoutAuthorityKind::ScalarLayout,
                    .range_verdict = bir::MemoryRangeVerdict::ProvenInBounds,
                },
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

prepare::PreparedBirModule make_prepared_pointer_cast_module(
    bir::CastOpcode opcode = bir::CastOpcode::IntToPtr,
    bir::TypeKind operand_type = bir::TypeKind::I64,
    bir::TypeKind result_type = bir::TypeKind::Ptr,
    prepare::PreparedValueHomeKind source_home_kind =
        prepare::PreparedValueHomeKind::Register,
    prepare::PreparedValueHomeKind result_home_kind =
        prepare::PreparedValueHomeKind::Register,
    bool immediate_operand = false,
    bool omit_source_home = false,
    bool omit_result_home = false) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-linux-gnu");
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("pointer_cast");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%in");
  const auto result_name = prepared.names.value_names.intern("%out");
  const std::size_t operand_size_bytes =
      operand_type == bir::TypeKind::I64 || operand_type == bir::TypeKind::Ptr
          ? 8
          : 4;
  const std::size_t result_size_bytes =
      result_type == bir::TypeKind::I64 || result_type == bir::TypeKind::Ptr
          ? 8
          : 4;

  const bool rematerialized_source =
      source_home_kind == prepare::PreparedValueHomeKind::RematerializableImmediate;
  const auto operand = immediate_operand
                           ? bir::Value::immediate_i32(12)
                           : bir::Value::named(operand_type, "%in");
  std::vector<bir::Inst> insts;
  if (rematerialized_source) {
    insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(operand_type, "%in"),
        .operand_type = operand_type,
        .lhs = bir::Value::immediate_i32(5),
        .rhs = bir::Value::immediate_i32(7),
    });
  }
  insts.push_back(bir::CastInst{
      .opcode = opcode,
      .result = bir::Value::named(result_type, "%out"),
      .operand = operand,
  });
  bir::Block entry{
      .label = "entry",
      .insts = std::move(insts),
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "pointer_cast",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = immediate_operand || rematerialized_source
                    ? std::vector<bir::Param>{}
                    : std::vector<bir::Param>{bir::Param{
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

  std::vector<prepare::PreparedValueHome> homes;
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
    source_home.size_bytes = operand_size_bytes;
    source_home.align_bytes = operand_size_bytes;
  }
  if (source_home_kind ==
      prepare::PreparedValueHomeKind::RematerializableImmediate) {
    source_home.register_name = std::nullopt;
    source_home.immediate_i32 = 12;
  }
  if (source_home_kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset) {
    source_home.pointer_base_value_name = source_name;
    source_home.pointer_byte_delta = 0;
  }
  if (!immediate_operand && !omit_source_home) {
    homes.push_back(std::move(source_home));
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
    result_home.size_bytes = result_size_bytes;
    result_home.align_bytes = result_size_bytes;
  }
  if (result_home_kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset) {
    result_home.pointer_base_value_name = source_name;
    result_home.pointer_byte_delta = 0;
  }
  if (!omit_result_home) {
    homes.push_back(std::move(result_home));
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = std::move(homes),
  });
  if (source_home_kind == prepare::PreparedValueHomeKind::StackSlot ||
      result_home_kind == prepare::PreparedValueHomeKind::StackSlot) {
    prepared.stack_layout.frame_size_bytes = 16;
    prepared.stack_layout.frame_alignment_bytes = 8;
  }
  if (source_home_kind == prepare::PreparedValueHomeKind::StackSlot) {
    prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
        .object_id = prepare::PreparedObjectId{1},
        .function_name = function_name,
        .value_name = source_name,
        .source_kind = "regalloc.spill_slot",
        .type = operand_type,
        .size_bytes = operand_size_bytes,
        .align_bytes = operand_size_bytes,
    });
    prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
        .slot_id = prepare::PreparedFrameSlotId{0},
        .object_id = prepare::PreparedObjectId{1},
        .function_name = function_name,
        .offset_bytes = 0,
        .size_bytes = operand_size_bytes,
        .align_bytes = operand_size_bytes,
    });
  }
  if (result_home_kind == prepare::PreparedValueHomeKind::StackSlot) {
    prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
        .object_id = prepare::PreparedObjectId{2},
        .function_name = function_name,
        .value_name = result_name,
        .source_kind = "regalloc.spill_slot",
        .type = result_type,
        .size_bytes = result_size_bytes,
        .align_bytes = result_size_bytes,
    });
    prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
        .slot_id = prepare::PreparedFrameSlotId{1},
        .object_id = prepare::PreparedObjectId{2},
        .function_name = function_name,
        .offset_bytes = 8,
        .size_bytes = result_size_bytes,
        .align_bytes = result_size_bytes,
    });
  }
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

prepare::PreparedValueHome rv64_sized_stack_slot_home(
    prepare::PreparedValueId value_id,
    c4c::FunctionNameId function_name,
    c4c::ValueNameId value_name,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t offset_bytes,
    std::size_t size_bytes) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = slot_id,
      .offset_bytes = offset_bytes,
      .size_bytes = size_bytes,
      .align_bytes = size_bytes,
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

prepare::PreparedBirModule make_prepared_fpr_compare_publication_module(
    bir::TypeKind operand_type,
    bool with_select_consumer,
    bool rhs_zero_immediate = false) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern(
          rhs_zero_immediate ? "fpr_compare_zero_publication"
          : with_select_consumer ? "fpr_compare_select_publication"
                                 : "fpr_compare_publication");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto compare_name = prepared.names.value_names.intern("%cmp");
  const auto selected_name = prepared.names.value_names.intern("%selected");
  const char* function = rhs_zero_immediate ? "fpr_compare_zero_publication"
                         : with_select_consumer ? "fpr_compare_select_publication"
                                                : "fpr_compare_publication";
  const auto rhs_value = rhs_zero_immediate
                             ? (operand_type == bir::TypeKind::F32
                                    ? bir::Value::immediate_f32_bits(0)
                                    : bir::Value::immediate_f64_bits(0))
                             : bir::Value::named(operand_type, "%rhs");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I32, "%cmp"),
                  .operand_type = operand_type,
                  .lhs = bir::Value::named(operand_type, "%lhs"),
                  .rhs = rhs_value,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  if (with_select_consumer) {
    entry.insts.push_back(bir::SelectInst{
        .predicate = bir::BinaryOpcode::Ne,
        .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
        .compare_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "%cmp"),
        .rhs = bir::Value::immediate_i32(0),
        .true_value = bir::Value::immediate_i32(7),
        .false_value = bir::Value::immediate_i32(3),
    });
    entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%selected");
  } else {
    entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%cmp");
  }

  prepared.module.functions.push_back(bir::Function{
      .name = function,
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
  auto lhs_home = make_fpr_home(function_name, lhs_name, 1, "fa0", 10);
  auto rhs_home = make_fpr_home(function_name, rhs_name, 2, "fa1", 11);
  const std::size_t operand_size = operand_type == bir::TypeKind::F32 ? 4 : 8;
  lhs_home.size_bytes = operand_size;
  lhs_home.align_bytes = operand_size;
  rhs_home.size_bytes = operand_size;
  rhs_home.align_bytes = operand_size;

  std::vector<prepare::PreparedValueHome> homes = {
      std::move(lhs_home),
      rv64_gpr_home(3, function_name, compare_name, "s1", 9),
  };
  if (!rhs_zero_immediate) {
    homes.insert(homes.begin() + 1, std::move(rhs_home));
  }
  if (with_select_consumer) {
    homes.push_back(rv64_gpr_home(4, function_name, selected_name, "a0", 10));
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = std::move(homes),
  });
  return prepared;
}

prepare::PreparedBirModule
make_prepared_before_instruction_register_to_stack_move_bundle_module() {
  auto prepared = make_prepared_scalar_compare_trunc_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto compare_name = prepared.names.value_names.find("%cmp");

  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{12},
      .function_name = function_name,
      .offset_bytes = 16,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes[0].size_bytes = std::size_t{4};
  locations.value_homes[1] = rv64_stack_slot_home(
      2, function_name, compare_name, prepare::PreparedFrameSlotId{12}, 16);
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = function_name,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = 0,
      .instruction_index = 0,
      .moves = {prepare::PreparedMoveResolution{
          .from_value_id = 1,
          .to_value_id = 2,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .destination_contiguous_width = 1,
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .reason = "consumer_register_to_stack",
      }},
  });
  return prepared;
}

prepare::PreparedBirModule
make_prepared_before_instruction_stack_to_stack_move_bundle_module() {
  auto prepared = make_prepared_stack_slot_scalar_flow_module();
  const auto function_name = prepared.names.function_names.find("main");
  auto& locations = prepared.value_locations.functions.front();
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = function_name,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = 0,
      .instruction_index = 2,
      .moves = {prepare::PreparedMoveResolution{
          .from_value_id = 1,
          .to_value_id = 2,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .destination_contiguous_width = 1,
          .block_index = 0,
          .instruction_index = 2,
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .reason = "consumer_stack_to_stack",
      }},
  });
  return prepared;
}

prepare::PreparedBirModule
make_prepared_before_instruction_stack_widening_move_bundle_module(
    bir::TypeKind source_type,
    std::size_t source_size_bytes) {
  auto prepared = make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  auto& function = prepared.module.functions[0];
  auto& local_slot = function.local_slots[0];
  auto& store =
      std::get<bir::StoreLocalInst>(function.blocks[0].insts[0]);
  auto& load =
      std::get<bir::LoadLocalInst>(function.blocks[0].insts[1]);
  local_slot.type = source_type;
  local_slot.size_bytes = source_size_bytes;
  local_slot.align_bytes = source_size_bytes;
  store.value = source_type == bir::TypeKind::I8
                    ? bir::Value::immediate_i8(5)
                    : bir::Value::immediate_i16(5);
  store.align_bytes = source_size_bytes;
  load.result = bir::Value::named(source_type, "%loaded");
  load.align_bytes = source_size_bytes;
  function.blocks[0].insts[2] = bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .operand = bir::Value::named(source_type, "%loaded"),
  };

  auto& locations = prepared.value_locations.functions[0];
  locations.value_homes[0].size_bytes = source_size_bytes;
  locations.value_homes[0].align_bytes = source_size_bytes;
  prepared.stack_layout.frame_slots[1].size_bytes = source_size_bytes;
  prepared.stack_layout.frame_slots[1].align_bytes = source_size_bytes;
  prepared.addressing.functions[0].accesses[0].address.size_bytes =
      source_size_bytes;
  prepared.addressing.functions[0].accesses[0].address.align_bytes =
      source_size_bytes;
  prepared.addressing.functions[0].accesses[1].address.size_bytes =
      source_size_bytes;
  prepared.addressing.functions[0].accesses[1].address.align_bytes =
      source_size_bytes;

  auto& bundle = locations.move_bundles[0];
  bundle.authority_kind =
      prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion;
  bundle.moves[0].authority_kind =
      prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion;
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

prepare::PreparedBirModule make_prepared_loaded_base_pointer_arithmetic_module(
    bir::BinaryOpcode pointer_opcode = bir::BinaryOpcode::Add) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("pointer_arithmetic");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto slot_name = prepared.names.slot_names.intern("%lv.base");
  const auto base_name = prepared.names.value_names.intern("%base.loaded");
  const auto index_name = prepared.names.value_names.intern("%p.index");
  const auto byte_offset_name =
      prepared.names.value_names.intern("%scaled.byte_offset");
  const auto result_name = prepared.names.value_names.intern("%result.ptr");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::Ptr, "%base.loaded"),
                  .slot_name = "%lv.base",
                  .slot_id = slot_name,
                  .align_bytes = 8,
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Mul,
                  .result =
                      bir::Value::named(bir::TypeKind::I64,
                                        "%scaled.byte_offset"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%p.index"),
                  .rhs = bir::Value::immediate_i64(4),
              },
              bir::BinaryInst{
                  .opcode = pointer_opcode,
                  .result =
                      bir::Value::named(bir::TypeKind::Ptr, "%result.ptr"),
                  .operand_type = bir::TypeKind::Ptr,
                  .lhs = bir::Value::named(bir::TypeKind::Ptr, "%base.loaded"),
                  .rhs = bir::Value::named(bir::TypeKind::I64,
                                           "%scaled.byte_offset"),
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value =
      bir::Value::named(bir::TypeKind::Ptr, "%result.ptr");

  prepared.module.functions.push_back(bir::Function{
      .name = "pointer_arithmetic",
      .return_type = bir::TypeKind::Ptr,
      .return_size_bytes = 8,
      .return_align_bytes = 8,
      .params = {bir::Param{
          .type = bir::TypeKind::I64,
          .name = "%p.index",
          .size_bytes = 8,
          .align_bytes = 8,
      }},
      .local_slots = {bir::LocalSlot{
          .name = "%lv.base",
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
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.stack_layout.frame_size_bytes = 8;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{0},
      .function_name = function_name,
      .offset_bytes = 0,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, base_name, "s1", 9),
              rv64_gpr_home(2, function_name, index_name, "t0", 5),
              rv64_gpr_home(3, function_name, byte_offset_name, "s2", 18),
              rv64_gpr_home(4, function_name, result_name, "t1", 6),
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 8,
      .frame_alignment_bytes = 8,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 0,
          .result_value_name = base_name,
          .address = prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = prepare::PreparedFrameSlotId{0},
              .byte_offset = 0,
              .size_bytes = 8,
              .align_bytes = 8,
              .can_use_base_plus_offset = true,
          },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_fpr_binary_module(
    bir::BinaryOpcode opcode,
    bir::TypeKind type) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("fp_binary");
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

  const std::size_t size_bytes = type == bir::TypeKind::F128 ? 16
                                 : type == bir::TypeKind::F32  ? 4
                                                               : 8;
  prepared.module.functions.push_back(bir::Function{
      .name = "fp_binary",
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
              make_fpr_home(function_name, lhs_name, 1, "fa0", 10),
              make_fpr_home(function_name, rhs_name, 2, "fa1", 11),
              make_fpr_home(function_name, result_name, 3, "ft0", 0),
          },
  });
  for (auto& home : prepared.value_locations.functions.back().value_homes) {
    home.size_bytes = size_bytes;
    home.align_bytes = size_bytes;
  }
  return prepared;
}

prepare::PreparedBirModule make_prepared_scalar_f64_immediate_binary_module(
    bool lhs_immediate) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("fp_binary");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  const auto result_name = prepared.names.value_names.intern("%result");
  const auto lhs = lhs_immediate
                       ? bir::Value::immediate_f64_bits(0x3ff0000000000000ULL)
                       : bir::Value::named(bir::TypeKind::F64, "%lhs");
  const auto rhs = lhs_immediate
                       ? bir::Value::named(bir::TypeKind::F64, "%rhs")
                       : bir::Value::immediate_f64_bits(0x3ff0000000000000ULL);

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::SDiv,
                  .result = bir::Value::named(bir::TypeKind::F64, "%result"),
                  .operand_type = bir::TypeKind::F64,
                  .lhs = lhs,
                  .rhs = rhs,
              },
          },
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "fp_binary",
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
  auto homes = std::vector<prepare::PreparedValueHome>{
      make_fpr_home(function_name, result_name, 1, "ft0", 0),
  };
  if (lhs_immediate) {
    homes.push_back(make_fpr_home(function_name, rhs_name, 2, "fa1", 11));
  } else {
    homes.push_back(make_fpr_home(function_name, lhs_name, 2, "fa0", 10));
  }
  for (auto& home : homes) {
    home.size_bytes = 8;
    home.align_bytes = 8;
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = std::move(homes),
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

prepare::PreparedBirModule
make_prepared_before_return_stack_to_register_abi_move_module(
    bir::TypeKind return_type = bir::TypeKind::I16,
    std::size_t return_size = 2) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("stack_return_move");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("%ret");

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };
  entry.terminator.value = bir::Value::named(return_type, "%ret");
  prepared.module.functions.push_back(bir::Function{
      .name = "stack_return_move",
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
  prepared.stack_layout.frame_size_bytes = 16;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{6},
          .function_name = function_name,
          .offset_bytes = 8,
          .size_bytes = return_size,
          .align_bytes = return_size,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_sized_stack_slot_home(1,
                                         function_name,
                                         source_name,
                                         prepare::PreparedFrameSlotId{6},
                                         8,
                                         return_size),
          },
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeReturn,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 0,
              .instruction_index = 0,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 1,
                  .to_value_id = 1,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"a0"},
                  .destination_contiguous_width = 1,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .reason = "return_stack_to_register",
                  .destination_register_placement =
                      prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CallResult,
                          .slot_index = 0,
                          .contiguous_width = 1,
                      },
              }},
          }},
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_out_of_ssa_phi_join_register_move_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("phi_join");
  const auto predecessor_label = prepared.names.block_labels.intern("pred");
  const auto successor_label = prepared.names.block_labels.intern("join");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto destination_name = prepared.names.value_names.intern("%dst");

  bir::Block pred{
      .label = "pred",
      .terminator = bir::Terminator{},
      .label_id = predecessor_label,
  };
  pred.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "phi_join",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(pred)},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = predecessor_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
      .parallel_copy_bundles =
          {prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor_label,
              .successor_label = successor_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
              .execution_block_label = predecessor_label,
              .moves = {prepare::PreparedParallelCopyMove{
                  .source_value = bir::Value::named(bir::TypeKind::I32, "%src"),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "%dst"),
              }},
              .steps = {prepare::PreparedParallelCopyStep{
                  .kind = prepare::PreparedParallelCopyStepKind::Move,
                  .move_index = 0,
              }},
          }},
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, source_name, "t0", 5),
              rv64_gpr_home(2, function_name, destination_name, "s1", 9),
          },
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 0,
              .instruction_index = 0,
              .source_parallel_copy_predecessor_label = predecessor_label,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 1,
                  .to_value_id = 2,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_contiguous_width = 1,
                  .source_parallel_copy_step_index = std::size_t{0},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .source_parallel_copy_predecessor_label = predecessor_label,
                  .source_parallel_copy_successor_label = successor_label,
                  .reason = "phi_join_register_to_register",
              }},
          }},
  });
  return prepared;
}

prepare::PreparedBirModule
make_prepared_out_of_ssa_phi_join_immediate_materialization_module() {
  auto prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  auto& parallel_copy =
      prepared.control_flow.functions.front().parallel_copy_bundles.front();
  parallel_copy.moves.front().source_value = bir::Value::immediate_i32(1234);

  auto& move_bundle = prepared.value_locations.functions.front().move_bundles.front();
  auto& move = move_bundle.moves.front();
  move.from_value_id = move.to_value_id;
  move.source_immediate_i32 = 1234;
  move.reason = "phi_join_immediate_materialization";
  return prepared;
}

prepare::PreparedBirModule
make_prepared_out_of_ssa_phi_join_i64_zero_materialization_module() {
  auto prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  auto& function = prepared.module.functions.front();
  function.return_type = bir::TypeKind::I64;
  function.return_size_bytes = 8;
  function.return_align_bytes = 8;
  auto& parallel_copy =
      prepared.control_flow.functions.front().parallel_copy_bundles.front();
  parallel_copy.moves.front().source_value = bir::Value::immediate_i64(0);
  parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I64, "%dst");

  auto& move = prepared.value_locations.functions.front().move_bundles.front().moves.front();
  move.source_immediate_i32 = 0;
  return prepared;
}

prepare::PreparedBirModule
make_prepared_out_of_ssa_edge_preservation_register_move_module() {
  auto prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  const auto function_name = prepared.names.function_names.find("phi_join");
  const auto predecessor_label = prepared.names.block_labels.find("pred");
  const auto successor_label = prepared.names.block_labels.find("join");
  const auto preservation_source_name = prepared.names.value_names.intern("%keep.src");
  const auto preservation_destination_name =
      prepared.names.value_names.intern("%keep.dst");

  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes.push_back(
      rv64_gpr_home(3, function_name, preservation_source_name, "t1", 6));
  locations.value_homes.push_back(rv64_gpr_home(
      4, function_name, preservation_destination_name, "s2", 18));
  auto& moves = locations.move_bundles.front().moves;
  moves.insert(moves.begin(),
               prepare::PreparedMoveResolution{
                   .from_value_id = 3,
                   .to_value_id = 4,
                   .destination_kind =
                       prepare::PreparedMoveDestinationKind::Value,
                   .destination_storage_kind =
                       prepare::PreparedMoveStorageKind::Register,
                   .destination_contiguous_width = 1,
                   .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                   .authority_kind =
                       prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                   .source_parallel_copy_predecessor_label = predecessor_label,
                   .source_parallel_copy_successor_label = successor_label,
                   .reason =
                       "edge_consumer_preservation_register_to_register",
               });
  return prepared;
}

prepare::PreparedBirModule
make_prepared_out_of_ssa_edge_preservation_stack_move_module() {
  auto prepared = make_prepared_out_of_ssa_edge_preservation_register_move_module();
  const auto function_name = prepared.names.function_names.find("phi_join");
  const auto preservation_destination_name =
      prepared.names.value_names.find("%keep.dst");

  prepared.stack_layout.frame_size_bytes = 16;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{21},
      .function_name = function_name,
      .offset_bytes = 8,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes[3] =
      rv64_stack_slot_home(4,
                           function_name,
                           preservation_destination_name,
                           prepare::PreparedFrameSlotId{21},
                           8);
  auto& move = locations.move_bundles.front().moves.front();
  move.destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  move.reason = "edge_consumer_preservation_register_to_stack";
  return prepared;
}

prepare::PreparedBirModule make_prepared_small_integer_ordinary_select_module(
    bir::TypeKind result_type) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const std::size_t result_size_bytes =
      result_type == bir::TypeKind::I8 ? std::size_t{1} : std::size_t{2};
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto false_name = prepared.names.value_names.intern("%fallback");
  const auto result_name = prepared.names.value_names.intern("%selected");
  const auto true_value =
      result_type == bir::TypeKind::I8
          ? bir::Value{.kind = bir::Value::Kind::Immediate,
                       .type = bir::TypeKind::I8,
                       .immediate = 7,
                       .immediate_bits = 7}
          : bir::Value{.kind = bir::Value::Kind::Immediate,
                       .type = bir::TypeKind::I16,
                       .immediate = 7,
                       .immediate_bits = 7};

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(result_type, "%selected"),
                  .compare_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
                  .rhs = bir::Value::immediate_i32(1),
                  .true_value = true_value,
                  .false_value = bir::Value::named(result_type, "%fallback"),
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
  prepared.stack_layout.frame_size_bytes = result_size_bytes;
  prepared.stack_layout.frame_alignment_bytes = result_size_bytes;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{3},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = result_size_bytes,
          .align_bytes = result_size_bytes,
      },
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              rv64_gpr_home(1, function_name, lhs_name, "t0", 5),
              rv64_gpr_home(2, function_name, false_name, "t1", 6),
              rv64_sized_stack_slot_home(3,
                                         function_name,
                                         result_name,
                                         prepare::PreparedFrameSlotId{3},
                                         0,
                                         result_size_bytes),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_nested_i32_ordinary_select_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto guard_name = prepared.names.value_names.intern("%guard");
  const auto fallback_name = prepared.names.value_names.intern("%fallback");
  const auto selected_name = prepared.names.value_names.intern("%selected");

  bir::Block entry{
      .label = "entry",
      .insts =
          {
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I32, "%inner"),
                  .compare_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
                  .rhs = bir::Value::immediate_i32(0),
                  .true_value = bir::Value::immediate_i32(7),
                  .false_value =
                      bir::Value::named(bir::TypeKind::I32, "%fallback"),
              },
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
                  .compare_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "%guard"),
                  .rhs = bir::Value::immediate_i32(0),
                  .true_value = bir::Value::named(bir::TypeKind::I32, "%inner"),
                  .false_value = bir::Value::immediate_i32(3),
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
              rv64_gpr_home(2, function_name, guard_name, "t1", 6),
              rv64_gpr_home(3, function_name, fallback_name, "t2", 7),
              rv64_sized_stack_slot_home(4,
                                         function_name,
                                         selected_name,
                                         prepare::PreparedFrameSlotId{3},
                                         0,
                                         4),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_reused_nested_i32_ordinary_select_module() {
  auto prepared = make_prepared_nested_i32_ordinary_select_module();
  const auto function_name = prepared.names.function_names.intern("main");
  const auto second_name = prepared.names.value_names.intern("%second");

  auto& entry = prepared.module.functions.front().blocks.front();
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%second"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%guard"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = bir::Value::named(bir::TypeKind::I32, "%inner"),
      .false_value = bir::Value::immediate_i32(5),
  });

  prepared.stack_layout.frame_size_bytes = 8;
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{4},
      .function_name = function_name,
      .offset_bytes = 4,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.value_locations.functions.front().value_homes.push_back(
      rv64_sized_stack_slot_home(5,
                                 function_name,
                                 second_name,
                                 prepare::PreparedFrameSlotId{4},
                                 4,
                                 4));
  return prepared;
}

prepare::PreparedBirModule make_prepared_tree_i32_ordinary_select_module() {
  auto prepared = make_prepared_nested_i32_ordinary_select_module();
  auto& entry = prepared.module.functions.front().blocks.front();
  entry.insts.clear();
  entry.insts = {
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%left.a"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::immediate_i32(7),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%fallback"),
      },
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%left.b"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::immediate_i32(7),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%fallback"),
      },
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%left"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%guard"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::named(bir::TypeKind::I32, "%left.a"),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%left.b"),
      },
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%right.a"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::immediate_i32(7),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%fallback"),
      },
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%right.b"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::immediate_i32(7),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%fallback"),
      },
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%right"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%guard"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::named(bir::TypeKind::I32, "%right.a"),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%right.b"),
      },
      bir::SelectInst{
          .predicate = bir::BinaryOpcode::Ne,
          .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
          .rhs = bir::Value::immediate_i32(0),
          .true_value = bir::Value::named(bir::TypeKind::I32, "%left"),
          .false_value = bir::Value::named(bir::TypeKind::I32, "%right"),
      },
  };
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

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_published_copies_stack_result_module() {
  auto prepared = make_prepared_join_transfer_select_with_published_copies_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto result_name = prepared.names.value_names.find("%selected");

  prepared.stack_layout.frame_size_bytes = 4;
  prepared.stack_layout.frame_alignment_bytes = 4;
  prepared.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{7},
          .function_name = function_name,
          .offset_bytes = 0,
          .size_bytes = 4,
          .align_bytes = 4,
      },
  };
  auto& result_home = prepared.value_locations.functions.front().value_homes.at(2);
  result_home = rv64_stack_slot_home(3,
                                     function_name,
                                     result_name,
                                     prepare::PreparedFrameSlotId{7},
                                     0);
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

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module() {
  auto prepared = make_prepared_join_transfer_select_with_edge_compare_source_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto ptr_lhs_name = prepared.names.value_names.intern("%ptr.lhs");
  const auto cast_source_name = prepared.names.value_names.intern("%rhs.cast.source");
  const auto cast_result_name = prepared.names.value_names.intern("%rhs.ptr");
  auto& join = prepared.module.functions.front().blocks.at(3);
  auto* compare = std::get_if<bir::BinaryInst>(&join.insts.front());
  if (compare == nullptr) {
    return prepared;
  }

  join.insts.insert(join.insts.begin(),
                    bir::CastInst{
                        .opcode = bir::CastOpcode::IntToPtr,
                        .result = bir::Value::named(bir::TypeKind::Ptr, "%rhs.ptr"),
                        .operand =
                            bir::Value::named(bir::TypeKind::I32, "%rhs.cast.source"),
                    });
  compare = std::get_if<bir::BinaryInst>(&join.insts.at(1));
  compare->opcode = bir::BinaryOpcode::Ule;
  compare->operand_type = bir::TypeKind::Ptr;
  compare->lhs = bir::Value::named(bir::TypeKind::Ptr, "%ptr.lhs");
  compare->rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs.ptr");

  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes.push_back(
      rv64_gpr_home(5, function_name, ptr_lhs_name, "s1", 9));
  locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 6,
      .function_name = function_name,
      .value_name = cast_source_name,
      .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
      .immediate_i32 = -2147483643,
  });
  locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 7,
      .function_name = function_name,
      .value_name = cast_result_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{8},
      .offset_bytes = 0,
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  });
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = function_name,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = 3,
      .instruction_index = 0,
      .moves = {prepare::PreparedMoveResolution{
          .from_value_id = 6,
          .to_value_id = 7,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .destination_contiguous_width = 1,
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      }},
  });
  prepared.stack_layout.frame_size_bytes = 8;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.stack_layout.objects = {prepare::PreparedStackObject{
      .object_id = prepare::PreparedObjectId{8},
      .function_name = function_name,
      .value_name = cast_result_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  }};
  prepared.stack_layout.frame_slots = {prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{8},
      .object_id = prepare::PreparedObjectId{8},
      .function_name = function_name,
      .offset_bytes = 0,
      .size_bytes = 8,
      .align_bytes = 8,
  }};
  return prepared;
}

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_carrier_alias_ule_source_module() {
  auto prepared = make_prepared_join_transfer_select_with_edge_compare_source_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto ptr_lhs_name = prepared.names.value_names.intern("%ptr.lhs");
  const auto ptr_rhs_name = prepared.names.value_names.intern("%ptr.rhs");
  prepared.names.value_names.intern("%selected.alias0");
  prepared.names.value_names.intern("%selected.alias1");

  auto& join = prepared.module.functions.front().blocks.at(3);
  auto* compare = std::get_if<bir::BinaryInst>(&join.insts.front());
  if (compare == nullptr) {
    return prepared;
  }
  compare->opcode = bir::BinaryOpcode::Ule;
  compare->operand_type = bir::TypeKind::Ptr;
  compare->lhs = bir::Value::named(bir::TypeKind::Ptr, "%ptr.lhs");
  compare->rhs = bir::Value::named(bir::TypeKind::Ptr, "%ptr.rhs");

  join.insts.insert(
      join.insts.begin() + 1,
      {
          bir::SelectInst{
              .predicate = bir::BinaryOpcode::Ne,
              .result = bir::Value::named(bir::TypeKind::I32,
                                          "%selected.alias0"),
              .compare_type = bir::TypeKind::I32,
              .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
              .rhs = bir::Value::immediate_i32(1),
              .true_value = bir::Value::named(bir::TypeKind::I32, "%rhs.cmp"),
              .false_value = bir::Value::immediate_i32(0),
          },
          bir::SelectInst{
              .predicate = bir::BinaryOpcode::Ne,
              .result = bir::Value::named(bir::TypeKind::I32,
                                          "%selected.alias1"),
              .compare_type = bir::TypeKind::I32,
              .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
              .rhs = bir::Value::immediate_i32(1),
              .true_value = bir::Value::named(bir::TypeKind::I32, "%rhs.cmp"),
              .false_value = bir::Value::immediate_i32(0),
          },
      });
  auto* final_select = std::get_if<bir::SelectInst>(&join.insts.at(3));
  if (final_select == nullptr) {
    return prepared;
  }
  final_select->true_value =
      bir::Value::named(bir::TypeKind::I32, "%selected.alias0");
  final_select->false_value =
      bir::Value::named(bir::TypeKind::I32, "%selected.alias1");

  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes.push_back(
      rv64_gpr_home(5, function_name, ptr_lhs_name, "s1", 9));
  locations.value_homes.push_back(
      rv64_gpr_home(6, function_name, ptr_rhs_name, "s2", 18));
  return prepared;
}

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_carrier_alias_ne_source_module() {
  auto prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ule_source_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto fallback_name = prepared.names.value_names.find("%fallback");
  const auto cmp_name = prepared.names.value_names.find("%rhs.cmp");

  auto& join = prepared.module.functions.front().blocks.at(3);
  auto* compare = std::get_if<bir::BinaryInst>(&join.insts.front());
  if (compare == nullptr) {
    return prepared;
  }
  compare->opcode = bir::BinaryOpcode::Ne;
  compare->operand_type = bir::TypeKind::I64;
  compare->lhs = bir::Value::named(bir::TypeKind::I64, "%fallback");
  compare->rhs = bir::Value::immediate_i64(1);

  auto& locations = prepared.value_locations.functions.front();
  for (auto& home : locations.value_homes) {
    if (home.value_name == fallback_name || home.value_name == cmp_name) {
      home = rv64_gpr_home(home.value_id, function_name, home.value_name, "a1", 11);
    }
  }
  return prepared;
}

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_predecessor_compare_publication_module() {
  auto prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ne_source_module();
  auto& function = prepared.module.functions.front();
  auto& predecessor = function.blocks.at(2);
  auto& join = function.blocks.at(3);
  if (join.insts.empty()) {
    return prepared;
  }
  predecessor.insts.push_back(std::move(join.insts.front()));
  join.insts.erase(join.insts.begin());
  return prepared;
}

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_only_carrier_use_ne_source_module() {
  auto prepared = make_prepared_join_transfer_select_with_edge_compare_source_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto fallback_name = prepared.names.value_names.find("%fallback");
  const auto cmp_name = prepared.names.value_names.find("%rhs.cmp");

  auto& join = prepared.module.functions.front().blocks.at(3);
  auto* compare = std::get_if<bir::BinaryInst>(&join.insts.front());
  if (compare == nullptr) {
    return prepared;
  }
  compare->opcode = bir::BinaryOpcode::Ne;
  compare->operand_type = bir::TypeKind::I64;
  compare->lhs = bir::Value::named(bir::TypeKind::I64, "%fallback");
  compare->rhs = bir::Value::immediate_i64(1);

  auto& locations = prepared.value_locations.functions.front();
  for (auto& home : locations.value_homes) {
    if (home.value_name == fallback_name || home.value_name == cmp_name) {
      home = rv64_gpr_home(home.value_id, function_name, home.value_name, "a1", 11);
    }
  }
  return prepared;
}

prepare::PreparedBirModule
make_prepared_join_transfer_select_with_suppressed_edge_compare_setup_module() {
  auto prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  auto& locations = prepared.value_locations.functions.front();
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = prepared.names.function_names.find("main"),
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = 3,
      .instruction_index = 1,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = 5,
                  .to_value_id = 4,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .block_index = 3,
                  .instruction_index = 1,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 7,
                  .to_value_id = 4,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .block_index = 3,
                  .instruction_index = 1,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
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

prepare::PreparedBirModule make_prepared_fp_to_int_cast_module(
    const char* function,
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
              rv64_gpr_home(2, function_name, result_name, "a0", 10),
          },
  });
  return prepared;
}

prepare::PreparedBirModule make_prepared_f64_immediate_fptrunc_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("f64_immediate_fptrunc");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto result_name = prepared.names.value_names.intern("%dst");

  bir::CastInst cast;
  cast.opcode = bir::CastOpcode::FPTrunc;
  cast.operand = bir::Value::immediate_f64_bits(0x3ff199999999999aull);
  cast.result = bir::Value::named(bir::TypeKind::F32, "%dst");
  bir::Block entry{
      .label = "entry",
      .insts = {cast},
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "f64_immediate_fptrunc",
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

prepare::PreparedBirModule make_prepared_empty_tied_scalar_gpr_inline_asm_module(
    bool complete_carrier = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile.arch = c4c::TargetArch::Riscv64;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto result_name = prepared.names.value_names.intern("%out");
  const auto input_name = prepared.names.value_names.intern("%x");
  const auto shared_identity = prepare::PreparedTargetRegisterIdentity{
      .target_arch = c4c::TargetArch::Riscv64,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .physical_index = 5,
  };

  bir::CallInst call;
  call.result = bir::Value::named(bir::TypeKind::I32, "%out");
  call.callee = "llvm.inline_asm";
  call.args = {bir::Value::named(bir::TypeKind::I32, "%x")};
  call.arg_types = {bir::TypeKind::I32};
  call.return_type = bir::TypeKind::I32;
  call.inline_asm = bir::InlineAsmMetadata{
      .asm_text = "",
      .constraints = "=r,0",
      .side_effects = true,
      .operands =
          {
              inline_asm_register_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                          0,
                                          "=r",
                                          std::nullopt,
                                          std::size_t{0}),
              inline_asm_register_operand(bir::InlineAsmOperandKind::TiedInput,
                                          1,
                                          "0",
                                          std::size_t{0},
                                          std::nullopt,
                                          std::size_t{0}),
          },
  };

  bir::Block entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%out");

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
              rv64_gpr_home(2, function_name, input_name, "t0", 5),
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
                        .asm_text = "",
                        .constraints = "=r,0",
                        .side_effects = true,
                        .operands =
                            {
                                prepare::PreparedInlineAsmOperand{
                                    .kind = bir::InlineAsmOperandKind::RegisterOutput,
                                    .constraint_index = 0,
                                    .constraint = "=r",
                                    .output_index = std::size_t{0},
                                    .register_class =
                                        bir::InlineAsmRegisterClass::General,
                                    .register_group_width = 1,
                                },
                                prepare::PreparedInlineAsmOperand{
                                    .kind = bir::InlineAsmOperandKind::TiedInput,
                                    .constraint_index = 1,
                                    .constraint = "0",
                                    .arg_index = std::size_t{0},
                                    .tied_output_index = std::size_t{0},
                                    .register_class =
                                        bir::InlineAsmRegisterClass::General,
                                    .register_group_width = 1,
                                    .value =
                                        bir::Value::named(bir::TypeKind::I32, "%x"),
                                    .value_name = input_name,
                                    .home = rv64_gpr_home(
                                        2, function_name, input_name, "t0", 5),
                                    .tied_home_authority =
                                        prepare::PreparedInlineAsmTiedHomeAuthority{
                                            .tied_output_index = 0,
                                            .shared_register = shared_identity,
                                        },
                                },
                            },
                        .result = bir::Value::named(bir::TypeKind::I32, "%out"),
                        .result_value_name = result_name,
                        .result_home = rv64_gpr_home(
                            1, function_name, result_name, "t0", 5),
                    },
                },
        });
  }
  return prepared;
}

prepare::PreparedBirModule make_prepared_no_result_memory_clobber_inline_asm_module(
    bool complete_carrier = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile.arch = c4c::TargetArch::Riscv64;
  const auto function_name = prepared.names.function_names.intern("main");

  bir::CallInst call;
  call.callee = "llvm.inline_asm";
  call.return_type = bir::TypeKind::Void;
  call.inline_asm = bir::InlineAsmMetadata{
      .asm_text = "",
      .constraints = "~{memory}",
      .side_effects = true,
      .operands =
          {
              bir::InlineAsmOperandMetadata{
                  .kind = bir::InlineAsmOperandKind::Clobber,
                  .constraint_index = 0,
                  .constraint = "~{memory}",
                  .name = std::string{"memory"},
              },
          },
      .clobbers = {"memory"},
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
                        .asm_text = "",
                        .constraints = "~{memory}",
                        .side_effects = true,
                        .operands =
                            {
                                prepare::PreparedInlineAsmOperand{
                                    .kind = bir::InlineAsmOperandKind::Clobber,
                                    .constraint_index = 0,
                                    .constraint = "~{memory}",
                                    .name = std::string{"memory"},
                                },
                            },
                        .clobbers = {"memory"},
                    },
                },
        });
  }
  return prepared;
}

prepare::PreparedBirModule make_prepared_symbol_address_imr_inline_asm_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile.arch = c4c::TargetArch::Riscv64;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto v1_link = prepared.module.names.link_names.intern("v1");
  const auto v2_link = prepared.module.names.link_names.intern("v2");
  const auto v3_link = prepared.module.names.link_names.intern("v3");

  bir::CallInst call;
  call.callee = "llvm.inline_asm";
  call.args = {
      bir::Value::named_symbol_pointer("@v1", v1_link),
      bir::Value::named_symbol_pointer("@v2", v2_link),
      bir::Value::named_symbol_pointer("@v3", v3_link),
  };
  call.arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr, bir::TypeKind::Ptr};
  call.return_type = bir::TypeKind::Void;
  call.inline_asm = bir::InlineAsmMetadata{
      .asm_text = "",
      .constraints = "imr,imr,imr,~{memory}",
      .side_effects = true,
      .operands =
          {
              bir::InlineAsmOperandMetadata{
                  .kind = bir::InlineAsmOperandKind::Unsupported,
                  .constraint_index = 0,
                  .constraint = "imr",
                  .arg_index = std::size_t{0},
              },
              bir::InlineAsmOperandMetadata{
                  .kind = bir::InlineAsmOperandKind::Unsupported,
                  .constraint_index = 1,
                  .constraint = "imr",
                  .arg_index = std::size_t{1},
              },
              bir::InlineAsmOperandMetadata{
                  .kind = bir::InlineAsmOperandKind::Unsupported,
                  .constraint_index = 2,
                  .constraint = "imr",
                  .arg_index = std::size_t{2},
              },
              bir::InlineAsmOperandMetadata{
                  .kind = bir::InlineAsmOperandKind::Clobber,
                  .constraint_index = 3,
                  .constraint = "~{memory}",
                  .name = std::string{"memory"},
              },
          },
      .clobbers = {"memory"},
      .unsupported_facts =
          {
              "unsupported_constraint0:imr",
              "unsupported_constraint1:imr",
              "unsupported_constraint2:imr",
              "constraint_operand_count_mismatch",
          },
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
  prepared.inline_asm_carriers.functions.push_back(
      prepare::PreparedInlineAsmCarrierFunction{
          .function_name = function_name,
          .carriers =
              {
                  prepare::PreparedInlineAsmCarrier{
                      .function_name = function_name,
                      .carrier_kind = prepare::PreparedInlineAsmCarrierKind::Missing,
                      .block_index = 0,
                      .inst_index = 0,
                      .asm_text = "",
                      .constraints = "imr,imr,imr,~{memory}",
                      .side_effects = true,
                      .operands =
                          {
                              prepare::PreparedInlineAsmOperand{
                                  .kind = bir::InlineAsmOperandKind::Unsupported,
                                  .constraint_index = 0,
                                  .constraint = "imr",
                                  .arg_index = std::size_t{0},
                                  .value = call.args[0],
                              },
                              prepare::PreparedInlineAsmOperand{
                                  .kind = bir::InlineAsmOperandKind::Unsupported,
                                  .constraint_index = 1,
                                  .constraint = "imr",
                                  .arg_index = std::size_t{1},
                                  .value = call.args[1],
                              },
                              prepare::PreparedInlineAsmOperand{
                                  .kind = bir::InlineAsmOperandKind::Unsupported,
                                  .constraint_index = 2,
                                  .constraint = "imr",
                                  .arg_index = std::size_t{2},
                                  .value = call.args[2],
                              },
                              prepare::PreparedInlineAsmOperand{
                                  .kind = bir::InlineAsmOperandKind::Clobber,
                                  .constraint_index = 3,
                                  .constraint = "~{memory}",
                                  .name = std::string{"memory"},
                              },
                          },
                      .clobbers = {"memory"},
                      .missing_required_facts =
                          {
                              "unsupported_constraint0:imr",
                              "unsupported_constraint1:imr",
                              "unsupported_constraint2:imr",
                              "constraint_operand_count_mismatch",
                          },
                  },
              },
          .missing_required_facts =
              {
                  "unsupported_constraint0:imr",
                  "unsupported_constraint1:imr",
                  "unsupported_constraint2:imr",
                  "constraint_operand_count_mismatch",
              },
      });
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

prepare::PreparedBirModule make_prepared_representative_same_module_sret_call_module() {
  auto prepared = make_prepared_same_module_sret_call_module();
  auto& callee = prepared.module.functions[0];
  auto& main = prepared.module.functions[1];
  auto& call = std::get<bir::CallInst>(main.blocks[0].insts[0]);
  auto& object = prepared.stack_layout.objects[0];
  auto& frame_slot = prepared.stack_layout.frame_slots[0];
  auto& call_plan = prepared.call_plans.functions[0].calls[0];

  call.args.resize(1);
  call.arg_types.resize(1);
  call.arg_abi.resize(1);
  callee.params.resize(1);
  prepared.value_locations.functions.clear();
  call_plan.arguments.resize(1);
  auto& sret_argument = call_plan.arguments[0];
  auto& source_selection = *sret_argument.source_selection;

  main.local_slots[0].size_bytes = 12;
  main.local_slots[0].align_bytes = 4;
  object.size_bytes = 12;
  object.align_bytes = 4;
  frame_slot.size_bytes = 12;
  frame_slot.align_bytes = 4;
  call_plan.memory_return->size_bytes = 12;
  call_plan.memory_return->align_bytes = 4;
  source_selection.source_size_bytes = std::size_t{12};
  source_selection.source_align_bytes = std::size_t{4};
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

prepare::PreparedBirModule make_prepared_stack_passed_scalar_param_home_module(
    bir::TypeKind type = bir::TypeKind::I64,
    bir::AbiValueClass abi_class = bir::AbiValueClass::Integer) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("stack_passed_scalar_param_home");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto param_name = prepared.names.value_names.intern("%p.late");
  const auto object_id = prepare::PreparedObjectId{12};
  const auto slot_id = prepare::PreparedFrameSlotId{14};
  const std::size_t size_bytes = type == bir::TypeKind::I32 ? 4 : 8;

  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
      .label_id = block_label,
  };

  prepared.module.functions.push_back(bir::Function{
      .name = "stack_passed_scalar_param_home",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .params = {bir::Param{
          .type = type,
          .name = "%p.late",
          .size_bytes = size_bytes,
          .align_bytes = size_bytes,
          .abi = bir::CallArgAbiInfo{
              .type = type,
              .size_bytes = size_bytes,
              .align_bytes = size_bytes,
              .primary_class = abi_class,
              .passed_in_register = false,
              .passed_on_stack = true,
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
      .type = type,
      .size_bytes = size_bytes,
      .align_bytes = size_bytes,
      .address_exposed = false,
      .requires_home_slot = false,
      .permanent_home_slot = false,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = object_id,
      .function_name = function_name,
      .offset_bytes = 48,
      .size_bytes = size_bytes,
      .align_bytes = size_bytes,
  });
  prepared.stack_layout.frame_size_bytes = 64;
  prepared.stack_layout.frame_alignment_bytes = 8;
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = 8,
          .function_name = function_name,
          .value_name = param_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = slot_id,
          .offset_bytes = std::size_t{48},
          .size_bytes = size_bytes,
          .align_bytes = size_bytes,
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

prepare::PreparedBirModule
make_prepared_frame_slot_value_and_prior_preserved_arg_call_module() {
  auto prepared = make_prepared_frame_slot_value_arg_call_module();
  const auto keep_name = prepared.names.function_names.intern("keep");
  const auto main_name = prepared.names.function_names.intern("main");
  const auto prior_name = prepared.names.value_names.intern("%p.b");
  const auto second_spill_name = prepared.names.value_names.intern("%spill2");

  bir::CallInst keep_call;
  keep_call.callee = "keep";
  keep_call.return_type = bir::TypeKind::Void;

  auto& sink = prepared.module.functions[0];
  sink.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "%p.y",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  sink.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "%p.z",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  auto& main = prepared.module.functions[1];
  main.return_type = bir::TypeKind::I64;
  main.return_size_bytes = 8;
  main.return_align_bytes = 8;
  main.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "%p.b",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  auto& entry = main.blocks[0];
  entry.insts.insert(entry.insts.begin() + 1, keep_call);
  auto* sink_call = std::get_if<bir::CallInst>(&entry.insts[2]);
  if (sink_call != nullptr) {
    sink_call->result = bir::Value::named(bir::TypeKind::I64, "%result");
    sink_call->args = {bir::Value::named(bir::TypeKind::I64, "%spill"),
                       bir::Value::named(bir::TypeKind::I64, "%spill2"),
                       bir::Value::named(bir::TypeKind::I64, "%p.b")};
    sink_call->arg_types = {bir::TypeKind::I64,
                            bir::TypeKind::I64,
                            bir::TypeKind::I64};
    sink_call->return_type = bir::TypeKind::I64;
  }
  entry.terminator.value = bir::Value::named(bir::TypeKind::I64, "%result");

  bir::Block keep_entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  prepared.module.functions.push_back(bir::Function{
      .name = "keep",
      .return_type = bir::TypeKind::Void,
      .return_size_bytes = 0,
      .return_align_bytes = 1,
      .blocks = {std::move(keep_entry)},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = keep_name,
  });

  auto& sink_homes = prepared.value_locations.functions[0].value_homes;
  sink_homes.push_back(prepare::PreparedValueHome{
      .value_id = 20,
      .function_name = prepared.names.function_names.intern("sink"),
      .value_name = prepared.names.value_names.intern("%p.y"),
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a1"},
  });
  sink_homes.push_back(prepare::PreparedValueHome{
      .value_id = 21,
      .function_name = prepared.names.function_names.intern("sink"),
      .value_name = prepared.names.value_names.intern("%p.z"),
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a2"},
  });

  auto& main_homes = prepared.value_locations.functions[1].value_homes;
  main_homes.push_back(prepare::PreparedValueHome{
      .value_id = 4,
      .function_name = main_name,
      .value_name = second_spill_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{4},
      .offset_bytes = 24,
  });
  main_homes.push_back(prepare::PreparedValueHome{
      .value_id = 5,
      .function_name = main_name,
      .value_name = prior_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a1"},
  });
  main_homes.push_back(prepare::PreparedValueHome{
      .value_id = 6,
      .function_name = main_name,
      .value_name = prepared.names.value_names.intern("%result"),
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });

  auto sink_plan = prepared.call_plans.functions[0].calls[0];
  sink_plan.instruction_index = 2;
  auto& first_arg = sink_plan.arguments[0];
  first_arg.instruction_index = 2;
  first_arg.source_selection->source_slot_id = prepare::PreparedFrameSlotId{0};
  first_arg.source_selection->source_stack_offset_bytes = std::size_t{0};
  sink_plan.arguments.push_back(prepare::PreparedCallArgumentPlan{
      .instruction_index = 2,
      .arg_index = 1,
      .value_bank = prepare::PreparedRegisterBank::Gpr,
      .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
      .source_value_id = prepare::PreparedValueId{4},
      .source_slot_id = prepare::PreparedFrameSlotId{4},
      .source_stack_offset_bytes = 24,
      .destination_register_name = std::string{"a1"},
      .destination_contiguous_width = 1,
      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
      .source_selection = prepare::PreparedCallArgumentSourceSelection{
          .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
          .source_value_id = prepare::PreparedValueId{4},
          .source_value_name = second_spill_name,
          .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
          .source_slot_id = prepare::PreparedFrameSlotId{4},
          .source_stack_offset_bytes = 24,
          .source_size_bytes = 8,
          .source_align_bytes = 8,
      },
  });
  sink_plan.arguments.push_back(prepare::PreparedCallArgumentPlan{
      .instruction_index = 2,
      .arg_index = 2,
      .value_bank = prepare::PreparedRegisterBank::Gpr,
      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
      .source_value_id = prepare::PreparedValueId{5},
      .source_register_name = std::string{"a1"},
      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
      .destination_register_name = std::string{"a2"},
      .destination_contiguous_width = 1,
      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
      .source_selection =
          prior_preserved_s2_selection(prior_name, prepare::PreparedValueId{5}),
  });
  sink_plan.result = prepare::PreparedCallResultPlan{
      .instruction_index = 2,
      .value_bank = prepare::PreparedRegisterBank::Gpr,
      .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_value_id = prepare::PreparedValueId{6},
      .source_register_name = std::string{"a0"},
      .source_contiguous_width = 1,
      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
      .destination_register_name = std::string{"t0"},
      .destination_contiguous_width = 1,
      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
  };

  const prepare::PreparedRegisterPlacement s2_placement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
      .slot_index = 2,
      .contiguous_width = 1,
  };
  const prepare::PreparedCallPlan keep_plan{
      .block_index = 0,
      .instruction_index = 1,
      .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
      .direct_callee_name = std::string{"keep"},
      .preserved_values = {prepare::PreparedCallPreservedValue{
          .value_id = prepare::PreparedValueId{5},
          .value_name = prior_name,
          .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
          .callee_saved_save_index = std::size_t{0},
          .contiguous_width = 1,
          .register_name = std::string{"s2"},
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .occupied_register_names = {std::string{"s2"}},
          .register_placement = s2_placement,
          .preservation_source =
              prepare::PreparedCallBoundaryEffectEndpoint{
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .value_id = prepare::PreparedValueId{5},
                  .value_name = prior_name,
                  .register_name = std::string{"a1"},
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .occupied_register_names = {std::string{"a1"}},
              },
          .preservation_destination =
              prepare::PreparedCallBoundaryEffectEndpoint{
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .value_id = prepare::PreparedValueId{5},
                  .value_name = prior_name,
                  .register_name = std::string{"s2"},
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .occupied_register_names = {std::string{"s2"}},
                  .callee_saved_save_index = std::size_t{0},
                  .register_placement = s2_placement,
              },
      }},
  };
  prepared.call_plans.functions[0].calls = {keep_plan, sink_plan};

  const prepare::PreparedSavedRegisterSlotPlacement s2_slot{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "s2",
      .contiguous_width = 1,
      .occupied_register_names = {"s2"},
      .save_index = 0,
      .register_placement = s2_placement,
      .slot_id = prepare::PreparedFrameSlotId{20},
      .stack_offset_bytes = std::size_t{0},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
      .fixed_location = true,
  };
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = keep_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 1,
  });
  auto& main_frame = prepared.frame_plan.functions[1];
  main_frame.frame_size_bytes = 40;
  main_frame.frame_alignment_bytes = 16;
  main_frame.frame_slot_order.push_back(prepare::PreparedFrameSlotId{4});
  main_frame.saved_callee_registers = {prepare::PreparedSavedRegister{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "s2",
      .contiguous_width = 1,
      .occupied_register_names = {"s2"},
      .save_index = 0,
      .placement = s2_placement,
      .slot_placement = s2_slot,
  }};
  prepared.stack_layout.frame_size_bytes = 40;
  prepared.stack_layout.frame_alignment_bytes = 16;
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{4},
      .function_name = main_name,
      .offset_bytes = 24,
      .size_bytes = 8,
      .align_bytes = 8,
  });
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
          .payload_frame_slot_id = prepare::PreparedFrameSlotId{7},
          .payload_stack_offset_bytes = 24,
          .payload_size_bytes = 8,
          .payload_align_bytes = 8,
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
          .payload_frame_slot_id = prepare::PreparedFrameSlotId{7},
          .payload_stack_offset_bytes = 24,
          .payload_size_bytes = 8,
          .payload_align_bytes = 8,
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
  x_fact.payload_frame_slot_id = std::nullopt;
  x_fact.payload_stack_offset_bytes = std::nullopt;
  x_fact.payload_size_bytes = std::nullopt;
  x_fact.payload_align_bytes = std::nullopt;

  auto& y_fact = prepared.call_argument_value_publications.facts[1];
  y_fact.source_store_instruction_index = 0;
  y_fact.payload_value_id = prepare::PreparedValueId{17};
  y_fact.payload_value_name = loaded_y_name;
  y_fact.payload_value = bir::Value::named(bir::TypeKind::Ptr, "%lv.loaded.y");
  y_fact.payload_frame_slot_id = std::nullopt;
  y_fact.payload_stack_offset_bytes = std::nullopt;
  y_fact.payload_size_bytes = std::nullopt;
  y_fact.payload_align_bytes = std::nullopt;

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

int builds_prepared_runtime_abort_external_call_object() {
  const auto prepared =
      make_prepared_runtime_external_call_module("abort", false);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared abort runtime external RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* caller = object::find_symbol(*module, "calls_abort");
  const auto* abort_symbol = object::find_symbol(*module, "abort");
  if (text == nullptr || caller == nullptr || abort_symbol == nullptr) {
    return fail("expected abort runtime external object symbols");
  }
  if (!object::is_undefined_symbol(*abort_symbol) ||
      abort_symbol->kind != object::SymbolKind::Function) {
    return fail("expected abort to remain an undefined function symbol");
  }
  if (caller->section != std::optional<object::SectionId>{text->id} ||
      caller->value != 0 || caller->size_bytes != text->bytes.size()) {
    return fail("expected abort caller symbol to cover emitted text");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != abort_symbol->id ||
      module->relocations[0].offset >= caller->size_bytes) {
    return fail("expected abort runtime external call relocation");
  }
  return 0;
}

int builds_prepared_runtime_exit_external_call_object() {
  const auto prepared = make_prepared_runtime_external_call_module("exit", true);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared exit runtime external RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* caller = object::find_symbol(*module, "calls_exit");
  const auto* exit_symbol = object::find_symbol(*module, "exit");
  if (text == nullptr || caller == nullptr || exit_symbol == nullptr) {
    return fail("expected exit runtime external object symbols");
  }
  if (!object::is_undefined_symbol(*exit_symbol) ||
      exit_symbol->kind != object::SymbolKind::Function) {
    return fail("expected exit to remain an undefined function symbol");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].type != R_RISCV_CALL_PLT ||
      module->relocations[0].symbol != exit_symbol->id) {
    return fail("expected exit runtime external call relocation");
  }
  const auto call_offset = module->relocations[0].offset;
  if (call_offset < 4 || call_offset + 8 > caller->size_bytes) {
    return fail("expected exit argument materialization before call pair");
  }
  if (!contains_u32(text->bytes, 0x00000513)) {
    return fail("expected exit immediate status to materialize in a0");
  }
  return 0;
}

int emits_prepared_runtime_external_module_text_policy() {
  const auto abort_prepared =
      make_prepared_runtime_external_call_module("abort", false);
  const auto abort_text = rv64::emit_prepared_module_text(abort_prepared);
  if (abort_text.find("    call abort\n") == std::string::npos) {
    return fail("expected prepared module policy to admit abort runtime external, got:\n" +
                abort_text);
  }

  const auto exit_prepared =
      make_prepared_runtime_external_call_module("exit", true);
  const auto exit_text = rv64::emit_prepared_module_text(exit_prepared);
  if (exit_text.find("    li a0, 0\n") == std::string::npos ||
      exit_text.find("    call exit\n") == std::string::npos) {
    return fail("expected prepared module policy to admit exit runtime external");
  }

  return 0;
}

int rejects_prepared_runtime_external_module_text_policy_fail_closed_shapes() {
  try {
    (void)rv64::emit_prepared_module_text(
        make_prepared_runtime_external_call_module("unknown_runtime", false));
    return fail("expected unknown runtime external to stay rejected");
  } catch (const std::invalid_argument& ex) {
    const std::string diagnostic = ex.what();
    if (diagnostic.find("unsupported_external_call") == std::string::npos ||
        diagnostic.find("callee='unknown_runtime'") == std::string::npos ||
        diagnostic.find("reason=unsupported runtime external symbol") ==
            std::string::npos) {
      return fail("expected precise unknown runtime external diagnostic");
    }
  }

  try {
    (void)rv64::emit_prepared_module_text(
        make_prepared_runtime_external_call_module("abort", true));
    return fail("expected nonzero-arity abort runtime external to stay rejected");
  } catch (const std::invalid_argument& ex) {
    const std::string diagnostic = ex.what();
    if (diagnostic.find("unsupported_external_call") == std::string::npos ||
        diagnostic.find("callee='abort'") == std::string::npos ||
        diagnostic.find("reason=unsupported runtime external signature") ==
            std::string::npos) {
      return fail("expected precise runtime external signature diagnostic");
    }
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

int builds_prepared_fused_ptr_register_compare_branch_object(
    bir::BinaryOpcode predicate,
    std::uint32_t expected_lhs_move,
    std::uint32_t expected_rhs_move,
    std::uint32_t expected_branch) {
  const auto prepared =
      make_prepared_fused_compare_branch_module(predicate, bir::TypeKind::Ptr);
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared fused pointer register compare branch RV64 object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "cmp_branch");
  const auto* true_label = object::find_symbol(*module, ".Lcmp_branch_is_true");
  const auto* false_label = object::find_symbol(*module, ".Lcmp_branch_is_false");
  if (text == nullptr || function == nullptr || true_label == nullptr ||
      false_label == nullptr) {
    return fail("expected fused pointer register compare branch object symbols and text");
  }
  if (text->bytes.size() != 32 || text->size_bytes != 32 ||
      function->value != 0 || function->size_bytes != 32 ||
      true_label->value != 16 || false_label->value != 24) {
    return fail("expected fused pointer register compare branch object text layout");
  }
  if (read_u32(text->bytes, 0) != expected_lhs_move ||
      read_u32(text->bytes, 4) != expected_rhs_move ||
      read_u32(text->bytes, 8) != expected_branch ||
      read_u32(text->bytes, 12) != 0x0000006f ||
      read_u32(text->bytes, 16) != 0x00100513 ||
      read_u32(text->bytes, 20) != 0x00008067 ||
      read_u32(text->bytes, 24) != 0x00000513 ||
      read_u32(text->bytes, 28) != 0x00008067) {
    return fail(("expected pointer register compare branch to lower through prepared authority; got " +
                 std::to_string(read_u32(text->bytes, 0)) + ", " +
                 std::to_string(read_u32(text->bytes, 4)) + ", " +
                 std::to_string(read_u32(text->bytes, 8)))
                    .c_str());
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
    return fail("expected fused pointer register compare branch local relocations");
  }
  return 0;
}

int builds_prepared_fused_ne_ptr_register_compare_branch_object() {
  return builds_prepared_fused_ptr_register_compare_branch_object(
      bir::BinaryOpcode::Ne,
      0x00028e13,
      0x00030e93,
      0x01de1063);
}

int builds_prepared_fused_eq_ptr_register_compare_branch_object() {
  return builds_prepared_fused_ptr_register_compare_branch_object(
      bir::BinaryOpcode::Eq,
      0x00028e13,
      0x00030e93,
      0x01de0063);
}

int builds_prepared_fused_ult_ptr_register_compare_branch_object() {
  return builds_prepared_fused_ptr_register_compare_branch_object(
      bir::BinaryOpcode::Ult,
      0x00028e13,
      0x00030e93,
      0x01de6063);
}

int builds_prepared_fused_uge_ptr_register_compare_branch_object() {
  return builds_prepared_fused_ptr_register_compare_branch_object(
      bir::BinaryOpcode::Uge,
      0x00028e13,
      0x00030e93,
      0x01de7063);
}

int builds_prepared_fused_ugt_ptr_register_compare_branch_object() {
  return builds_prepared_fused_ptr_register_compare_branch_object(
      bir::BinaryOpcode::Ugt,
      0x00030e13,
      0x00028e93,
      0x01de6063);
}

int builds_prepared_fused_ule_ptr_register_compare_branch_object() {
  return builds_prepared_fused_ptr_register_compare_branch_object(
      bir::BinaryOpcode::Ule,
      0x00030e13,
      0x00028e93,
      0x01de7063);
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
          make_prepared_fused_compare_branch_module(
              bir::BinaryOpcode::Ne,
              bir::TypeKind::Ptr,
              nonnull_pointer_immediate(8)),
          diagnostic) != 0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Slt,
                                                    bir::TypeKind::Ptr),
          diagnostic) != 0) {
    return 1;
  }

  auto missing_branch_condition =
      make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Ne,
                                                bir::TypeKind::Ptr);
  missing_branch_condition.control_flow.functions.front().branch_conditions.clear();
  if (expect_prepared_rejection_diagnostic(missing_branch_condition,
                                           diagnostic) != 0) {
    return 1;
  }

  auto missing_lhs_home =
      make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Ne,
                                                bir::TypeKind::Ptr);
  auto& missing_lhs_homes =
      missing_lhs_home.value_locations.functions.front().value_homes;
  missing_lhs_homes.erase(missing_lhs_homes.begin() + 1);
  if (expect_prepared_rejection_diagnostic(missing_lhs_home, diagnostic) != 0) {
    return 1;
  }

  auto target_mismatch =
      make_prepared_fused_compare_branch_module(bir::BinaryOpcode::Ne,
                                                bir::TypeKind::Ptr);
  target_mismatch.control_flow.functions.front().branch_conditions.front().false_label =
      target_mismatch.names.block_labels.intern("other");
  if (expect_prepared_rejection_diagnostic(target_mismatch, diagnostic) != 0) {
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

int reports_generic_fallback_context_for_prepared_rematerialized_instruction() {
  auto prepared = make_prepared_rematerialized_return_module();
  prepared.value_locations.functions.front().value_homes.erase(
      prepared.value_locations.functions.front().value_homes.begin());
  return expect_prepared_rejection_diagnostic_contains(
      prepared,
      {
          "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering",
          "function=main",
          "block=<none>",
          "block_index=0",
          "instruction_index=0",
          "instruction_kind=BinaryInst",
          "owner=i32 %t0",
      });
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

int builds_prepared_immediate_null_same_module_call_object() {
  const auto prepared = make_prepared_immediate_null_same_module_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared immediate/null same-module call to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* callee = object::find_symbol(module, "mix");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected prepared immediate/null call object to publish text/functions");
  }
  if (module.relocations.size() != 1 ||
      module.relocations[0].section != text->id ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != callee->id ||
      module.relocations[0].offset < main->value + 16) {
    return fail("expected immediate/null same-module call relocation in main");
  }
  const auto call_offset = module.relocations[0].offset;
  if (read_u32(text->bytes, call_offset - 8) != 0x06400513 ||
      read_u32(text->bytes, call_offset - 4) != 0x00000593) {
    return fail("expected same-module call to materialize immediate i32 and null pointer GPR args");
  }
  if (read_u32(text->bytes, call_offset + 8) != 0x00050293 ||
      read_u32(text->bytes, call_offset + 12) != 0x00028513 ||
      read_u32(text->bytes, call_offset + 24) != 0x00008067) {
    return fail("expected integer call result to publish from a0 to owner register before return use");
  }
  return 0;
}

int expect_scalar_register_result_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared, kUnsupportedSameModuleCallAbiDiagnostic);
}

int rejects_prepared_scalar_register_result_call_fail_closed_shapes() {
  auto prepared = make_prepared_scalar_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].result->destination_value_id =
      std::nullopt;
  if (expect_scalar_register_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_same_module_call_module();
  prepared.value_locations.functions[1].value_homes.clear();
  if (expect_scalar_register_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_same_module_call_module();
  prepared.value_locations.functions[1].value_homes[0].register_name =
      std::string{"s1"};
  if (expect_scalar_register_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].result->destination_register_bank =
      prepare::PreparedRegisterBank::Fpr;
  if (expect_scalar_register_result_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_same_module_call_module();
  prepared.call_plans.functions[0].calls[0].result->value_bank =
      prepare::PreparedRegisterBank::Fpr;
  if (expect_scalar_register_result_call_rejection(prepared) != 0) {
    return 1;
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
      prepared, kUnsupportedSameModuleCallAbiDiagnostic);
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

int builds_representative_prepared_same_module_sret_call_object() {
  const auto prepared = make_prepared_representative_same_module_sret_call_module();
  const auto& call_plan = prepared.call_plans.functions[0].calls[0];
  if (!call_plan.memory_return.has_value() ||
      call_plan.memory_return->sret_arg_index != std::optional<std::size_t>{0} ||
      call_plan.memory_return->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      call_plan.memory_return->size_bytes != 12 ||
      call_plan.memory_return->align_bytes != 4 ||
      call_plan.arguments.size() != 1 ||
      call_plan.arguments[0].value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      !call_plan.arguments[0].source_selection.has_value() ||
      call_plan.arguments[0].source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization ||
      call_plan.arguments[0].source_selection->source_size_bytes !=
          std::optional<std::size_t>{12} ||
      call_plan.arguments[0].source_selection->source_align_bytes !=
          std::optional<std::size_t>{4}) {
    return fail("representative same-module sret fixture lost prepared memory-return facts");
  }

  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected representative prepared same-module sret call RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* callee = object::find_symbol(module, "fill_result");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || callee == nullptr || main == nullptr) {
    return fail("expected representative same-module sret call object to publish text/functions");
  }
  if (text->bytes.size() != 36 || text->size_bytes != 36 ||
      callee->value != 0 || callee->size_bytes != 4 ||
      main->value != 4 || main->size_bytes != 32) {
    return fail("expected representative same-module sret call object text layout");
  }

  const std::size_t base = main->value;
  if (read_u32(text->bytes, base + 0) != 0xfd010113 ||
      read_u32(text->bytes, base + 4) != 0x02113423 ||
      read_u32(text->bytes, base + 8) != 0x01010513) {
    return fail("expected representative same-module sret call to pass frame-slot address in a0");
  }
  if (module.relocations.size() != 1 ||
      module.relocations[0].section != text->id ||
      module.relocations[0].offset != base + 12 ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != callee->id) {
    return fail("expected representative same-module sret call relocation");
  }
  if (read_u32(text->bytes, base + 20) != 0x02813083 ||
      read_u32(text->bytes, base + 24) != 0x03010113 ||
      read_u32(text->bytes, base + 28) != 0x00008067) {
    return fail("expected representative same-module sret call to restore stack and return");
  }
  return 0;
}

int expect_same_module_sret_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared, kUnsupportedSameModuleCallAbiDiagnostic);
}

int rejects_prepared_same_module_sret_call_fail_closed_shapes() {
  auto prepared = make_prepared_same_module_sret_call_module();
  prepared.call_plans.functions[0].calls[0].wrapper_kind =
      prepare::PreparedCallWrapperKind::DirectExternFixedArity;
  if (expect_prepared_rejection_diagnostic(
          prepared, kGenericUnsupportedInstructionFragmentDiagnostic) != 0) {
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
  prepared.call_plans.functions[0].calls[0].arguments[0]
      .source_selection->source_pointer_byte_delta = std::int64_t{-8};
  prepared.call_plans.functions[0].calls[0].arguments[0]
      .source_selection->address_materialization_byte_offset = std::int64_t{8};
  if (expect_same_module_sret_call_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_same_module_sret_call_module();
  prepared.frame_plan.functions[1].has_dynamic_stack = true;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame") !=
      0) {
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
      prepared, kUnsupportedSameModuleCallAbiDiagnostic);
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

int rejects_variadic_va_start_stack_backed_destination_address() {
  const auto prepared = make_prepared_variadic_va_start_module(
      true /*include_overflow_area_initial_state*/,
      true /*destination_va_list_is_stack_slot*/,
      false /*destination_address_is_gpr*/);
  const auto& va_start_homes =
      prepared.variadic_entry_plans.functions.front().helper_operand_homes.front();
  const auto* typed_va_start =
      prepare::find_prepared_variadic_va_start_operand_homes(va_start_homes);
  if (typed_va_start == nullptr ||
      typed_va_start->destination_va_list.kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      typed_va_start->destination_va_list.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{5}} ||
      typed_va_start->destination_va_list_address.kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      typed_va_start->destination_va_list_address.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{6}}) {
    return fail("expected RV64 va_start fixture to model stack-backed destination address");
  }
  return expect_prepared_rejection_diagnostic(
      std::move(prepared),
      "unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home");
}

int materializes_fact_complete_variadic_va_start_with_saved_gpr_publications() {
  const auto prepared =
      make_prepared_variadic_va_start_with_saved_gpr_publications_module();
  const auto& va_start_homes =
      prepared.variadic_entry_plans.functions.front().helper_operand_homes.front();
  const auto* typed_va_start =
      prepare::find_prepared_variadic_va_start_operand_homes(va_start_homes);
  if (typed_va_start == nullptr ||
      typed_va_start->destination_va_list.kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      typed_va_start->destination_va_list_address.register_name !=
          std::optional<std::string>{"a0"}) {
    return fail("expected RV64 va_start fixture to expose typed helper payload");
  }
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
  if (!prepared_rejection_diagnostic_matches(
          mismatched_callee.diagnostic,
          kGenericUnsupportedInstructionFragmentDiagnostic)) {
    return fail("expected mismatched va_end plan to fail closed before call relocation");
  }

  auto multiple_args =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(
          make_prepared_variadic_va_end_module("llvm.va_end.p0", 2));
  if (multiple_args.ok() || multiple_args.module.has_value()) {
    return fail("expected malformed two-argument va_end call to reject");
  }
  if (!prepared_rejection_diagnostic_matches(
          multiple_args.diagnostic,
          kGenericUnsupportedInstructionFragmentDiagnostic)) {
    return fail("expected malformed va_end arity to fail closed before call relocation");
  }
  return 0;
}

int materializes_fact_complete_variadic_aggregate_va_arg_helper() {
  const auto prepared =
      make_prepared_variadic_aggregate_va_arg_module(true);
  const auto& aggregate_homes =
      prepared.variadic_entry_plans.functions.front().helper_operand_homes.front();
  const auto* typed_aggregate =
      prepare::find_prepared_variadic_aggregate_va_arg_operand_homes(
          aggregate_homes);
  if (typed_aggregate == nullptr ||
      typed_aggregate->source_va_list.register_name !=
          std::optional<std::string>{"s1"} ||
      !typed_aggregate->aggregate_access_plan.payload_write_address.has_value()) {
    return fail("expected RV64 aggregate va_arg fixture to expose typed helper payload");
  }
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

int builds_prepared_prior_result_multi_gpr_same_module_call_object() {
  const auto prepared =
      make_prepared_prior_result_multi_gpr_same_module_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared prior-result multi-GPR same-module call to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* seed = object::find_symbol(module, "seed");
  const auto* combine = object::find_symbol(module, "combine");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || seed == nullptr || combine == nullptr || main == nullptr) {
    return fail("expected prior-result multi-GPR call object to publish text/functions");
  }
  if (module.relocations.size() != 2 ||
      module.relocations[0].section != text->id ||
      module.relocations[1].section != text->id ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[1].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != seed->id ||
      module.relocations[1].symbol != combine->id ||
      module.relocations[0].offset >= module.relocations[1].offset ||
      module.relocations[0].offset < main->value + 8 ||
      module.relocations[1].offset < main->value + 32) {
    return fail("expected ordered same-module seed/combine call relocations in main");
  }
  const auto first_call_offset = module.relocations[0].offset;
  const auto second_call_offset = module.relocations[1].offset;
  if (read_u32(text->bytes, first_call_offset + 8) != 0x00050293) {
    return fail("expected first call result to publish from a0 to %first owner register");
  }
  if (read_u32(text->bytes, second_call_offset - 12) != 0x00028513 ||
      read_u32(text->bytes, second_call_offset - 8) != 0x00700593 ||
      read_u32(text->bytes, second_call_offset - 4) != 0x00000613) {
    return fail("expected second same-module call to consume prior result plus GPR immediates");
  }
  if (read_u32(text->bytes, second_call_offset + 8) != 0x00050313 ||
      read_u32(text->bytes, second_call_offset + 12) != 0x00030513 ||
      read_u32(text->bytes, second_call_offset + 24) != 0x00008067) {
    return fail("expected second call result to publish before later return use");
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

int builds_prepared_ptrtoint_param_survives_nested_same_module_call_object() {
  const auto prepared =
      make_prepared_ptrtoint_param_survives_nested_same_module_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared ptrtoint-param nested-call RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* consume = object::find_symbol(module, "consume_bits");
  const auto* keep = object::find_symbol(module, "nested_keep");
  const auto* main = object::find_symbol(module, "ptrtoint_param_survives_call");
  if (text == nullptr || consume == nullptr || keep == nullptr || main == nullptr) {
    return fail("expected ptrtoint-param nested-call object to publish text/functions");
  }
  if (module.relocations.size() != 2 ||
      module.relocations[0].section != text->id ||
      module.relocations[1].section != text->id ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[1].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != keep->id ||
      module.relocations[1].symbol != consume->id ||
      module.relocations[0].offset >= module.relocations[1].offset ||
      module.relocations[0].offset < main->value ||
      module.relocations[1].offset >= main->value + main->size_bytes) {
    return fail("expected ordered nested_keep/consume_bits same-module call relocations");
  }
  const auto keep_call_offset = module.relocations[0].offset;
  const auto consume_call_offset = module.relocations[1].offset;
  bool materialized_c_to_d = false;
  for (std::size_t offset = main->value; offset + 4 <= keep_call_offset;
       offset += 4) {
    if (read_u32(text->bytes, offset) == 0x00060493) {
      materialized_c_to_d = true;
      break;
    }
  }
  if (!materialized_c_to_d) {
    return fail("expected incoming pointer parameter in a2 to materialize as ptrtoint local in s1 before nested call");
  }
  if (read_u32(text->bytes, consume_call_offset - 4) != 0x00048513) {
    return fail("expected nested-call-preserved ptrtoint local in s1 to feed a0 for later same-module call");
  }
  const auto epilogue_offset = main->value + main->size_bytes - 16;
  if (read_u32(text->bytes, epilogue_offset + 0) != 0x00013483 ||
      read_u32(text->bytes, epilogue_offset + 4) != 0x01813083 ||
      read_u32(text->bytes, epilogue_offset + 8) != 0x02010113 ||
      read_u32(text->bytes, epilogue_offset + 12) != 0x00008067) {
    return fail("expected s1 preservation to survive through function epilogue");
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
          prepared, kGenericUnsupportedInstructionFragmentDiagnostic) !=
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
          prepared, kGenericUnsupportedInstructionFragmentDiagnostic) !=
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
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved save slots") !=
      0) {
    return 1;
  }

  prepared = make_prepared_prior_preserved_arg_call_module();
  auto& fpr_saved = prepared.frame_plan.functions[0].saved_callee_registers[0];
  fpr_saved.bank = prepare::PreparedRegisterBank::Fpr;
  fpr_saved.register_name = "fs1";
  fpr_saved.occupied_register_names = {"fs1"};
  fpr_saved.placement->bank = prepare::PreparedRegisterBank::Fpr;
  fpr_saved.slot_placement->bank = prepare::PreparedRegisterBank::Fpr;
  fpr_saved.slot_placement->register_name = "fs1";
  fpr_saved.slot_placement->occupied_register_names = {"fs1"};
  fpr_saved.slot_placement->register_placement = fpr_saved.placement;
  fpr_saved.slot_placement->size_bytes = std::size_t{4};
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires supported prepared callee-saved save slots") !=
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
          prepared, kGenericUnsupportedInstructionFragmentDiagnostic) !=
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

int expect_stack_passed_scalar_param_home_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_param_home: RV64 object route requires stack-passed scalar formal homes to match prepared frame-slot facts");
}

int builds_stack_passed_scalar_param_home_object() {
  const auto prepared = make_prepared_stack_passed_scalar_param_home_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared stack-passed scalar formal home to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function =
      object::find_symbol(*module, "stack_passed_scalar_param_home");
  if (text == nullptr || function == nullptr) {
    return fail("expected stack-passed scalar formal object to publish text/function");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      function->value != 0 || function->size_bytes != 12 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected stack-passed scalar formal object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfc010113 ||
      read_u32(text->bytes, 4) != 0x04010113 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected stack-passed formal to use prepared home without entry register store");
  }
  if (!module->relocations.empty()) {
    return fail("expected stack-passed scalar formal object to need no relocations");
  }

  const auto floating =
      make_prepared_stack_passed_scalar_param_home_module(bir::TypeKind::F64,
                                                         bir::AbiValueClass::Sse);
  if (!rv64::build_rv64_prepared_text_object_module(floating).has_value()) {
    return fail("expected prepared stack-passed F64 formal home to build");
  }
  return 0;
}

int rejects_stack_passed_scalar_param_home_fail_closed_shapes() {
  auto prepared = make_prepared_stack_passed_scalar_param_home_module();
  prepared.value_locations.functions[0].value_homes[0].offset_bytes =
      std::size_t{56};
  if (expect_stack_passed_scalar_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_passed_scalar_param_home_module();
  prepared.stack_layout.frame_slots[0].size_bytes = 4;
  if (expect_stack_passed_scalar_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_passed_scalar_param_home_module();
  prepared.stack_layout.objects[0].source_kind = "local";
  if (expect_stack_passed_scalar_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_passed_scalar_param_home_module();
  prepared.module.functions[0].params[0].abi->passed_on_stack = false;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes") !=
      0) {
    return 1;
  }

  prepared = make_prepared_stack_passed_scalar_param_home_module();
  prepared.module.functions[0].params[0].abi->primary_class =
      bir::AbiValueClass::Memory;
  if (expect_stack_passed_scalar_param_home_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_stack_passed_scalar_param_home_module(
      bir::TypeKind::F128, bir::AbiValueClass::Sse);
  if (expect_stack_passed_scalar_param_home_rejection(prepared) != 0) {
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

int records_prepared_fpr_callee_saved_frame_slot_facts() {
  const auto saved = make_prepared_fpr_callee_saved_fs1();
  const auto offset =
      rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 40);
  if (offset != std::optional<std::int32_t>{24}) {
    return fail("expected prepared FPR callee-saved slot offset fact");
  }
  if (saved.bank != prepare::PreparedRegisterBank::Fpr ||
      saved.register_name != "fs1" || saved.save_index != 0 ||
      !saved.slot_placement.has_value() ||
      saved.slot_placement->slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{11}} ||
      saved.slot_placement->stack_offset_bytes != std::optional<std::size_t>{24} ||
      saved.slot_placement->size_bytes != std::optional<std::size_t>{8} ||
      saved.slot_placement->align_bytes != std::optional<std::size_t>{8}) {
    return fail("expected explicit prepared FPR saved-register slot facts");
  }
  return 0;
}

int rejects_malformed_prepared_fpr_callee_saved_frame_slot_facts() {
  auto saved = make_prepared_fpr_callee_saved_fs1();
  saved.slot_placement = std::nullopt;
  if (rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 40).has_value()) {
    return fail("expected missing prepared FPR saved slot to fail closed");
  }

  saved = make_prepared_fpr_callee_saved_fs1();
  saved.slot_placement->save_index = 1;
  if (rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 40).has_value()) {
    return fail("expected mismatched prepared FPR save index to fail closed");
  }

  saved = make_prepared_fpr_callee_saved_fs1();
  saved.slot_placement->size_bytes = std::size_t{4};
  if (rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 40).has_value()) {
    return fail("expected malformed prepared FPR slot size to fail closed");
  }

  saved = make_prepared_fpr_callee_saved_fs1();
  saved.slot_placement->fixed_location = false;
  if (rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 40).has_value()) {
    return fail("expected non-fixed prepared FPR saved slot to fail closed");
  }

  saved = make_prepared_fpr_callee_saved_fs1();
  saved.placement->bank = prepare::PreparedRegisterBank::Gpr;
  if (rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 40).has_value()) {
    return fail("expected mismatched prepared FPR placement to fail closed");
  }

  saved = make_prepared_fpr_callee_saved_fs1();
  if (rv64::rv64_prepared_saved_callee_fpr_stack_offset(saved, 16).has_value()) {
    return fail("expected out-of-frame prepared FPR saved slot to fail closed");
  }

  return 0;
}

int records_prepared_gpr_callee_saved_frame_slot_facts() {
  const auto direct = make_prepared_gpr_callee_saved_s1(24);
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(direct, 40) !=
      std::optional<std::int32_t>{24}) {
    return fail("expected direct prepared GPR callee-saved slot offset fact");
  }

  const auto large = make_prepared_gpr_callee_saved_s1(80000);
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(large, 80016) !=
      std::optional<std::int32_t>{80000}) {
    return fail("expected large prepared GPR callee-saved slot offset fact");
  }

  if (large.bank != prepare::PreparedRegisterBank::Gpr ||
      large.register_name != "s1" || large.save_index != 0 ||
      !large.slot_placement.has_value() ||
      large.slot_placement->slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{10000}} ||
      large.slot_placement->stack_offset_bytes !=
          std::optional<std::size_t>{80000} ||
      large.slot_placement->size_bytes != std::optional<std::size_t>{8} ||
      large.slot_placement->align_bytes != std::optional<std::size_t>{8}) {
    return fail("expected explicit prepared GPR saved-register slot facts");
  }

  return 0;
}

int rejects_malformed_prepared_gpr_callee_saved_frame_slot_facts() {
  auto saved = make_prepared_gpr_callee_saved_s1(80000);
  saved.slot_placement = std::nullopt;
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(saved, 80016)
          .has_value()) {
    return fail("expected missing prepared GPR saved slot to fail closed");
  }

  saved = make_prepared_gpr_callee_saved_s1(80000);
  saved.slot_placement->save_index = 1;
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(saved, 80016)
          .has_value()) {
    return fail("expected mismatched prepared GPR save index to fail closed");
  }

  saved = make_prepared_gpr_callee_saved_s1(80000);
  saved.slot_placement->size_bytes = std::size_t{4};
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(saved, 80016)
          .has_value()) {
    return fail("expected malformed prepared GPR slot size to fail closed");
  }

  saved = make_prepared_gpr_callee_saved_s1(80000);
  saved.slot_placement->fixed_location = false;
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(saved, 80016)
          .has_value()) {
    return fail("expected non-fixed prepared GPR saved slot to fail closed");
  }

  saved = make_prepared_gpr_callee_saved_s1(80000);
  saved.placement->bank = prepare::PreparedRegisterBank::Fpr;
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(saved, 80016)
          .has_value()) {
    return fail("expected mismatched prepared GPR placement to fail closed");
  }

  saved = make_prepared_gpr_callee_saved_s1(80000);
  if (rv64::rv64_prepared_saved_callee_gpr_stack_offset(saved, 80000)
          .has_value()) {
    return fail("expected out-of-frame prepared GPR saved slot to fail closed");
  }

  return 0;
}

int materializes_large_offset_prepared_gpr_callee_saved_frame_slots() {
  const auto prepared = make_prepared_gpr_callee_saved_frame_module(80016, 80000);
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared large-offset GPR callee-saved frame object "
                "to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* function = object::find_symbol(*result.module, "main");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared large-offset GPR object to publish text/main");
  }
  if (function->value != 0 || function->section !=
                                std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared large-offset GPR main symbol layout");
  }

  const std::vector<std::uint32_t> large_save = {
      0x01400313,  // addi t1, zero, 20
      0x00c31313,  // slli t1, t1, 12
      0x88030313,  // addi t1, t1, -1920
      0x00610333,  // add t1, sp, t1
      0x00933023,  // sd s1, 0(t1)
  };
  const std::vector<std::uint32_t> large_restore = {
      0x01400313,  // addi t1, zero, 20
      0x00c31313,  // slli t1, t1, 12
      0x88030313,  // addi t1, t1, -1920
      0x00610333,  // add t1, sp, t1
      0x00033483,  // ld s1, 0(t1)
  };
  if (!contains_u32_sequence(text->bytes, large_save) ||
      !contains_u32_sequence(text->bytes, large_restore)) {
    return fail("expected large prepared GPR callee-saved save/restore "
                "address materialization sequence");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected large prepared GPR callee-saved object to need no relocations");
  }
  return 0;
}

int materializes_prepared_fpr_callee_saved_frame_slots() {
  const auto prepared = make_prepared_fpr_callee_saved_frame_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared FPR callee-saved frame object to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* function = object::find_symbol(*result.module, "main");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared FPR callee-saved object to publish text/function");
  }
  if (text->bytes.size() != 44 || text->size_bytes != 44 ||
      function->value != 0 || function->size_bytes != 44) {
    return fail("expected prepared FPR callee-saved object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfd010113 ||
      read_u32(text->bytes, 4) != 0x00913c27 ||
      read_u32(text->bytes, 8) != 0x03213027 ||
      read_u32(text->bytes, 12) != 0x00500313 ||
      read_u32(text->bytes, 16) != 0x00612023 ||
      read_u32(text->bytes, 20) != 0x00012283 ||
      read_u32(text->bytes, 24) != 0x00028513 ||
      read_u32(text->bytes, 28) != 0x02013907 ||
      read_u32(text->bytes, 32) != 0x01813487 ||
      read_u32(text->bytes, 36) != 0x03010113 ||
      read_u32(text->bytes, 40) != 0x00008067) {
    return fail("expected prepared FPR callee-saved fsd/fld sequence");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected prepared FPR callee-saved object to need no relocations");
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

int builds_prepared_large_fixed_stack_frame_adjustment_object() {
  const auto prepared = make_prepared_large_fixed_stack_frame_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared large fixed stack-frame RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected large fixed stack-frame object to publish text/main");
  }
  if (text->bytes.size() != 32 || text->size_bytes != 32 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 32) {
    return fail("expected large fixed stack-frame object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfff00293 ||
      read_u32(text->bytes, 4) != 0x00c29293 ||
      read_u32(text->bytes, 8) != 0x00510133 ||
      read_u32(text->bytes, 12) != 0x00000513 ||
      read_u32(text->bytes, 16) != 0x00100293 ||
      read_u32(text->bytes, 20) != 0x00c29293 ||
      read_u32(text->bytes, 24) != 0x00510133 ||
      read_u32(text->bytes, 28) != 0x00008067) {
    return fail("expected materialized large stack-pointer adjustment sequence");
  }
  if (!module->relocations.empty()) {
    return fail("expected large fixed stack-frame object to need no relocations");
  }
  return 0;
}

int builds_prepared_large_fixed_slot_addressing_object() {
  const auto prepared = make_prepared_large_fixed_slot_addressing_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared large fixed-slot addressing object to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* main_symbol = object::find_symbol(*result.module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected large fixed-slot addressing object to publish text/main");
  }
  if (main_symbol->value != 0 || main_symbol->size_bytes != text->size_bytes) {
    return fail("expected large fixed-slot addressing symbol bounds");
  }
  if (!contains_u32(text->bytes, 0x00830313) ||
      !contains_u32(text->bytes, 0x00610333)) {
    return fail("expected frame-slot address materialization above 12-bit offset");
  }
  if (!contains_u32(text->bytes, 0x0063b023) ||
      !contains_u32(text->bytes, 0x0063a023) ||
      !contains_u32(text->bytes, 0x00032283)) {
    return fail("expected large fixed-slot pointer store, scalar store, and scalar load");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected large fixed-slot addressing object to need no relocations");
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

int builds_prepared_pointer_result_frame_slot_address_materialization_object() {
  const auto prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared dynamic pointer-result frame-slot address object to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* function = object::find_symbol(*result.module,
                                             "dynamic_pointer_result");
  if (text == nullptr || function == nullptr) {
    return fail("expected dynamic pointer-result frame-slot address object to publish text/function");
  }
  if (function->value != 0 || function->size_bytes != text->size_bytes ||
      text->size_bytes != text->bytes.size()) {
    return fail("expected dynamic pointer-result frame-slot address object text layout");
  }
  if (!contains_frame_slot_dynamic_pointer_result_publication(
          text->bytes,
          9U,
          80,
          88)) {
    return fail("expected stack offset load, dynamic pointer add, and frame-slot pointer result publication");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected dynamic pointer-result frame-slot address object to need no relocations");
  }
  return 0;
}

int expect_pointer_result_frame_slot_address_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared,
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering");
}

int rejects_prepared_pointer_result_frame_slot_address_fail_closed_shapes() {
  auto prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.addressing.functions[0].address_materializations.clear();
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  auto duplicate = prepared.addressing.functions[0].address_materializations[0];
  duplicate.byte_offset = 32;
  prepared.addressing.functions[0].address_materializations.push_back(duplicate);
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.addressing.functions[0].address_materializations[0].kind =
      prepare::PreparedAddressMaterializationKind::DirectGlobal;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.addressing.functions[0].address_materializations[0].address_space =
      bir::AddressSpace::Tls;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.addressing.functions[0].address_materializations[0].is_thread_local =
      true;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.addressing.functions[0].address_materializations[0].frame_slot_id =
      std::nullopt;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.addressing.functions[0].address_materializations[0].byte_offset = -8;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.value_locations.functions[0].value_homes[0].kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions[0].value_homes[0].register_name =
      std::nullopt;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.value_locations.functions[0].value_homes[1].offset_bytes =
      std::nullopt;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
  }

  prepared =
      make_prepared_pointer_result_frame_slot_address_materialization_module();
  prepared.value_locations.functions[0].value_homes[2].offset_bytes =
      std::nullopt;
  if (expect_pointer_result_frame_slot_address_rejection(prepared) != 0) {
    return 1;
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

int builds_prepared_pointer_value_scalar_stack_home_local_object() {
  const auto prepared =
      make_prepared_pointer_value_scalar_stack_home_local_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared pointer-value stack-home local RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected prepared pointer-value stack-home local object to publish text/main");
  }
  if (text->bytes.size() < 32 || text->size_bytes != text->bytes.size() ||
      main_symbol->value != 0 || main_symbol->size_bytes != text->bytes.size()) {
    return fail("expected prepared pointer-value stack-home local object text layout");
  }
  if (!contains_u32_sequence(text->bytes,
                             {0x00013383,
                              0x00900313,
                              0x00639123,
                              0x00013383,
                              0x00239283})) {
    return fail("expected pointer-value stack-home local object to load the base from its stack home before sh/lh");
  }
  if (!module->relocations.empty()) {
    return fail("expected pointer-value stack-home local object to need no relocations");
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

int builds_prepared_pointer_value_i8_local_store_object() {
  const auto prepared = make_prepared_pointer_value_i8_local_store_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared pointer-value i8 local store RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "pointer_value_i8_store");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared pointer-value i8 store object to publish text/function");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      function->value != 0 || function->size_bytes != 12 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared pointer-value i8 store object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028313 ||
      read_u32(text->bytes, 4) != 0x00648023 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected pointer-value i8 local store to emit mv t1, t0; sb t1, 0(s1); ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected pointer-value i8 local store object to need no relocations");
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

int rejects_prepared_pointer_value_i8_local_store_fail_closed_shapes() {
  auto prepared = make_prepared_pointer_value_i8_local_store_module();
  prepared.addressing.functions[0].accesses[0].address.pointer_value_name =
      std::nullopt;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_i8_local_store_module();
  prepared.value_locations.functions[0].value_homes[0].register_name =
      std::nullopt;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  prepared = make_prepared_pointer_value_i8_local_store_module();
  prepared.addressing.functions[0].accesses[0].address.can_use_base_plus_offset =
      false;
  if (expect_pointer_value_scalar_local_rejection(prepared) != 0) {
    return 1;
  }

  return 0;
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

int builds_prepared_before_return_stack_to_register_abi_move_object() {
  const auto prepared =
      make_prepared_before_return_stack_to_register_abi_move_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared before-return stack-to-GPR ABI move object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "stack_return_move");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared stack-to-return-register object to publish text/function");
  }
  if (text->bytes.size() != 16 || text->size_bytes != 16 ||
      function->value != 0 || function->size_bytes != 16 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared stack-to-return-register text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00811503 ||
      read_u32(text->bytes, 8) != 0x01010113 ||
      read_u32(text->bytes, 12) != 0x00008067) {
    return fail("expected lh a0, 8(sp) before stack-frame epilogue and ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared stack-to-return-register object to need no relocations");
  }
  return 0;
}

int keeps_generic_return_load_for_mismatched_before_return_stack_move() {
  auto prepared = make_prepared_before_return_stack_to_register_abi_move_module(
      bir::TypeKind::I32, 4);
  const auto function_name =
      prepared.names.function_names.intern("stack_return_move");
  const auto other_name = prepared.names.value_names.intern("%other");
  const auto base_name = prepared.names.value_names.intern("%base");

  prepared.module.functions[0].blocks[0].insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%ret"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%base"),
      .rhs = bir::Value::immediate_i32(0),
  });
  prepared.module.functions[0].blocks[0].terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%other");
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{7},
      .function_name = function_name,
      .offset_bytes = 12,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.value_locations.functions[0].value_homes.push_back(
      rv64_sized_stack_slot_home(2,
                                 function_name,
                                 other_name,
                                 prepare::PreparedFrameSlotId{7},
                                 12,
                                 4));
  prepared.value_locations.functions[0].value_homes.push_back(
      rv64_gpr_home(3, function_name, base_name, "t0", 5));

  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected mismatched before-return stack move to build safely, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* function = object::find_symbol(module, "stack_return_move");
  if (text == nullptr || function == nullptr) {
    return fail("expected mismatched stack return object to publish text/function");
  }
  if (text->bytes.size() != 28 || text->size_bytes != 28 ||
      function->value != 0 || function->size_bytes != 28 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected mismatched stack return to keep generic return load");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00028f13 ||
      read_u32(text->bytes, 8) != 0x01e12423 ||
      read_u32(text->bytes, 12) != 0x00812503 ||
      read_u32(text->bytes, 16) != 0x00c12503 ||
      read_u32(text->bytes, 20) != 0x01010113 ||
      read_u32(text->bytes, 24) != 0x00008067) {
    return fail("expected prepared move load followed by generic %other return load");
  }
  if (!module.relocations.empty()) {
    return fail("expected mismatched stack return object to need no relocations");
  }
  return 0;
}

int rejects_prepared_before_return_stack_to_register_abi_move_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].value_homes.clear();
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("missing source home shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  const auto function_name = prepared.names.function_names.intern("stack_return_move");
  const auto source_name = prepared.names.value_names.intern("%ret");
  prepared.value_locations.functions[0].value_homes[0] =
      rv64_gpr_home(1, function_name, source_name, "t0", 5);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("non-stack source shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_register_name = std::string{"fa0"};
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_register_placement->bank = prepare::PreparedRegisterBank::Fpr;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("non-GPR destination bank shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_register_name = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("missing destination register shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module(
      bir::TypeKind::F128, 16);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("unsupported source size shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_kind = prepare::PreparedMoveDestinationKind::Value;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("destination kind confusion shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0].phase =
      prepare::PreparedMovePhase::BeforeInstruction;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("before-instruction phase shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0].authority_kind =
      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("out-of-SSA authority shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .source_immediate_i32 = 1;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("immediate source shape should reject");
  }

  prepared = make_prepared_before_return_stack_to_register_abi_move_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .source_parallel_copy_step_index = 0;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("parallel-copy step source shape should reject");
  }

  return 0;
}

int builds_prepared_register_to_stack_before_instruction_move_bundle_object() {
  const auto prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared GPR to stack-slot move-bundle RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "main");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared GPR-to-stack move object to publish text/main");
  }
  if (text->bytes.size() < 8 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared GPR-to-stack move object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xfe010113 ||
      read_u32(text->bytes, 4) != 0x00512823) {
    return fail("expected sw t0, 16(sp) before prepared compare instruction");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared GPR-to-stack move object to need no relocations");
  }
  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (!image.ok() || !image.image.has_value() ||
      image.prepared_consumer_category.has_value() ||
      !image.diagnostic.empty()) {
    return fail("expected prepared GPR-to-stack ELF writer to accept same-width register-source stack-destination move");
  }
  return 0;
}

int rejects_prepared_register_to_stack_move_bundle_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.value_locations.functions[0].move_bundles[0].moves[0].reason =
      "consumer_stack_to_stack";
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.value_locations.functions[0].value_homes[0] = rv64_stack_slot_home(
      1,
      prepared.names.function_names.find("main"),
      prepared.names.value_names.find("%lhs"),
      prepare::PreparedFrameSlotId{13},
      8);
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{13},
      .function_name = prepared.names.function_names.find("main"),
      .offset_bytes = 8,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.value_locations.functions[0].move_bundles[0].phase =
      prepare::PreparedMovePhase::BeforeReturn;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .destination_stack_offset_bytes = 16;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.value_locations.functions[0].move_bundles[0].moves.push_back(
      prepared.value_locations.functions[0].move_bundles[0].moves[0]);
  if (expect_prepared_consumer_rejection_diagnostic(
          prepared,
          prepare::PreparedObjectConsumerDiagnosticCategory::
              AmbiguousNonParallelMultiSourceStackDestination,
          "prepared move-bundle classifier rejected ambiguous non-parallel "
          "multi-source stack-destination authority") != 0) {
    return fail("multiple register-source stack-destination moves should reject");
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  auto& conversion_locations = prepared.value_locations.functions[0];
  auto& conversion_move = conversion_locations.move_bundles[0].moves[0];
  conversion_locations.value_homes[2] =
      rv64_gpr_home(3,
                    prepared.names.function_names.find("main"),
                    prepared.names.value_names.find("%trunc"),
                    "a4",
                    14);
  conversion_move.from_value_id = 3;
  conversion_move.reason = "consumer_stack_to_stack";
  const auto conversion_mismatch =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (conversion_mismatch.ok() || conversion_mismatch.module.has_value()) {
    return fail("register-source conversion classified as stack-to-stack should reject");
  }
  if (conversion_mismatch.prepared_consumer_category.has_value() ||
      conversion_mismatch.diagnostic.find(
          "unsupported_prepared_move_bundle_classification: register-source "
          "stack-destination conversion move was classified as "
          "consumer_stack_to_stack") != 0 ||
      conversion_mismatch.diagnostic.find("move[0].reason=consumer_stack_to_stack") ==
          std::string::npos ||
      conversion_mismatch.diagnostic.find("move[0].source_home_kind=register") ==
          std::string::npos ||
      conversion_mismatch.diagnostic.find(
          "move[0].destination_home_kind=stack_slot") == std::string::npos ||
      conversion_mismatch.diagnostic.find("move[0].source_type=i16") ==
          std::string::npos ||
      conversion_mismatch.diagnostic.find("move[0].destination_type=i32") ==
          std::string::npos ||
      conversion_mismatch.diagnostic.find(
          "diagnostic_owner=prepared_move_bundle_classifier") ==
          std::string::npos ||
      conversion_mismatch.diagnostic.find(
          "fragment_status=producer_classification_rejected_register_source_"
          "stack_destination_conversion") == std::string::npos ||
      conversion_mismatch.diagnostic.find("generic_move_bundle_materialization_failed") !=
          std::string::npos) {
    return fail("conversion-aware register-source stack-destination mismatch should produce classifier diagnostic, got `" +
                conversion_mismatch.diagnostic + "`");
  }
  const auto conversion_mismatch_image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(
          prepared);
  if (conversion_mismatch_image.ok() ||
      conversion_mismatch_image.image.has_value() ||
      conversion_mismatch_image.prepared_consumer_category.has_value() ||
      conversion_mismatch_image.diagnostic != conversion_mismatch.diagnostic) {
    return fail("ELF writer should preserve classifier-owned register-source stack-destination conversion diagnostic");
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.value_locations.functions[0].value_homes[0].value_name =
      prepared.names.value_names.intern("%prepared.only.source");
  prepared.value_locations.functions[0].value_homes[0].size_bytes =
      std::nullopt;
  const auto missing_source_size =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (missing_source_size.ok() || missing_source_size.module.has_value()) {
    return fail("missing register-source size authority should reject");
  }
  if (missing_source_size.prepared_consumer_category.has_value() ||
      missing_source_size.diagnostic.find(diagnostic) != 0 ||
      missing_source_size.diagnostic.find(
          "move[0].reason=consumer_register_to_stack") == std::string::npos ||
      missing_source_size.diagnostic.find(
          "move[0].source_home_kind=register") == std::string::npos ||
      missing_source_size.diagnostic.find("move[0].source_type=") !=
          std::string::npos ||
      missing_source_size.diagnostic.find(
          "move[0].destination_home_kind=stack_slot") == std::string::npos ||
      missing_source_size.diagnostic.find("move[0].destination_type=i32") ==
          std::string::npos ||
      missing_source_size.diagnostic.find(
          "fragment_status=generic_move_bundle_materialization_failed") ==
          std::string::npos) {
    return fail("missing register-source size authority should produce a semantic move-bundle diagnostic");
  }
  const auto missing_source_size_image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(
          prepared);
  if (missing_source_size_image.ok() ||
      missing_source_size_image.image.has_value() ||
      missing_source_size_image.prepared_consumer_category.has_value() ||
      missing_source_size_image.diagnostic !=
          missing_source_size.diagnostic) {
    return fail("ELF writer should preserve missing source-size diagnostic");
  }

  return 0;
}

int rejects_ambiguous_non_parallel_multi_source_stack_destination_move_bundle() {
  auto prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto rhs_name = prepared.names.value_names.intern("%rhs");

  auto* compare = std::get_if<bir::BinaryInst>(
      &prepared.module.functions[0].blocks[0].insts[0]);
  if (compare == nullptr) {
    return fail("expected fixture to keep binary compare owner");
  }
  compare->rhs = bir::Value::named(bir::TypeKind::I32, "%rhs");

  auto& locations = prepared.value_locations.functions[0];
  locations.value_homes.push_back(
      rv64_gpr_home(4, function_name, rhs_name, "s2", 18));
  auto& bundle = locations.move_bundles[0];
  bundle.moves.push_back(prepare::PreparedMoveResolution{
      .from_value_id = 4,
      .to_value_id = 2,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
      .destination_contiguous_width = 1,
      .block_index = 0,
      .instruction_index = 0,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "consumer_register_to_stack",
  });

  if (bundle.phase != prepare::PreparedMovePhase::BeforeInstruction ||
      bundle.authority_kind != prepare::PreparedMoveAuthorityKind::None ||
      prepared.control_flow.functions[0].parallel_copy_bundles.empty() ==
          false ||
      bundle.moves.size() != 2 ||
      bundle.moves[0].from_value_id == bundle.moves[1].from_value_id ||
      bundle.moves[0].to_value_id != bundle.moves[1].to_value_id) {
    return fail("ambiguous stack-destination fixture did not publish the intended authority shape");
  }

  const auto two_source_result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (two_source_result.ok() || two_source_result.module.has_value() ||
      two_source_result.prepared_consumer_category !=
          prepare::PreparedObjectConsumerDiagnosticCategory::
              AmbiguousNonParallelMultiSourceStackDestination ||
      two_source_result.diagnostic.find(
          "unsupported_prepared_move_bundle_classification: "
          "non-parallel register-source fan-in to one stack destination "
          "has no ordering or mutually-exclusive authority") != 0 ||
      two_source_result.diagnostic.find(
          "fragment_status=producer_authority_missing_for_register_fan_in_stack_destination") ==
          std::string::npos) {
    return fail("two-register-source stack destination should reject with narrowed authority diagnostic");
  }
  const auto two_source_image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(
          prepared);
  if (two_source_image.ok() || two_source_image.image.has_value() ||
      two_source_image.prepared_consumer_category !=
          two_source_result.prepared_consumer_category ||
      two_source_image.diagnostic != two_source_result.diagnostic) {
    return fail("ELF writer should preserve narrowed two-source stack-destination diagnostic");
  }

  auto generic_ambiguous =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  const auto generic_function_name =
      generic_ambiguous.names.function_names.find("main");
  const auto generic_rhs_name =
      generic_ambiguous.names.value_names.intern("%rhs");
  const auto generic_stack_name =
      generic_ambiguous.names.value_names.intern("%stack.source");
  auto& generic_locations = generic_ambiguous.value_locations.functions[0];
  generic_locations.value_homes.push_back(
      rv64_gpr_home(4, generic_function_name, generic_rhs_name, "s2", 18));
  generic_locations.value_homes.push_back(
      rv64_stack_slot_home(5,
                           generic_function_name,
                           generic_stack_name,
                           prepare::PreparedFrameSlotId{12},
                           16));
  auto& generic_bundle = generic_locations.move_bundles[0];
  generic_bundle.moves.push_back(prepare::PreparedMoveResolution{
      .from_value_id = 4,
      .to_value_id = 2,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
      .destination_contiguous_width = 1,
      .block_index = 0,
      .instruction_index = 0,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "consumer_register_to_stack",
  });
  generic_bundle.moves.push_back(prepare::PreparedMoveResolution{
      .from_value_id = 5,
      .to_value_id = 2,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
      .destination_contiguous_width = 1,
      .block_index = 0,
      .instruction_index = 0,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "consumer_stack_to_stack",
  });
  if (expect_prepared_consumer_rejection_diagnostic(
          generic_ambiguous,
          prepare::PreparedObjectConsumerDiagnosticCategory::
              AmbiguousNonParallelMultiSourceStackDestination,
          "prepared move-bundle classifier rejected ambiguous non-parallel "
          "multi-source stack-destination authority") != 0) {
    return fail("generic ambiguous multi-source stack-destination move bundle should reject");
  }

  prepared =
      make_prepared_before_instruction_register_to_stack_move_bundle_module();
  prepared.module.functions[0].blocks[0].insts[0] = bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "%trunc"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "%rhs"),
  };
  prepared.value_locations.functions[0].value_homes.push_back(
      rv64_gpr_home(4, function_name, rhs_name, "s2", 18));
  prepared.value_locations.functions[0].move_bundles[0].moves.push_back(
      prepare::PreparedMoveResolution{
          .from_value_id = 4,
          .to_value_id = 2,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_contiguous_width = 1,
          .block_index = 0,
          .instruction_index = 0,
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .reason = "consumer_register_to_stack",
      });
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
      0) {
    return fail("metadata-mismatched multi-source bundle should stay on generic shape diagnostic");
  }

  return 0;
}

int builds_prepared_stack_to_stack_before_instruction_move_bundle_object() {
  const auto prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected prepared stack-to-stack move-bundle RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "main");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared stack-to-stack move object to publish text/main");
  }
  if (text->bytes.size() < 12 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared stack-to-stack move object text layout");
  }
  if (!contains_adjacent_u32_pair(text->bytes, 0x00412303, 0x00612423)) {
    return fail("expected lw t1, 4(sp); sw t1, 8(sp) from prepared stack-to-stack move");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared stack-to-stack move object to need no relocations");
  }
  return 0;
}

int builds_prepared_mixed_stack_destination_move_bundle_object() {
  auto prepared = make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  const auto function_name = prepared.names.function_names.find("main");
  const auto loaded_name = prepared.names.value_names.find("%loaded");

  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{12},
      .function_name = function_name,
      .offset_bytes = 12,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  auto& locations = prepared.value_locations.functions.front();
  locations.value_homes.push_back(rv64_stack_slot_home(
      3, function_name, loaded_name, prepare::PreparedFrameSlotId{12}, 12));
  locations.value_homes.push_back(
      rv64_gpr_home(4, function_name, loaded_name, "t0", 5));
  auto& bundle = locations.move_bundles.front();
  bundle.moves.insert(bundle.moves.begin(),
                      prepare::PreparedMoveResolution{
                          .from_value_id = 4,
                          .to_value_id = 3,
                          .destination_kind =
                              prepare::PreparedMoveDestinationKind::Value,
                          .destination_storage_kind =
                              prepare::PreparedMoveStorageKind::StackSlot,
                          .destination_contiguous_width = 1,
                          .block_index = 0,
                          .instruction_index = 2,
                          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                          .reason = "consumer_register_to_stack",
                      });

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected mixed stack-destination move-bundle RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "main");
  if (text == nullptr || function == nullptr) {
    return fail("expected mixed stack-destination move object to publish text/main");
  }
  if (!contains_adjacent_u32_pair(text->bytes, 0x00512623, 0x00412303) ||
      !contains_adjacent_u32_pair(text->bytes, 0x00412303, 0x00612423)) {
    return fail("expected mixed register-to-stack and stack-to-stack moves to emit");
  }
  if (!module->relocations.empty()) {
    return fail("expected mixed stack-destination move object to need no relocations");
  }
  return 0;
}

int builds_explicit_prepared_stack_widening_authority_move_bundle_object() {
  const auto expect_materialized_width = [&](bir::TypeKind source_type,
                                             std::size_t source_size_bytes,
                                             std::uint32_t expected_load_word) {
    const auto prepared =
        make_prepared_before_instruction_stack_widening_move_bundle_module(
            source_type, source_size_bytes);
    const auto& bundle =
        prepared.value_locations.functions[0].move_bundles[0];
    const auto& move = bundle.moves[0];
    if (bundle.authority_kind !=
            prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion ||
        move.authority_kind !=
            prepare::PreparedMoveAuthorityKind::StackSlotWideningConversion ||
        move.reason != "consumer_stack_to_stack") {
      return fail("prepared stack widening fixture did not publish explicit stack-slot widening authority");
    }

    const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
    if (!module.has_value()) {
      const auto result =
          rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
      return fail("expected explicit prepared stack widening authority to build, got `" +
                  result.diagnostic + "`");
    }
    const auto* text = object::find_section(*module, ".text");
    const auto* function = object::find_symbol(*module, "main");
    if (text == nullptr || function == nullptr) {
      return fail("expected prepared stack widening move object to publish text/main");
    }
    if (!contains_adjacent_u32_pair(text->bytes,
                                    expected_load_word,
                                    0x00612423)) {
      return fail("expected prepared stack widening move to load narrow source and store i32 destination");
    }
    if (!module->relocations.empty()) {
      return fail("expected prepared stack widening move object to need no relocations");
    }
    return 0;
  };

  if (const int status =
          expect_materialized_width(bir::TypeKind::I8, 1, 0x00410303);
      status != 0) {
    return status;
  }
  return expect_materialized_width(bir::TypeKind::I16, 2, 0x00411303);
}

int rejects_non_integer_prepared_stack_widening_authority_shapes() {
  auto prepared =
      make_prepared_before_instruction_stack_widening_move_bundle_module(
          bir::TypeKind::I8, 1);
  auto* cast = std::get_if<bir::CastInst>(
      &prepared.module.functions[0].blocks[0].insts[2]);
  if (cast == nullptr) {
    return fail("expected stack widening fixture to publish cast result");
  }
  cast->result = bir::Value::named(bir::TypeKind::F32, "%sum");

  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("non-integer explicit stack widening authority should reject");
  }
  if (result.prepared_consumer_category.has_value() ||
      result.diagnostic.find(
          "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves") !=
          0 ||
      result.diagnostic.find("authority=stack_slot_widening_conversion") ==
          std::string::npos ||
      result.diagnostic.find("move[0].source_type=i8") == std::string::npos ||
      result.diagnostic.find("move[0].destination_type=float") ==
          std::string::npos ||
      result.diagnostic.find(
          "unsupported_prepared_move_bundle_classification") !=
          std::string::npos) {
    return fail("non-integer explicit stack widening authority should remain RV64 fail-closed, got `" +
                result.diagnostic + "`");
  }
  return 0;
}

int rejects_prepared_stack_to_stack_move_bundle_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";
  const auto route_move_before_first_instruction =
      [](prepare::PreparedBirModule& prepared) {
        auto& bundle = prepared.value_locations.functions[0].move_bundles[0];
        bundle.instruction_index = 0;
        bundle.moves[0].instruction_index = 0;
      };

  auto prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  prepared.value_locations.functions[0].value_homes[0].slot_id = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("expected missing source slot id to reject at move bundle");
  }

  prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  prepared.value_locations.functions[0].value_homes[0].size_bytes =
      std::size_t{2};
  prepared.stack_layout.frame_slots.back().size_bytes = 2;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("expected source size mismatch to reject at move bundle");
  }

  prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  auto* sum = std::get_if<bir::BinaryInst>(
      &prepared.module.functions[0].blocks[0].insts[2]);
  if (sum == nullptr) {
    return fail("expected stack-to-stack fixture to keep binary sum");
  }
  sum->result = bir::Value::named(bir::TypeKind::I64, "%sum");
  const auto conversion_adjacent =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (conversion_adjacent.ok() || conversion_adjacent.module.has_value()) {
    return fail("stack-source stack-destination conversion-adjacent move should reject");
  }
  if (conversion_adjacent.prepared_consumer_category.has_value() ||
      conversion_adjacent.diagnostic.find(
          "unsupported_prepared_move_bundle_classification: stack-source "
          "stack-destination conversion-adjacent move was classified as "
          "consumer_stack_to_stack") != 0 ||
      conversion_adjacent.diagnostic.find(
          "move[0].reason=consumer_stack_to_stack") == std::string::npos ||
      conversion_adjacent.diagnostic.find(
          "move[0].source_home_kind=stack_slot") == std::string::npos ||
      conversion_adjacent.diagnostic.find(
          "move[0].destination_home_kind=stack_slot") == std::string::npos ||
      conversion_adjacent.diagnostic.find("move[0].source_type=i32") ==
          std::string::npos ||
      conversion_adjacent.diagnostic.find("move[0].destination_type=i64") ==
          std::string::npos ||
      conversion_adjacent.diagnostic.find(
          "diagnostic_owner=prepared_move_bundle_classifier") ==
          std::string::npos ||
      conversion_adjacent.diagnostic.find(
          "fragment_status=producer_classification_rejected_stack_source_"
          "stack_destination_conversion_adjacent_move") == std::string::npos ||
      conversion_adjacent.diagnostic.find(
          "generic_move_bundle_materialization_failed") != std::string::npos) {
    return fail("conversion-adjacent stack-to-stack mismatch should produce classifier diagnostic, got `" +
                conversion_adjacent.diagnostic + "`");
  }
  const auto conversion_adjacent_image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(
          prepared);
  if (conversion_adjacent_image.ok() ||
      conversion_adjacent_image.image.has_value() ||
      conversion_adjacent_image.prepared_consumer_category.has_value() ||
      conversion_adjacent_image.diagnostic != conversion_adjacent.diagnostic) {
    return fail("ELF writer should preserve classifier-owned conversion-adjacent stack-to-stack diagnostic");
  }

  prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  const auto function_name = prepared.names.function_names.find("main");
  const auto t1_name = prepared.names.value_names.intern("%scratch.t1");
  const auto t2_name = prepared.names.value_names.intern("%scratch.t2");
  const auto t3_name = prepared.names.value_names.intern("%scratch.t3");
  auto& homes = prepared.value_locations.functions[0].value_homes;
  homes.push_back(rv64_gpr_home(4, function_name, t1_name, "t1", 6));
  homes.push_back(rv64_gpr_home(5, function_name, t2_name, "t2", 7));
  homes.push_back(rv64_gpr_home(6, function_name, t3_name, "t3", 28));
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("expected unavailable scratch GPR to reject at move bundle");
  }

  prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  prepared.value_locations.functions[0].value_homes[0].target_register_identity =
      prepare::PreparedTargetRegisterIdentity{
          .target_arch = c4c::TargetArch::Riscv64,
          .bank = prepare::PreparedRegisterBank::None,
          .register_class = prepare::PreparedRegisterClass::General,
          .physical_index = 0,
      };
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("expected incoherent source storage identity to reject at move bundle");
  }

  prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  const auto& bank_none_source_home =
      prepared.value_locations.functions[0].value_homes[0];
  prepared.storage_plans.functions = {prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = bank_none_source_home.value_id,
          .value_name = bank_none_source_home.value_name,
          .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
          .bank = prepare::PreparedRegisterBank::None,
          .contiguous_width = 1,
          .slot_id = bank_none_source_home.slot_id,
          .stack_offset_bytes = bank_none_source_home.offset_bytes,
          .spill_slot_placement = prepare::PreparedSpillSlotPlacement{
              .slot_id = *bank_none_source_home.slot_id,
              .offset_bytes = *bank_none_source_home.offset_bytes,
          },
      }},
  }};
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("expected storage-plan frame_slot bank=none source to reject at move bundle");
  }

  prepared =
      make_prepared_before_instruction_stack_to_stack_move_bundle_module();
  route_move_before_first_instruction(prepared);
  prepared.value_locations.functions[0].move_bundles[0].moves[0].reason =
      "consumer_register_to_stack";
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return fail("expected reason/source-home mismatch to reject at move bundle");
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

int builds_prepared_out_of_ssa_phi_join_register_move_object() {
  const auto prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected prepared out-of-SSA phi-join register move RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "phi_join");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared out-of-SSA phi-join object to publish text/function");
  }
  if (text->bytes.size() < 8 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared out-of-SSA phi-join object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028493) {
    return fail("expected mv s1, t0 for prepared out-of-SSA phi-join register move");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared out-of-SSA phi-join register object to need no relocations");
  }
  return 0;
}

int builds_prepared_out_of_ssa_phi_join_immediate_materialization_object() {
  const auto prepared =
      make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected prepared out-of-SSA phi-join immediate materialization RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "phi_join");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared out-of-SSA phi-join immediate object to publish text/function");
  }
  if (text->bytes.size() < 8 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared out-of-SSA phi-join immediate object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x4d200493) {
    return fail("expected li s1, 1234 for prepared out-of-SSA phi-join immediate materialization");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared out-of-SSA phi-join immediate object to need no relocations");
  }
  return 0;
}

int builds_prepared_out_of_ssa_phi_join_i64_zero_materialization_object() {
  const auto prepared =
      make_prepared_out_of_ssa_phi_join_i64_zero_materialization_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected prepared out-of-SSA phi-join i64 zero materialization RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "phi_join");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared out-of-SSA phi-join i64 zero object to publish text/function");
  }
  if (text->bytes.size() < 8 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared out-of-SSA phi-join i64 zero object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00000493) {
    return fail("expected li s1, 0 for prepared out-of-SSA phi-join i64 zero materialization");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared out-of-SSA phi-join i64 zero object to need no relocations");
  }
  return 0;
}

int builds_prepared_out_of_ssa_edge_preservation_register_move_object() {
  const auto prepared =
      make_prepared_out_of_ssa_edge_preservation_register_move_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected prepared out-of-SSA edge-preservation register move RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "phi_join");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared edge-preservation object to publish text/function");
  }
  if (text->bytes.size() < 12 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared edge-preservation object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00030913 ||
      read_u32(text->bytes, 4) != 0x00028493) {
    return fail("expected prepared move order mv s2, t1 then mv s1, t0");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared edge-preservation register object to need no relocations");
  }
  return 0;
}

int builds_prepared_out_of_ssa_edge_preservation_stack_move_object() {
  const auto prepared =
      make_prepared_out_of_ssa_edge_preservation_stack_move_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected prepared out-of-SSA edge-preservation stack move RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "phi_join");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared edge-preservation stack object to publish text/function");
  }
  if (text->bytes.size() < 12 || text->size_bytes != text->bytes.size() ||
      function->value != 0 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared edge-preservation stack object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x00612423 ||
      read_u32(text->bytes, 8) != 0x00028493) {
    return fail("expected stack frame, prepared store sw t1, 8(sp), then mv s1, t0");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared edge-preservation stack object to need no relocations");
  }
  return 0;
}

int rejects_prepared_out_of_ssa_edge_preservation_stack_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared =
      make_prepared_out_of_ssa_edge_preservation_stack_move_module();
  prepared.stack_layout.frame_slots.clear();
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_edge_preservation_stack_move_module();
  prepared.value_locations.functions[0].value_homes[2] =
      rv64_stack_slot_home(3,
                           prepared.names.function_names.find("phi_join"),
                           prepared.names.value_names.find("%keep.src"),
                           prepare::PreparedFrameSlotId{22},
                           12);
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{22},
      .function_name = prepared.names.function_names.find("phi_join"),
      .offset_bytes = 12,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_edge_preservation_stack_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_parallel_copy_step_index = std::size_t{0};
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_edge_preservation_stack_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_out_of_ssa_edge_preservation_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared =
      make_prepared_out_of_ssa_edge_preservation_register_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_parallel_copy_step_index = std::size_t{0};
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_edge_preservation_register_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_edge_preservation_register_move_module();
  prepared.value_locations.functions[0].move_bundles[0].moves[0].reason =
      "edge_consumer_preservation_register_to_stack";
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_edge_preservation_register_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_parallel_copy_predecessor_label =
      prepared.names.block_labels.intern("wrong.pred");
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_out_of_ssa_phi_join_register_move_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  prepared.value_locations.functions[0].move_bundles[0].moves[0].reason =
      "edge_consumer_preservation_register_to_register";
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_parallel_copy_step_index = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  prepared.control_flow.functions[0].parallel_copy_bundles[0].steps[0].kind =
      prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  prepared.control_flow.functions[0].parallel_copy_bundles[0]
      .steps[0]
      .uses_cycle_temp_source = true;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_register_move_module();
  prepared.control_flow.functions[0].parallel_copy_bundles[0].execution_site =
      prepare::PreparedParallelCopyExecutionSite::SuccessorEntry;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_out_of_ssa_phi_join_immediate_materialization_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared =
      make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_immediate_i32 = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.value_locations.functions[0].move_bundles[0].moves[0].reason =
      "phi_join_register_to_register";
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.control_flow.functions[0]
      .parallel_copy_bundles[0]
      .moves[0]
      .source_value = bir::Value::named(bir::TypeKind::I32, "%src");
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_parallel_copy_successor_label =
      prepared.names.block_labels.intern("wrong.join");
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.value_locations.functions[0]
      .move_bundles[0]
      .moves[0]
      .source_parallel_copy_step_index = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.control_flow.functions[0].parallel_copy_bundles[0]
      .steps[0]
      .uses_cycle_temp_source = true;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_out_of_ssa_phi_join_immediate_materialization_module();
  prepared.value_locations.functions[0].move_bundles[0].moves[0].op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int reports_prepared_move_bundle_coordinate_diagnostic() {
  auto prepared = make_prepared_stack_slot_to_gpr_move_bundle_module();
  auto& move_bundle = prepared.value_locations.functions[0].move_bundles[0];
  auto& move = move_bundle.moves[0];
  move.destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  move.destination_stack_offset_bytes = 8;
  move.reason = "consumer_register_to_stack";

  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("expected prepared move-bundle coordinate diagnostic rejection");
  }
  const std::string& diagnostic = result.diagnostic;
  const std::vector<std::string> required_fragments = {
      kGenericPreparedMoveBundleDiagnostic,
      "event_kind=pre_terminator_copies",
      "function=stack_move",
      "block_index=0",
      "block_label=entry",
      "instruction_index=0",
      "phase=before_return",
      "bundle_block_index=0",
      "bundle_instruction_index=0",
      "authority=none",
      "move_count=1",
      "parallel_copy=no",
      "select_edge_suppression_authorized=no",
      "cast_dependency_stack_publication_authorized=no",
      "move[0].from_value_id=1",
      "move[0].to_value_id=2",
      "move[0].destination_kind=value",
      "move[0].destination_storage=stack_slot",
      "move[0].op_kind=move",
      "move[0].reason=consumer_register_to_stack",
      "fragment_status=generic_move_bundle_materialization_failed",
  };
  for (const auto& fragment : required_fragments) {
    if (diagnostic.find(fragment) == std::string::npos) {
      return fail("expected move-bundle coordinate diagnostic fragment `" +
                  fragment + "`, got `" + diagnostic + "`");
    }
  }

  const auto image =
      rv64::write_rv64_prepared_relocatable_elf_object_with_diagnostics(prepared);
  if (image.ok() || image.image.has_value() ||
      image.diagnostic.find("event_kind=pre_terminator_copies") ==
          std::string::npos ||
      image.diagnostic.find("move[0].destination_storage=stack_slot") ==
          std::string::npos) {
    return fail("expected ELF writer to preserve move-bundle coordinate diagnostic");
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

int reports_generic_fallback_context_for_prepared_traversal_instruction() {
  auto prepared = make_prepared_scalar_ashr_module(bir::TypeKind::I32, true);
  auto* shift = std::get_if<bir::BinaryInst>(
      &prepared.module.functions.front().blocks.front().insts.front());
  if (shift == nullptr) {
    return fail("expected prepared scalar ashr fixture to contain a binary instruction");
  }
  shift->rhs = bir::Value::immediate_i32(32);
  return expect_prepared_rejection_diagnostic_contains(
      prepared,
      {
          "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering",
          "function=main",
          "block=entry",
          "block_index=0",
          "instruction_index=0",
          "instruction_kind=BinaryInst",
          "owner=i32 %result",
      });
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
      {bir::BinaryOpcode::UDiv, bir::TypeKind::I32, 0x03de593b, "divuw"},
      {bir::BinaryOpcode::UDiv, bir::TypeKind::I64, 0x03de5933, "divu"},
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

int builds_prepared_scalar_f64_binary_object() {
  auto prepared =
      make_prepared_scalar_fpr_binary_module(bir::BinaryOpcode::SDiv,
                                             bir::TypeKind::F64);
  auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared scalar fdiv.d RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* function = object::find_symbol(module, "fp_binary");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared scalar fdiv.d object to publish text/function");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      function->value != 0 || function->size_bytes != 8 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared scalar fdiv.d object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x1ab50053 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fdiv.d ft0, fa0, fa1 followed by ret");
  }
  if (!module.relocations.empty()) {
    return fail("expected scalar fdiv.d object to need no relocations");
  }

  prepared = make_prepared_scalar_f64_immediate_binary_module(true);
  result = rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared scalar fdiv.d lhs-immediate RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  text = object::find_section(*result.module, ".text");
  function = object::find_symbol(*result.module, "fp_binary");
  if (text == nullptr || function == nullptr || text->bytes.size() < 16 ||
      function->size_bytes != text->bytes.size()) {
    return fail("expected prepared scalar fdiv.d lhs-immediate object text layout");
  }
  if (!contains_u32(text->bytes, 0xf20e02d3) ||
      !contains_u32(text->bytes, 0x1ab28053) ||
      read_u32(text->bytes, text->bytes.size() - 4) != 0x00008067) {
    return fail("expected lhs F64 immediate materialization before fdiv.d");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected scalar fdiv.d lhs-immediate object to need no relocations");
  }

  prepared = make_prepared_scalar_f64_immediate_binary_module(false);
  result = rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared scalar fdiv.d rhs-immediate RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  text = object::find_section(*result.module, ".text");
  function = object::find_symbol(*result.module, "fp_binary");
  if (text == nullptr || function == nullptr || text->bytes.size() < 16 ||
      function->size_bytes != text->bytes.size()) {
    return fail("expected prepared scalar fdiv.d rhs-immediate object text layout");
  }
  if (!contains_u32(text->bytes, 0xf20e8353) ||
      !contains_u32(text->bytes, 0x1a650053) ||
      read_u32(text->bytes, text->bytes.size() - 4) != 0x00008067) {
    return fail("expected rhs F64 immediate materialization before fdiv.d");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected scalar fdiv.d rhs-immediate object to need no relocations");
  }
  return 0;
}

int builds_prepared_scalar_f32_binary_object() {
  const auto prepared =
      make_prepared_scalar_fpr_binary_module(bir::BinaryOpcode::Mul,
                                             bir::TypeKind::F32);
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared scalar fmul.s RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* function = object::find_symbol(module, "fp_binary");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared scalar fmul.s object to publish text/function");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      function->value != 0 || function->size_bytes != 8 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared scalar fmul.s object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x10b50053 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected fmul.s ft0, fa0, fa1 followed by ret");
  }
  if (!module.relocations.empty()) {
    return fail("expected scalar fmul.s object to need no relocations");
  }
  return 0;
}

int rejects_prepared_scalar_fp_binary_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";

  auto prepared =
      make_prepared_scalar_fpr_binary_module(bir::BinaryOpcode::SDiv,
                                             bir::TypeKind::F128);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_fpr_binary_module(bir::BinaryOpcode::SRem,
                                                    bir::TypeKind::F64);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_scalar_fpr_binary_module(bir::BinaryOpcode::SRem,
                                                    bir::TypeKind::F32);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_scalar_division_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";

  auto prepared =
      make_prepared_scalar_binary_module(bir::BinaryOpcode::UDiv,
                                         bir::TypeKind::I32);
  prepared.value_locations.functions[0].value_homes.erase(
      prepared.value_locations.functions[0].value_homes.begin() + 1);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_scalar_binary_module(bir::BinaryOpcode::UDiv,
                                         bir::TypeKind::I32);
  prepared.value_locations.functions[0].value_homes.pop_back();
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_scalar_binary_module(bir::BinaryOpcode::UDiv,
                                         bir::TypeKind::Ptr);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_scalar_remainder_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";

  auto prepared =
      make_prepared_scalar_binary_module(bir::BinaryOpcode::SRem,
                                         bir::TypeKind::I32);
  prepared.value_locations.functions[0].value_homes.erase(
      prepared.value_locations.functions[0].value_homes.begin());
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_scalar_binary_module(bir::BinaryOpcode::URem,
                                         bir::TypeKind::I64);
  prepared.value_locations.functions[0].value_homes.pop_back();
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_scalar_binary_module(bir::BinaryOpcode::SRem,
                                         bir::TypeKind::Ptr);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_pointer_arithmetic_with_precise_diagnostic() {
  return expect_prepared_rejection_diagnostic_contains(
      make_prepared_loaded_base_pointer_arithmetic_module(),
      {
          "unsupported_pointer_arithmetic: RV64 object route requires prepared pointer arithmetic lowering for loaded pointer base plus scaled integer byte offset",
          "function=pointer_arithmetic",
          "block=entry",
          "instruction_index=2",
          "instruction_kind=BinaryInst",
          "owner=ptr %result.ptr",
      });
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

int builds_prepared_f32_scalar_compare_publication_object() {
  const auto prepared =
      make_prepared_fpr_compare_publication_module(bir::TypeKind::F32, false);
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared F32 scalar compare result publication to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* function =
      object::find_symbol(*result.module, "fpr_compare_publication");
  if (text == nullptr || function == nullptr || text->bytes.empty() ||
      function->size_bytes != text->bytes.size()) {
    return fail("expected F32 scalar compare publication object to publish text/function");
  }
  if (!contains_u32_sequence(text->bytes,
                             {
                                 0xa0b524d3,  // feq.s s1, fa0, fa1
                                 0x0014c493,  // xori s1, s1, 1
                             })) {
    return fail("expected F32 scalar compare publication to emit feq.s and xori into prepared GPR home");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected F32 scalar compare publication object to need no relocations");
  }
  return 0;
}

int builds_prepared_f32_scalar_compare_zero_publication_object() {
  const auto prepared =
      make_prepared_fpr_compare_publication_module(bir::TypeKind::F32,
                                                  false,
                                                  true);
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared F32 scalar compare zero publication to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* function =
      object::find_symbol(*result.module, "fpr_compare_zero_publication");
  if (text == nullptr || function == nullptr || text->bytes.empty() ||
      function->size_bytes != text->bytes.size()) {
    return fail("expected F32 scalar compare zero publication object to publish text/function");
  }
  if (!contains_u32_sequence(text->bytes,
                             {
                                 0x00000313,  // addi t1, zero, 0
                                 0xf0030fd3,  // fmv.w.x ft11, t1
                                 0xa1f524d3,  // feq.s s1, fa0, ft11
                                 0x0014c493,  // xori s1, s1, 1
                             })) {
    return fail("expected F32 scalar compare zero publication to materialize zero and emit feq.s into prepared GPR home");
  }
  if (!result.module->relocations.empty()) {
    return fail("expected F32 scalar compare zero publication object to need no relocations");
  }
  return 0;
}

int builds_prepared_f64_scalar_compare_select_consumer_publication_object() {
  const auto prepared =
      make_prepared_fpr_compare_publication_module(bir::TypeKind::F64, true);
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared F64 scalar compare publication feeding select to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*result.module, ".text");
  const auto* function =
      object::find_symbol(*result.module, "fpr_compare_select_publication");
  const auto* select_true =
      object::find_symbol(*result.module,
                          ".Lfpr_compare_select_publication_entry_select_1_true");
  const auto* select_end =
      object::find_symbol(*result.module,
                          ".Lfpr_compare_select_publication_entry_select_1_end");
  if (text == nullptr || function == nullptr || select_true == nullptr ||
      select_end == nullptr || text->bytes.empty() ||
      function->size_bytes != text->bytes.size()) {
    return fail("expected F64 compare select-consumer object to publish text/function/select labels");
  }
  if (!contains_u32_sequence(text->bytes,
                             {
                                 0xa2b524d3,  // feq.d s1, fa0, fa1
                                 0x0014c493,  // xori s1, s1, 1
                             })) {
    return fail("expected F64 scalar compare publication to emit feq.d and xori into prepared GPR home");
  }
  if (!contains_u32(text->bytes, 0x00300513) ||
      !contains_u32(text->bytes, 0x00700513)) {
    return fail("expected F64 compare select consumer to materialize false/true values");
  }
  if (result.module->relocations.size() != 2 ||
      result.module->relocations[0].section != text->id ||
      result.module->relocations[0].type != R_RISCV_BRANCH ||
      result.module->relocations[0].symbol != select_true->id ||
      result.module->relocations[0].addend != 0 ||
      result.module->relocations[1].section != text->id ||
      result.module->relocations[1].type != R_RISCV_JAL ||
      result.module->relocations[1].symbol != select_end->id ||
      result.module->relocations[1].addend != 0) {
    return fail("expected F64 compare select-consumer local branch/jump relocations");
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

int builds_prepared_small_integer_ordinary_select_materialization_objects() {
  for (const auto [type, expected_store, type_name] :
       {std::tuple{bir::TypeKind::I8, std::uint32_t{0x01e10023}, "i8"},
        std::tuple{bir::TypeKind::I16, std::uint32_t{0x01e11023}, "i16"}}) {
    const auto prepared = make_prepared_small_integer_ordinary_select_module(type);
    const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
    if (!module.has_value()) {
      return fail(std::string{"expected prepared ordinary "} + type_name +
                  " select RV64 object module to build");
    }
    const auto* text = object::find_section(*module, ".text");
    const auto* main_symbol = object::find_symbol(*module, "main");
    const auto* true_label =
        object::find_symbol(*module, ".Lmain_entry_select_0_true");
    const auto* end_label =
        object::find_symbol(*module, ".Lmain_entry_select_0_end");
    if (text == nullptr || main_symbol == nullptr || true_label == nullptr ||
        end_label == nullptr) {
      return fail(std::string{"expected prepared ordinary "} + type_name +
                  " select object to publish text/main/select labels");
    }
    if (text->bytes.size() != 44 || text->size_bytes != 44 ||
        main_symbol->value != 0 || main_symbol->size_bytes != 44 ||
        true_label->value != 24 || end_label->value != 28) {
      return fail(std::string{"expected prepared ordinary "} + type_name +
                  " select materialization object text layout");
    }
    if (read_u32(text->bytes, 0) != 0xff010113 ||
        read_u32(text->bytes, 4) != 0x00028e13 ||
        read_u32(text->bytes, 8) != 0x00100e93 ||
        read_u32(text->bytes, 12) != 0x01de1063 ||
        read_u32(text->bytes, 16) != 0x00030f13 ||
        read_u32(text->bytes, 20) != 0x0000006f ||
        read_u32(text->bytes, 24) != 0x00700f13 ||
        read_u32(text->bytes, 28) != expected_store ||
        read_u32(text->bytes, 32) != 0x00000513 ||
        read_u32(text->bytes, 36) != 0x01010113 ||
        read_u32(text->bytes, 40) != 0x00008067) {
      return fail(std::string{"expected prepared ordinary "} + type_name +
                  " select compare/branch/materialize/narrow-store sequence");
    }
    if (module->relocations.size() != 2 ||
        module->relocations[0].section != text->id ||
        module->relocations[0].offset != 12 ||
        module->relocations[0].type != R_RISCV_BRANCH ||
        module->relocations[0].symbol != true_label->id ||
        module->relocations[1].section != text->id ||
        module->relocations[1].offset != 20 ||
        module->relocations[1].type != R_RISCV_JAL ||
        module->relocations[1].symbol != end_label->id) {
      return fail(std::string{"expected prepared ordinary "} + type_name +
                  " select local branch/jump relocations");
    }
  }
  return 0;
}

int materializes_nested_i32_ordinary_select_without_intermediate_home_object() {
  const auto prepared = make_prepared_nested_i32_ordinary_select_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected nested i32 ordinary select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* skipped_true_label =
      object::find_symbol(*module, ".Lmain_entry_select_0_true");
  const auto* skipped_end_label =
      object::find_symbol(*module, ".Lmain_entry_select_0_end");
  const auto* final_true_label =
      object::find_symbol(*module, ".Lmain_entry_select_1_true");
  const auto* final_end_label =
      object::find_symbol(*module, ".Lmain_entry_select_1_end");
  if (text == nullptr || main_symbol == nullptr ||
      skipped_true_label == nullptr || skipped_end_label == nullptr ||
      final_true_label == nullptr || final_end_label == nullptr) {
    return fail("expected nested i32 ordinary select object to publish both select label pairs");
  }
  if (text->bytes.size() <= 44 || text->size_bytes <= 44 ||
      main_symbol->value != 0 || main_symbol->size_bytes != text->size_bytes) {
    return fail("expected nested i32 ordinary select object to materialize more than one select");
  }
  bool saw_branch_relocation = false;
  bool saw_jump_relocation = false;
  for (const auto& relocation : module->relocations) {
    saw_branch_relocation = saw_branch_relocation ||
                            relocation.type == R_RISCV_BRANCH;
    saw_jump_relocation = saw_jump_relocation || relocation.type == R_RISCV_JAL;
  }
  if (!saw_branch_relocation || !saw_jump_relocation) {
    return fail("expected nested i32 ordinary select object to use local select control flow");
  }
  return 0;
}

int materializes_tree_i32_ordinary_select_without_intermediate_home_object() {
  const auto prepared = make_prepared_tree_i32_ordinary_select_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    const auto result =
        rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
    return fail("expected tree i32 ordinary select RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* first_true_label =
      object::find_symbol(*module, ".Lmain_entry_select_0_true");
  const auto* branch_true_label =
      object::find_symbol(*module, ".Lmain_entry_select_2_true");
  const auto* final_true_label =
      object::find_symbol(*module, ".Lmain_entry_select_6_true");
  const auto* final_end_label =
      object::find_symbol(*module, ".Lmain_entry_select_6_end");
  if (text == nullptr || main_symbol == nullptr ||
      first_true_label == nullptr || branch_true_label == nullptr ||
      final_true_label == nullptr || final_end_label == nullptr) {
    return fail("expected tree i32 ordinary select object to publish nested select labels");
  }
  if (text->bytes.size() <= 100 || text->size_bytes <= 100 ||
      main_symbol->value != 0 || main_symbol->size_bytes != text->size_bytes) {
    return fail("expected tree i32 ordinary select object to materialize nested select tree");
  }
  std::size_t branch_relocations = 0;
  std::size_t jump_relocations = 0;
  for (const auto& relocation : module->relocations) {
    branch_relocations += relocation.type == R_RISCV_BRANCH ? 1U : 0U;
    jump_relocations += relocation.type == R_RISCV_JAL ? 1U : 0U;
  }
  if (branch_relocations < 6U || jump_relocations < 6U) {
    return fail("expected tree i32 ordinary select object to emit nested select control flow");
  }
  return 0;
}

int rejects_reused_nested_i32_ordinary_select_without_intermediate_home_object() {
  const auto prepared =
      make_prepared_reused_nested_i32_ordinary_select_module();
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected reused no-home nested i32 ordinary select to remain fail-closed");
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

int materializes_published_prepared_join_transfer_select_stack_carrier_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_published_copies_stack_result_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected stack-home published join-transfer select RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* true_copy_label = object::find_symbol(*module, ".Lmain_pred_true");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  const auto* select_true_label = object::find_symbol(*module, ".Lmain_join_select_0_true");
  const auto* select_end_label = object::find_symbol(*module, ".Lmain_join_select_0_end");
  if (text == nullptr || main_symbol == nullptr || true_copy_label == nullptr ||
      false_copy_label == nullptr || join_label == nullptr ||
      select_true_label == nullptr || select_end_label == nullptr) {
    return fail("expected stack-home join object to publish block and select labels");
  }
  if (text->bytes.size() != 60 || text->size_bytes != 60 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 60 ||
      true_copy_label->value != 8 || false_copy_label->value != 12 ||
      join_label->value != 20 || select_true_label->value != 40 ||
      select_end_label->value != 44) {
    return fail("expected stack-home join select object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xff010113 ||
      read_u32(text->bytes, 4) != 0x0000006f ||
      read_u32(text->bytes, 8) != 0x0000006f ||
      read_u32(text->bytes, 12) != 0x00612023 ||
      read_u32(text->bytes, 16) != 0x0000006f ||
      read_u32(text->bytes, 20) != 0x00048e13 ||
      read_u32(text->bytes, 24) != 0x00100e93 ||
      read_u32(text->bytes, 28) != 0x01de1063 ||
      read_u32(text->bytes, 32) != 0x00030f13 ||
      read_u32(text->bytes, 36) != 0x0000006f ||
      read_u32(text->bytes, 40) != 0x00100f13 ||
      read_u32(text->bytes, 44) != 0x01e12023 ||
      read_u32(text->bytes, 48) != 0x00012503 ||
      read_u32(text->bytes, 52) != 0x01010113 ||
      read_u32(text->bytes, 56) != 0x00008067) {
    return fail("expected stack-home join carrier to materialize select at join and reload return");
  }
  bool saw_select_branch = false;
  for (const auto& relocation : module->relocations) {
    const auto symbol_it =
        std::find_if(module->symbols.begin(),
                     module->symbols.end(),
                     [&](const object::SymbolRecord& symbol) {
                       return symbol.id == relocation.symbol;
                     });
    if (symbol_it == module->symbols.end()) {
      continue;
    }
    saw_select_branch =
        saw_select_branch || symbol_it->name.find("_select_") != std::string::npos;
  }
  if (!saw_select_branch) {
    return fail("expected stack-home join carrier to use local select relocations");
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

int materializes_published_prepared_join_transfer_select_cast_dependency_source_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  const auto& module = result.module;
  if (!module.has_value()) {
    return fail(
        "expected cast-dependency edge-compare source prepared join-transfer select RV64 object module to build: " +
        result.diagnostic);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  if (text == nullptr || main_symbol == nullptr || false_copy_label == nullptr ||
      join_label == nullptr) {
    return fail("expected cast-dependency edge-compare join object to publish text/main/block labels");
  }
  if (text->bytes.size() < 40 || main_symbol->value != 0 ||
      join_label->value <= false_copy_label->value) {
    return fail("expected cast-dependency edge-compare join object text layout");
  }
  bool saw_cast_source_materialization = false;
  bool saw_unsigned_compare = false;
  bool saw_compare_inversion = false;
  for (std::size_t offset = 0; offset + 4 <= text->bytes.size(); offset += 4) {
    const auto instruction = read_u32(text->bytes, offset);
    const auto opcode = instruction & 0x7fU;
    const auto rd = (instruction >> 7) & 0x1fU;
    const auto funct3 = (instruction >> 12) & 0x7U;
    const auto rs1 = (instruction >> 15) & 0x1fU;
    const auto rs2 = (instruction >> 20) & 0x1fU;
    if (rd == 29 && opcode != 0x03U) {
      saw_cast_source_materialization = true;
    }
    if (opcode == 0x33U && rd == 10 && funct3 == 3 && rs1 == 29 &&
        rs2 == 28) {
      saw_unsigned_compare = true;
    }
    if (opcode == 0x13U && rd == 10 && funct3 == 4 && rs1 == 10 &&
        ((instruction >> 20) & 0xfffU) == 1) {
      saw_compare_inversion = true;
    }
  }
  if (!saw_cast_source_materialization || !saw_unsigned_compare ||
      !saw_compare_inversion) {
    return fail("expected cast dependency to materialize RHS then emit ULE compare into edge result");
  }
  return 0;
}

int materializes_carrier_authorized_prepared_join_transfer_select_ule_source_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ule_source_module();
  const auto carrier_aliases =
      prepare::collect_prepared_select_carrier_alias_authorities(prepared);
  if (carrier_aliases.records.empty()) {
    return fail("expected carrier-alias ULE fixture to publish alias authority");
  }
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  const auto& module = result.module;
  if (!module.has_value()) {
    return fail(
        "expected carrier-authorized ULE source prepared join-transfer select RV64 object module to build: " +
        result.diagnostic);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  const auto* alias0_label =
      object::find_symbol(*module, ".Lmain_join_select_1_true");
  if (text == nullptr || false_copy_label == nullptr || join_label == nullptr ||
      join_label->value <= false_copy_label->value) {
    return fail("expected carrier-authorized ULE object labels");
  }
  if (alias0_label != nullptr) {
    return fail("expected authorized carrier aliases to avoid raw select materialization");
  }
  bool saw_unsigned_compare = false;
  bool saw_compare_inversion = false;
  for (std::size_t offset = false_copy_label->value;
       offset + 4 <= text->bytes.size() && offset < join_label->value;
       offset += 4) {
    const auto instruction = read_u32(text->bytes, offset);
    const auto opcode = instruction & 0x7fU;
    const auto rd = (instruction >> 7) & 0x1fU;
    const auto funct3 = (instruction >> 12) & 0x7U;
    const auto rs1 = (instruction >> 15) & 0x1fU;
    const auto rs2 = (instruction >> 20) & 0x1fU;
    if (opcode == 0x33U && rd == 10 && funct3 == 3 &&
        ((rs1 == 29 && rs2 == 28) || (rs1 == 18 && rs2 == 9))) {
      saw_unsigned_compare = true;
    }
    if (opcode == 0x13U && rd == 10 && funct3 == 4 && rs1 == 10 &&
        ((instruction >> 20) & 0xfffU) == 1) {
      saw_compare_inversion = true;
    }
  }
  if (!saw_unsigned_compare || !saw_compare_inversion) {
    return fail("expected carrier-authorized predecessor edge to rematerialize ULE into select result");
  }
  return 0;
}

int materializes_carrier_alias_prepared_join_transfer_select_ne_source_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ne_source_module();
  const auto carrier_aliases =
      prepare::collect_prepared_select_carrier_alias_authorities(prepared);
  if (carrier_aliases.records.empty()) {
    return fail("expected NE carrier-alias fixture to publish alias authority");
  }
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  const auto& module = result.module;
  if (!module.has_value()) {
    return fail(
        "expected carrier-authorized NE source prepared join-transfer select RV64 object module to build: " +
        result.diagnostic);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  const auto* alias0_label =
      object::find_symbol(*module, ".Lmain_join_select_1_true");
  if (text == nullptr || false_copy_label == nullptr || join_label == nullptr ||
      join_label->value <= false_copy_label->value) {
    return fail("expected carrier-authorized NE object labels");
  }
  if (alias0_label != nullptr) {
    return fail("expected authorized carrier aliases to avoid raw select materialization");
  }

  bool saw_raw_operand_copy = false;
  bool saw_operand_to_scratch = false;
  bool saw_one_to_scratch = false;
  bool saw_compare_against_one = false;
  bool saw_booleanize_compare = false;
  for (std::size_t offset = false_copy_label->value;
       offset + 4 <= text->bytes.size() && offset < join_label->value;
       offset += 4) {
    const auto instruction = read_u32(text->bytes, offset);
    const auto opcode = instruction & 0x7fU;
    const auto rd = (instruction >> 7) & 0x1fU;
    const auto funct3 = (instruction >> 12) & 0x7U;
    const auto rs1 = (instruction >> 15) & 0x1fU;
    const auto rs2 = (instruction >> 20) & 0x1fU;
    const auto imm = (instruction >> 20) & 0xfffU;

    if (opcode == 0x13U && rd == 10 && funct3 == 0 && rs1 == 11 && imm == 0) {
      saw_raw_operand_copy = true;
    }
    if (opcode == 0x13U && rd == 28 && funct3 == 0 && rs1 == 11 && imm == 0) {
      saw_operand_to_scratch = true;
    }
    if (opcode == 0x13U && rd == 29 && funct3 == 0 && rs1 == 0 && imm == 1) {
      saw_one_to_scratch = true;
    }
    if ((opcode == 0x13U && rd == 10 && funct3 == 4 && rs1 == 11 &&
         imm == 1) ||
        (opcode == 0x33U && rd == 10 && funct3 == 4 &&
         ((rs1 == 11 && rs2 != 11) ||
          (rs2 == 11 && rs1 != 11) ||
          (rs1 == 28 && rs2 == 29)))) {
      saw_compare_against_one = true;
    }
    if (opcode == 0x33U && rd == 10 && funct3 == 3 && rs1 == 0 &&
        rs2 == 10) {
      saw_booleanize_compare = true;
    }
  }
  if (saw_raw_operand_copy) {
    return fail("expected carrier-alias NE edge to publish compare result, not raw operand register");
  }
  if ((!saw_compare_against_one &&
       !(saw_operand_to_scratch && saw_one_to_scratch)) ||
      !saw_booleanize_compare) {
    return fail("expected carrier-alias NE edge to materialize b != 1 into select result");
  }
  return 0;
}

int materializes_predecessor_compare_publication_select_carrier_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_predecessor_compare_publication_module();
  const auto carrier_aliases =
      prepare::collect_prepared_select_carrier_alias_authorities(prepared);
  if (carrier_aliases.records.empty()) {
    return fail(
        "expected predecessor compare publication fixture to publish carrier-alias authority");
  }
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  const auto& module = result.module;
  if (!module.has_value()) {
    return fail(
        "expected predecessor compare publication to selected join carrier to build: " +
        result.diagnostic);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* false_copy_label =
      object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  if (text == nullptr || false_copy_label == nullptr || join_label == nullptr ||
      join_label->value <= false_copy_label->value) {
    return fail("expected predecessor compare publication object labels");
  }

  bool saw_raw_operand_copy = false;
  bool saw_operand_to_scratch = false;
  bool saw_one_to_scratch = false;
  bool saw_compare_against_one = false;
  bool saw_booleanize_compare = false;
  for (std::size_t offset = false_copy_label->value;
       offset + 4 <= text->bytes.size() && offset < join_label->value;
       offset += 4) {
    const auto instruction = read_u32(text->bytes, offset);
    const auto opcode = instruction & 0x7fU;
    const auto rd = (instruction >> 7) & 0x1fU;
    const auto funct3 = (instruction >> 12) & 0x7U;
    const auto rs1 = (instruction >> 15) & 0x1fU;
    const auto rs2 = (instruction >> 20) & 0x1fU;
    const auto imm = (instruction >> 20) & 0xfffU;

    if (opcode == 0x13U && rd == 10 && funct3 == 0 && rs1 == 11 && imm == 0) {
      saw_raw_operand_copy = true;
    }
    if (opcode == 0x13U && rd == 28 && funct3 == 0 && rs1 == 11 && imm == 0) {
      saw_operand_to_scratch = true;
    }
    if (opcode == 0x13U && rd == 29 && funct3 == 0 && rs1 == 0 && imm == 1) {
      saw_one_to_scratch = true;
    }
    if ((opcode == 0x13U && rd == 10 && funct3 == 4 && rs1 == 11 &&
         imm == 1) ||
        (opcode == 0x33U && rd == 10 && funct3 == 4 &&
         ((rs1 == 11 && rs2 != 11) ||
          (rs2 == 11 && rs1 != 11) ||
          (rs1 == 28 && rs2 == 29)))) {
      saw_compare_against_one = true;
    }
    if (opcode == 0x33U && rd == 10 && funct3 == 3 && rs1 == 0 &&
        rs2 == 10) {
      saw_booleanize_compare = true;
    }
  }
  if (saw_raw_operand_copy) {
    return fail(
        "expected predecessor edge to publish compare result, not raw operand register");
  }
  if ((!saw_compare_against_one &&
       !(saw_operand_to_scratch && saw_one_to_scratch)) ||
      !saw_booleanize_compare) {
    return fail("expected predecessor edge to materialize compare into selected join carrier");
  }
  return 0;
}

int materializes_only_carrier_use_prepared_join_transfer_select_ne_source_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_only_carrier_use_ne_source_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  const auto& module = result.module;
  if (!module.has_value()) {
    return fail(
        "expected only-carrier-use NE source prepared join-transfer select RV64 object module to build: " +
        result.diagnostic);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  if (text == nullptr || false_copy_label == nullptr || join_label == nullptr ||
      join_label->value <= false_copy_label->value) {
    return fail("expected only-carrier-use NE object labels");
  }

  bool saw_raw_operand_copy = false;
  bool saw_operand_to_scratch = false;
  bool saw_one_to_scratch = false;
  bool saw_compare_against_one = false;
  bool saw_booleanize_compare = false;
  for (std::size_t offset = false_copy_label->value;
       offset + 4 <= text->bytes.size() && offset < join_label->value;
       offset += 4) {
    const auto instruction = read_u32(text->bytes, offset);
    const auto opcode = instruction & 0x7fU;
    const auto rd = (instruction >> 7) & 0x1fU;
    const auto funct3 = (instruction >> 12) & 0x7U;
    const auto rs1 = (instruction >> 15) & 0x1fU;
    const auto rs2 = (instruction >> 20) & 0x1fU;
    const auto imm = (instruction >> 20) & 0xfffU;

    if (opcode == 0x13U && rd == 10 && funct3 == 0 && rs1 == 11 && imm == 0) {
      saw_raw_operand_copy = true;
    }
    if (opcode == 0x13U && rd == 28 && funct3 == 0 && rs1 == 11 && imm == 0) {
      saw_operand_to_scratch = true;
    }
    if (opcode == 0x13U && rd == 29 && funct3 == 0 && rs1 == 0 && imm == 1) {
      saw_one_to_scratch = true;
    }
    if ((opcode == 0x13U && rd == 10 && funct3 == 4 && rs1 == 11 &&
         imm == 1) ||
        (opcode == 0x33U && rd == 10 && funct3 == 4 &&
         ((rs1 == 11 && rs2 != 11) ||
          (rs2 == 11 && rs1 != 11) ||
          (rs1 == 28 && rs2 == 29)))) {
      saw_compare_against_one = true;
    }
    if (opcode == 0x33U && rd == 10 && funct3 == 3 && rs1 == 0 &&
        rs2 == 10) {
      saw_booleanize_compare = true;
    }
  }
  if (saw_raw_operand_copy) {
    return fail("expected only-carrier-use NE edge to publish compare result, not raw operand register");
  }
  if ((!saw_compare_against_one &&
       !(saw_operand_to_scratch && saw_one_to_scratch)) ||
      !saw_booleanize_compare) {
    return fail("expected only-carrier-use NE edge to materialize b != 1 into select result");
  }
  return 0;
}

int suppresses_authorized_prepared_select_edge_source_producer_setup_object() {
  const auto prepared =
      make_prepared_join_transfer_select_with_suppressed_edge_compare_setup_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  const auto& module = result.module;
  if (!module.has_value()) {
    return fail(
        "expected explicit select-edge source-producer suppression to build: " +
        result.diagnostic);
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* false_copy_label = object::find_symbol(*module, ".Lmain_pred_false");
  const auto* join_label = object::find_symbol(*module, ".Lmain_join");
  if (text == nullptr || false_copy_label == nullptr || join_label == nullptr ||
      join_label->value <= false_copy_label->value) {
    return fail("expected suppressed select-edge setup object labels");
  }
  bool saw_edge_compare = false;
  bool saw_cast_source_materialization = false;
  for (std::size_t offset = 0; offset + 4 <= text->bytes.size(); offset += 4) {
    const auto instruction = read_u32(text->bytes, offset);
    const auto opcode = instruction & 0x7fU;
    const auto rd = (instruction >> 7) & 0x1fU;
    const auto funct3 = (instruction >> 12) & 0x7U;
    const auto rs1 = (instruction >> 15) & 0x1fU;
    const auto rs2 = (instruction >> 20) & 0x1fU;
    if (offset >= false_copy_label->value && offset < join_label->value &&
        rd == 29 && opcode != 0x03U) {
      saw_cast_source_materialization = true;
    }
    if (opcode == 0x33U && rd == 10 && funct3 == 3 && rs1 == 29 &&
        rs2 == 28) {
      saw_edge_compare = true;
    }
  }
  if (!saw_edge_compare) {
    return fail("expected predecessor edge to remain the compare materializer");
  }
  if (!saw_cast_source_materialization) {
    return fail("expected predecessor edge compare to materialize cast source");
  }
  return 0;
}

int rejects_prepared_join_transfer_select_cast_dependency_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  prepared.module.functions.front().blocks.at(3).insts.erase(
      prepared.module.functions.front().blocks.at(3).insts.begin());
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  auto& cast_source_home =
      prepared.value_locations.functions.front().value_homes.at(
          prepared.value_locations.functions.front().value_homes.size() - 2);
  cast_source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  cast_source_home.register_name.reset();
  cast_source_home.target_register_identity.reset();
  cast_source_home.slot_id = prepare::PreparedFrameSlotId{9};
  cast_source_home.offset_bytes = 8;
  cast_source_home.size_bytes = std::size_t{4};
  cast_source_home.align_bytes = std::size_t{4};
  prepared.stack_layout.frame_size_bytes = 16;
  prepared.value_locations.functions.front()
      .value_homes.back()
      .offset_bytes = 0;
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{9},
      .object_id = prepare::PreparedObjectId{9},
      .function_name = prepared.names.function_names.find("main"),
      .offset_bytes = 8,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = prepare::PreparedObjectId{9},
      .function_name = prepared.names.function_names.find("main"),
      .value_name = cast_source_home.value_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  prepared.module.functions.front().blocks.at(3).insts.insert(
      prepared.module.functions.front().blocks.at(3).insts.begin() + 2,
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Eq,
          .result = bir::Value::named(bir::TypeKind::I32, "%extra.use"),
          .operand_type = bir::TypeKind::Ptr,
          .lhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs.ptr"),
          .rhs = bir::Value::named(bir::TypeKind::Ptr, "%ptr.lhs"),
      });
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  auto* lhs_authority_compare = std::get_if<bir::BinaryInst>(
      &prepared.module.functions.front().blocks.at(3).insts.at(1));
  if (lhs_authority_compare != nullptr) {
    lhs_authority_compare->lhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs.ptr");
    lhs_authority_compare->rhs = bir::Value::named(bir::TypeKind::Ptr, "%ptr.lhs");
  }
  const auto ptr_lhs_name = prepared.names.value_names.find("%ptr.lhs");
  for (auto& home : prepared.value_locations.functions.front().value_homes) {
    if (home.value_name == ptr_lhs_name) {
      home.register_name = "t3";
      home.target_register_identity = prepare::PreparedTargetRegisterIdentity{
          .target_arch = c4c::TargetArch::Riscv64,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_class = prepare::PreparedRegisterClass::General,
          .physical_index = 28,
      };
    }
  }
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  prepared.value_locations.functions.front().move_bundles.back().moves.front().from_value_id = 7;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_cast_dependency_edge_compare_source_module();
  auto* compare = std::get_if<bir::BinaryInst>(
      &prepared.module.functions.front().blocks.at(3).insts.at(1));
  if (compare != nullptr) {
    compare->rhs = bir::Value::named(bir::TypeKind::Ptr, "%other.ptr");
  }
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }
  return 0;
}

int rejects_prepared_join_transfer_select_carrier_alias_ule_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ule_source_module();
  auto* final_select = std::get_if<bir::SelectInst>(
      &prepared.module.functions.front().blocks.at(3).insts.at(3));
  if (final_select != nullptr) {
    final_select->true_value = bir::Value::immediate_i32(1);
  }
  if (!prepare::collect_prepared_select_carrier_alias_authorities(prepared)
           .records.empty()) {
    return fail("expected missing carrier-alias authority mutation to publish no records");
  }
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ule_source_module();
  auto* compare = std::get_if<bir::BinaryInst>(
      &prepared.module.functions.front().blocks.at(3).insts.front());
  if (compare != nullptr) {
    compare->opcode = bir::BinaryOpcode::Eq;
  }
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared =
      make_prepared_join_transfer_select_with_carrier_alias_ule_source_module();
  const auto ptr_rhs_name = prepared.names.value_names.find("%ptr.rhs");
  auto& homes = prepared.value_locations.functions.front().value_homes;
  homes.erase(std::remove_if(homes.begin(),
                             homes.end(),
                             [&](const prepare::PreparedValueHome& home) {
                               return home.value_name == ptr_rhs_name;
                             }),
              homes.end());
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int keeps_unauthorized_prepared_select_edge_source_producer_suppression_fail_closed() {
  constexpr const char* diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  const auto accepted =
      make_prepared_join_transfer_select_with_suppressed_edge_compare_setup_module();
  const auto accepted_module = rv64::build_rv64_prepared_text_object_module(accepted);
  if (!accepted_module.has_value()) {
    return fail("expected accepted suppression baseline to build");
  }
  const auto* accepted_text = object::find_section(*accepted_module, ".text");
  if (accepted_text == nullptr) {
    return fail("expected accepted suppression baseline text section");
  }

  auto prepared =
      make_prepared_join_transfer_select_with_suppressed_edge_compare_setup_module();
  prepared.value_locations.functions.front()
      .move_bundles.back()
      .instruction_index = 0;
  for (auto& move :
       prepared.value_locations.functions.front().move_bundles.back().moves) {
    move.instruction_index = 0;
  }
  if (!prepare::collect_prepared_select_edge_source_producer_placements(prepared)
           .records.empty()) {
    return fail("expected wrong-site suppression bundle to publish no placement");
  }
  auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  const auto* text = module.has_value() ? object::find_section(*module, ".text")
                                        : nullptr;
  if (text == nullptr || text->bytes != accepted_text->bytes) {
    return fail("expected wrong-site suppression bundle to emit no generic moves");
  }

  prepared =
      make_prepared_join_transfer_select_with_suppressed_edge_compare_setup_module();
  prepared.value_locations.functions.front()
      .move_bundles.back()
      .moves.back()
      .to_value_id = 3;
  if (!prepare::collect_prepared_select_edge_source_producer_placements(prepared)
           .records.empty()) {
    return fail("expected wrong-destination suppression bundle to publish no placement");
  }
  module = rv64::build_rv64_prepared_text_object_module(prepared);
  text = module.has_value() ? object::find_section(*module, ".text") : nullptr;
  if (text == nullptr || text->bytes != accepted_text->bytes) {
    return fail(
        "expected wrong-destination suppression bundle to emit no generic moves");
  }

  prepared =
      make_prepared_join_transfer_select_with_suppressed_edge_compare_setup_module();
  prepared.value_locations.functions.front()
      .move_bundles.back()
      .moves.front()
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot;
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
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
  if (expect_prepared_rejection_diagnostic_contains(
          stack_source,
          {
              "unsupported_move_bundle_target_shape: prepared select publication move bundle requires unsupported RV64 moves",
              "select_publication_evidence=yes",
              "event_kind=pre_terminator_copies",
              "function=main",
              "block_label=pred.false",
              "phase=block_entry",
              "authority=out_of_ssa_parallel_copy",
              "move_count=1",
              "parallel_copy=yes",
              "parallel_copy_predecessor=pred.false",
              "parallel_copy_successor=join",
              "parallel_copy_execution_site=predecessor_terminator",
              "selected_step_index=0",
              "selected_step_kind=move",
              "selected_move_carrier=select_materialization",
              "selected_move_destination_value_id=3",
              "publication_present=yes",
              "publication_status=available",
              "publication_predecessor=pred.false",
              "publication_successor=join",
              "publication_destination_value_id=3",
              "publication_destination_value_name=%selected",
              "publication_source_value_id=2",
              "publication_source_value_name=%fallback",
              "publication_source_producer=unknown",
              "publication_source_home_kind=stack_slot",
              "publication_destination_home_kind=register",
              "publication_carrier=select_materialization",
              "intent_status=unsupported_source_home",
              "intent_source_value_id=2",
              "intent_source_stack_offset=<none>",
              "intent_destination_register=<none>",
              "select_publication_rejection_reason=intent_status_unsupported_source_home",
          }) != 0) {
    return 1;
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

int publishes_select_publication_stack_home_move_intent_fields() {
  auto stack_source = make_prepared_join_transfer_select_with_published_copies_module();
  const auto function_name = stack_source.names.function_names.find("main");
  const auto false_predecessor =
      stack_source.names.block_labels.find("pred.false");
  const auto join_label = stack_source.names.block_labels.find("join");
  const auto false_name = stack_source.names.value_names.find("%fallback");
  const auto result_name = stack_source.names.value_names.find("%selected");

  auto& stack_source_function = stack_source.module.functions.front();
  stack_source_function.return_type = bir::TypeKind::Ptr;
  stack_source_function.return_size_bytes = 8;
  stack_source_function.return_align_bytes = 8;
  auto& stack_source_join = stack_source_function.blocks.at(3);
  auto* stack_source_select =
      std::get_if<bir::SelectInst>(&stack_source_join.insts.front());
  if (stack_source_select == nullptr) {
    return fail("expected prepared select fixture");
  }
  stack_source_select->result = bir::Value::named(bir::TypeKind::Ptr, "%selected");
  stack_source_select->true_value = null_pointer_value();
  stack_source_select->false_value = bir::Value::named(bir::TypeKind::Ptr,
                                                       "%fallback");
  stack_source_join.terminator.value =
      bir::Value::named(bir::TypeKind::Ptr, "%selected");

  auto& stack_source_join_transfer =
      stack_source.control_flow.functions.front().join_transfers.front();
  stack_source_join_transfer.result =
      bir::Value::named(bir::TypeKind::Ptr, "%selected");
  stack_source_join_transfer.incomings.at(0).value = null_pointer_value();
  stack_source_join_transfer.incomings.at(1).value =
      bir::Value::named(bir::TypeKind::Ptr, "%fallback");
  stack_source_join_transfer.edge_transfers.at(0).incoming_value =
      null_pointer_value();
  stack_source_join_transfer.edge_transfers.at(0).destination_value =
      bir::Value::named(bir::TypeKind::Ptr, "%selected");
  stack_source_join_transfer.edge_transfers.at(1).incoming_value =
      bir::Value::named(bir::TypeKind::Ptr, "%fallback");
  stack_source_join_transfer.edge_transfers.at(1).destination_value =
      bir::Value::named(bir::TypeKind::Ptr, "%selected");
  auto& stack_source_true_parallel_copy =
      stack_source.control_flow.functions.front().parallel_copy_bundles.at(0);
  stack_source_true_parallel_copy.moves.front().source_value =
      null_pointer_value();
  stack_source_true_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::Ptr, "%selected");
  auto& stack_source_parallel_copy =
      stack_source.control_flow.functions.front().parallel_copy_bundles.at(1);
  stack_source_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::Ptr, "%fallback");
  stack_source_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::Ptr, "%selected");

  auto& stack_source_homes =
      stack_source.value_locations.functions.front().value_homes;
  stack_source_homes.at(1) = rv64_sized_stack_slot_home(
      2,
      function_name,
      false_name,
      prepare::PreparedFrameSlotId{7},
      8,
      8);
  stack_source.value_locations.functions.front()
      .move_bundles.at(0)
      .moves.front()
      .source_immediate_i32 = 0;
  stack_source.stack_layout.frame_size_bytes = 16;
  stack_source.stack_layout.frame_alignment_bytes = 8;
  stack_source.stack_layout.frame_slots = {prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{7},
      .function_name = function_name,
      .offset_bytes = 8,
      .size_bytes = 8,
      .align_bytes = 8,
  }};

  auto stack_source_lookups = prepare::make_prepared_function_lookups(
      stack_source,
      stack_source.control_flow.functions.front());
  const auto stack_source_intent = rv64::consume_edge_publication_move_intent(
      &stack_source_lookups,
      false_predecessor,
      join_label,
      prepare::PreparedValueId{3});
  if (stack_source_intent.status !=
          rv64::EdgePublicationMoveIntentStatus::Available ||
      stack_source_intent.source_value_id !=
          std::optional<prepare::PreparedValueId>{2} ||
      stack_source_intent.source_type != bir::TypeKind::Ptr ||
      stack_source_intent.destination_type != bir::TypeKind::Ptr ||
      stack_source_intent.source_stack_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{7}} ||
      stack_source_intent.source_stack_offset_bytes !=
          std::optional<std::size_t>{8} ||
      stack_source_intent.source_stack_size_bytes !=
          std::optional<std::size_t>{8} ||
      stack_source_intent.destination_register != "a0" ||
      stack_source_intent.destination_stack_offset_bytes.has_value() ||
      stack_source_intent.instruction_text != "ld a0, 8(sp)") {
    return fail("expected pointer stack-source select-publication intent fields");
  }
  const auto stack_source_module =
      rv64::build_rv64_prepared_text_object_module(stack_source);
  if (!stack_source_module.has_value()) {
    return fail("expected pointer stack-source select-publication to materialize");
  }
  const auto* stack_source_text =
      object::find_section(*stack_source_module, ".text");
  if (stack_source_text == nullptr ||
      !contains_u32(stack_source_text->bytes, 0x00813503)) {
    return fail("expected pointer stack-source select-publication to emit ld a0, 8(sp)");
  }

  stack_source.value_locations.functions.front().value_homes.at(1).offset_bytes =
      4096;
  if (rv64::build_rv64_prepared_text_object_module(stack_source).has_value()) {
    return fail("expected large-offset pointer stack-source select-publication to remain fail-closed");
  }

  auto stack_destination =
      make_prepared_join_transfer_select_with_published_copies_module();
  const auto stack_destination_function_name =
      stack_destination.names.function_names.find("main");
  const auto stack_destination_false_predecessor =
      stack_destination.names.block_labels.find("pred.false");
  const auto stack_destination_join_label =
      stack_destination.names.block_labels.find("join");
  const auto stack_destination_result_name =
      stack_destination.names.value_names.find("%selected");
  auto& stack_destination_function = stack_destination.module.functions.front();
  stack_destination_function.return_type = bir::TypeKind::I16;
  stack_destination_function.return_size_bytes = 2;
  stack_destination_function.return_align_bytes = 2;
  auto& stack_destination_join = stack_destination_function.blocks.at(3);
  auto* stack_destination_select =
      std::get_if<bir::SelectInst>(&stack_destination_join.insts.front());
  if (stack_destination_select == nullptr) {
    return fail("expected prepared select fixture");
  }
  stack_destination_select->result =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  stack_destination_select->true_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_select->false_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_join.terminator.value =
      bir::Value::named(bir::TypeKind::I16, "%selected");

  auto& stack_destination_join_transfer =
      stack_destination.control_flow.functions.front().join_transfers.front();
  stack_destination_join_transfer.result =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  stack_destination_join_transfer.incomings.at(0).value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_join_transfer.incomings.at(1).value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_join_transfer.edge_transfers.at(0).incoming_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_join_transfer.edge_transfers.at(0).destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  stack_destination_join_transfer.edge_transfers.at(1).incoming_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_join_transfer.edge_transfers.at(1).destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  auto& stack_destination_true_parallel_copy =
      stack_destination.control_flow.functions.front().parallel_copy_bundles.at(0);
  stack_destination_true_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_true_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  auto& stack_destination_parallel_copy =
      stack_destination.control_flow.functions.front().parallel_copy_bundles.at(1);
  stack_destination_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_destination_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");

  auto& stack_destination_homes =
      stack_destination.value_locations.functions.front().value_homes;
  stack_destination_homes.at(2) = rv64_i16_stack_slot_home(
      3,
      stack_destination_function_name,
      stack_destination_result_name,
      prepare::PreparedFrameSlotId{8},
      4);
  stack_destination.stack_layout.frame_size_bytes = 8;
  stack_destination.stack_layout.frame_alignment_bytes = 4;
  stack_destination.stack_layout.frame_slots = {prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{8},
      .function_name = stack_destination_function_name,
      .offset_bytes = 4,
      .size_bytes = 2,
      .align_bytes = 2,
  }};

  auto stack_destination_lookups = prepare::make_prepared_function_lookups(
      stack_destination,
      stack_destination.control_flow.functions.front());
  const auto stack_destination_intent =
      rv64::consume_edge_publication_move_intent(
          &stack_destination_lookups,
          stack_destination_false_predecessor,
          stack_destination_join_label,
          prepare::PreparedValueId{3});
  if (stack_destination_intent.status !=
          rv64::EdgePublicationMoveIntentStatus::Available ||
      stack_destination_intent.source_value_id !=
          std::optional<prepare::PreparedValueId>{2} ||
      stack_destination_intent.source_type != bir::TypeKind::I16 ||
      stack_destination_intent.destination_type != bir::TypeKind::I16 ||
      stack_destination_intent.source_register != "t1" ||
      stack_destination_intent.destination_stack_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{8}} ||
      stack_destination_intent.destination_stack_offset_bytes !=
          std::optional<std::size_t>{4} ||
      stack_destination_intent.destination_stack_size_bytes !=
          std::optional<std::size_t>{2} ||
      stack_destination_intent.destination_register.empty() == false ||
      stack_destination_intent.instruction_text != "sh t1, 4(sp)") {
    return fail("expected i16 register-source stack-destination select-publication intent fields");
  }
  const auto stack_destination_module =
      rv64::build_rv64_prepared_text_object_module(stack_destination);
  if (!stack_destination_module.has_value()) {
    return fail("expected i16 stack-destination select-publication to materialize");
  }
  const auto* stack_destination_text =
      object::find_section(*stack_destination_module, ".text");
  if (stack_destination_text == nullptr ||
      !contains_u32(stack_destination_text->bytes, 0x00611223)) {
    return fail("expected i16 stack-destination select-publication to emit sh t1, 4(sp)");
  }

  auto byte_stack_destination = stack_destination;
  auto& byte_function = byte_stack_destination.module.functions.front();
  byte_function.return_type = bir::TypeKind::I8;
  byte_function.return_size_bytes = 1;
  byte_function.return_align_bytes = 1;
  auto& byte_join = byte_function.blocks.at(3);
  auto* byte_select = std::get_if<bir::SelectInst>(&byte_join.insts.front());
  if (byte_select == nullptr) {
    return fail("expected prepared byte select fixture");
  }
  byte_select->result = bir::Value::named(bir::TypeKind::I8, "%selected");
  byte_select->true_value = bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_select->false_value = bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_join.terminator.value =
      bir::Value::named(bir::TypeKind::I8, "%selected");
  auto& byte_join_transfer =
      byte_stack_destination.control_flow.functions.front().join_transfers.front();
  byte_join_transfer.result = bir::Value::named(bir::TypeKind::I8, "%selected");
  byte_join_transfer.incomings.at(0).value =
      bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_join_transfer.incomings.at(1).value =
      bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_join_transfer.edge_transfers.at(0).incoming_value =
      bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_join_transfer.edge_transfers.at(0).destination_value =
      bir::Value::named(bir::TypeKind::I8, "%selected");
  byte_join_transfer.edge_transfers.at(1).incoming_value =
      bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_join_transfer.edge_transfers.at(1).destination_value =
      bir::Value::named(bir::TypeKind::I8, "%selected");
  auto& byte_true_parallel_copy =
      byte_stack_destination.control_flow.functions.front().parallel_copy_bundles.at(0);
  byte_true_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_true_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I8, "%selected");
  auto& byte_parallel_copy =
      byte_stack_destination.control_flow.functions.front().parallel_copy_bundles.at(1);
  byte_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I8, "%fallback");
  byte_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I8, "%selected");
  byte_stack_destination.value_locations.functions.front().value_homes.at(2) =
      rv64_sized_stack_slot_home(3,
                                 stack_destination_function_name,
                                 stack_destination_result_name,
                                 prepare::PreparedFrameSlotId{8},
                                 4,
                                 1);
  byte_stack_destination.stack_layout.frame_slots.front().size_bytes = 1;
  byte_stack_destination.stack_layout.frame_slots.front().align_bytes = 1;
  const auto byte_stack_destination_module =
      rv64::build_rv64_prepared_text_object_module(byte_stack_destination);
  if (!byte_stack_destination_module.has_value()) {
    return fail("expected i8 stack-destination select-publication to materialize");
  }
  const auto* byte_stack_destination_text =
      object::find_section(*byte_stack_destination_module, ".text");
  if (byte_stack_destination_text == nullptr ||
      !contains_u32(byte_stack_destination_text->bytes, 0x00610223)) {
    return fail("expected i8 stack-destination select-publication to emit sb t1, 4(sp)");
  }

  auto word_stack_destination = stack_destination;
  auto& word_function = word_stack_destination.module.functions.front();
  word_function.return_type = bir::TypeKind::I32;
  word_function.return_size_bytes = 4;
  word_function.return_align_bytes = 4;
  auto& word_join = word_function.blocks.at(3);
  auto* word_select = std::get_if<bir::SelectInst>(&word_join.insts.front());
  if (word_select == nullptr) {
    return fail("expected prepared word select fixture");
  }
  word_select->result = bir::Value::named(bir::TypeKind::I32, "%selected");
  word_select->true_value = bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_select->false_value = bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_join.terminator.value =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  auto& word_join_transfer =
      word_stack_destination.control_flow.functions.front().join_transfers.front();
  word_join_transfer.result = bir::Value::named(bir::TypeKind::I32, "%selected");
  word_join_transfer.incomings.at(0).value =
      bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_join_transfer.incomings.at(1).value =
      bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_join_transfer.edge_transfers.at(0).incoming_value =
      bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_join_transfer.edge_transfers.at(0).destination_value =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  word_join_transfer.edge_transfers.at(1).incoming_value =
      bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_join_transfer.edge_transfers.at(1).destination_value =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  auto& word_true_parallel_copy =
      word_stack_destination.control_flow.functions.front().parallel_copy_bundles.at(0);
  word_true_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_true_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  auto& word_parallel_copy =
      word_stack_destination.control_flow.functions.front().parallel_copy_bundles.at(1);
  word_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I32, "%fallback");
  word_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  word_stack_destination.value_locations.functions.front().value_homes.at(2) =
      rv64_sized_stack_slot_home(3,
                                 stack_destination_function_name,
                                 stack_destination_result_name,
                                 prepare::PreparedFrameSlotId{8},
                                 4,
                                 4);
  word_stack_destination.stack_layout.frame_slots.front().size_bytes = 4;
  word_stack_destination.stack_layout.frame_slots.front().align_bytes = 4;
  const auto word_stack_destination_module =
      rv64::build_rv64_prepared_text_object_module(word_stack_destination);
  if (!word_stack_destination_module.has_value()) {
    return fail("expected i32 stack-destination select-publication to materialize");
  }
  const auto* word_stack_destination_text =
      object::find_section(*word_stack_destination_module, ".text");
  if (word_stack_destination_text == nullptr ||
      !contains_u32(word_stack_destination_text->bytes, 0x00612223)) {
    return fail("expected i32 stack-destination select-publication to emit sw t1, 4(sp)");
  }

  stack_destination.value_locations.functions.front()
      .value_homes.at(2)
      .offset_bytes = 4096;
  auto large_offset_lookups = prepare::make_prepared_function_lookups(
      stack_destination,
      stack_destination.control_flow.functions.front());
  const auto large_offset_intent = rv64::consume_edge_publication_move_intent(
      &large_offset_lookups,
      stack_destination_false_predecessor,
      stack_destination_join_label,
      prepare::PreparedValueId{3});
  if (large_offset_intent.status !=
          rv64::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome ||
      large_offset_intent.destination_stack_offset_bytes.has_value()) {
    return fail("expected large stack-destination offset to remain fail-closed");
  }
  if (rv64::build_rv64_prepared_text_object_module(stack_destination).has_value()) {
    return fail("expected large-offset stack-destination select-publication to remain fail-closed");
  }

  auto stack_to_stack_destination =
      make_prepared_join_transfer_select_with_published_copies_module();
  auto& stack_to_stack_function =
      stack_to_stack_destination.module.functions.front();
  stack_to_stack_function.return_type = bir::TypeKind::I16;
  stack_to_stack_function.return_size_bytes = 2;
  stack_to_stack_function.return_align_bytes = 2;
  auto& stack_to_stack_join = stack_to_stack_function.blocks.at(3);
  auto* stack_to_stack_select =
      std::get_if<bir::SelectInst>(&stack_to_stack_join.insts.front());
  if (stack_to_stack_select == nullptr) {
    return fail("expected prepared stack-to-stack select fixture");
  }
  stack_to_stack_select->result =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  stack_to_stack_select->true_value = bir::Value::immediate_i16(1);
  stack_to_stack_select->false_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_to_stack_join.terminator.value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  auto& stack_to_stack_join_transfer =
      stack_to_stack_destination.control_flow.functions.front().join_transfers.front();
  stack_to_stack_join_transfer.result =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  stack_to_stack_join_transfer.incomings.at(0).value =
      bir::Value::immediate_i16(1);
  stack_to_stack_join_transfer.incomings.at(1).value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_to_stack_join_transfer.edge_transfers.at(0).incoming_value =
      bir::Value::immediate_i16(1);
  stack_to_stack_join_transfer.edge_transfers.at(0).destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  stack_to_stack_join_transfer.edge_transfers.at(1).incoming_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_to_stack_join_transfer.edge_transfers.at(1).destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  auto& stack_to_stack_parallel_copy =
      stack_to_stack_destination.control_flow.functions.front()
          .parallel_copy_bundles.at(1);
  stack_to_stack_parallel_copy.moves.front().source_value =
      bir::Value::named(bir::TypeKind::I16, "%fallback");
  stack_to_stack_parallel_copy.moves.front().destination_value =
      bir::Value::named(bir::TypeKind::I16, "%selected");
  auto& stack_to_stack_homes =
      stack_to_stack_destination.value_locations.functions.front().value_homes;
  stack_to_stack_homes.at(1) = rv64_i16_stack_slot_home(
      2,
      stack_destination_function_name,
      false_name,
      prepare::PreparedFrameSlotId{9},
      0);
  stack_to_stack_homes.at(2) = rv64_i16_stack_slot_home(
      3,
      stack_destination_function_name,
      stack_destination_result_name,
      prepare::PreparedFrameSlotId{8},
      4);
  stack_to_stack_destination.stack_layout.frame_size_bytes = 8;
  stack_to_stack_destination.stack_layout.frame_alignment_bytes = 4;
  stack_to_stack_destination.stack_layout.frame_slots = {
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{9},
          .function_name = stack_destination_function_name,
          .offset_bytes = 0,
          .size_bytes = 2,
          .align_bytes = 2,
      },
      prepare::PreparedFrameSlot{
          .slot_id = prepare::PreparedFrameSlotId{8},
          .function_name = stack_destination_function_name,
          .offset_bytes = 4,
          .size_bytes = 2,
          .align_bytes = 2,
      },
  };
  if (rv64::build_rv64_prepared_text_object_module(stack_to_stack_destination)
          .has_value()) {
    return fail("expected stack-to-stack select-publication destination to remain fail-closed");
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

int builds_prepared_frame_slot_value_and_prior_preserved_arg_call_object() {
  const auto prepared =
      make_prepared_frame_slot_value_and_prior_preserved_arg_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared frame-slot/prior-preserved arg call RV64 object module to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* sink = object::find_symbol(module, "sink");
  const auto* keep = object::find_symbol(module, "keep");
  const auto* main = object::find_symbol(module, "main");
  if (text == nullptr || sink == nullptr || keep == nullptr || main == nullptr) {
    return fail("expected frame-slot/prior-preserved arg call object to publish text/functions");
  }
  if (module.relocations.size() != 2 ||
      module.relocations[0].section != text->id ||
      module.relocations[1].section != text->id ||
      module.relocations[0].type != R_RISCV_CALL_PLT ||
      module.relocations[1].type != R_RISCV_CALL_PLT ||
      module.relocations[0].symbol != keep->id ||
      module.relocations[1].symbol != sink->id ||
      module.relocations[0].offset >= module.relocations[1].offset ||
      module.relocations[1].offset < main->value + 24) {
    return fail("expected ordered keep/sink same-module call relocations in main");
  }
  const auto sink_call_offset = module.relocations[1].offset;
  if (read_u32(text->bytes, sink_call_offset - 12) != 0x01013503 ||
      read_u32(text->bytes, sink_call_offset - 8) != 0x01813583 ||
      read_u32(text->bytes, sink_call_offset - 4) != 0x00090613) {
    return fail("expected same-module call to consume two frame-slot GPR args and prior-preserved s2");
  }
  if (read_u32(text->bytes, sink_call_offset + 8) != 0x00050293) {
    return fail("expected same-module call result to publish from a0 to %result owner register");
  }
  return 0;
}

int expect_frame_slot_value_arg_call_rejection(
    const prepare::PreparedBirModule& prepared) {
  return expect_prepared_rejection_diagnostic(
      prepared, kUnsupportedSameModuleCallAbiDiagnostic);
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
  if (expect_prepared_rejection_diagnostic(
          prepared, kGenericUnsupportedInstructionFragmentDiagnostic) != 0) {
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
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame") !=
      0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_value_arg_call_module();
  prepared.value_locations.functions[1].value_homes[0].offset_bytes = 2048;
  prepared.call_plans.functions[0].calls[0].arguments[0].source_stack_offset_bytes =
      2048;
  prepared.stack_layout.frame_slots[0].offset_bytes = 2048;
  prepared.frame_plan.functions[1].frame_size_bytes = 2056;
  if (expect_frame_slot_value_arg_call_rejection(prepared) != 0) {
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
          0x01810313 ||
      read_u32(text->bytes, module->relocations[0].offset - 20) !=
          0x00613c23 ||
      read_u32(text->bytes, module->relocations[0].offset - 16) !=
          0x01810513 ||
      read_u32(text->bytes, module->relocations[0].offset - 12) !=
          0x01810313 ||
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
      prepared, kUnsupportedSameModuleCallAbiDiagnostic);
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
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame") !=
      0) {
    return 1;
  }

  prepared = make_prepared_frame_slot_address_arg_call_module();
  prepared.frame_plan.functions[1].uses_frame_pointer_for_fixed_slots = true;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_stack_frame: RV64 object route requires a supported prepared stack frame") !=
      0) {
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

int builds_prepared_empty_tied_scalar_gpr_inline_asm_object() {
  const auto prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected empty-template tied scalar GPR inline-asm object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected empty-template tied inline-asm object to publish text/main");
  }
  if (text->bytes.size() != 8 || text->size_bytes != 8 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 8) {
    return fail("expected empty-template tied inline-asm object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028513 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected empty-template tied inline-asm to publish result home then return");
  }
  return 0;
}

int builds_prepared_no_result_memory_clobber_inline_asm_object() {
  const auto prepared = make_prepared_no_result_memory_clobber_inline_asm_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected no-result memory-clobber inline-asm object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* main_symbol = object::find_symbol(*module, "main");
  if (text == nullptr || main_symbol == nullptr) {
    return fail("expected no-result memory-clobber inline-asm object to publish text/main");
  }
  if (text->bytes.size() != 4 || text->size_bytes != 4 ||
      main_symbol->value != 0 || main_symbol->size_bytes != 4) {
    return fail("expected no-result memory-clobber inline-asm object text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00008067) {
    return fail("expected no-result memory-clobber inline-asm to emit only ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected no-result memory-clobber inline-asm object to need no relocations");
  }
  return 0;
}

int rejects_prepared_no_result_memory_clobber_inline_asm_fail_closed_shapes() {
  auto prepared = make_prepared_no_result_memory_clobber_inline_asm_module(false);
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_inline_asm_fragment: RV64 object route requires a complete supported inline-asm carrier") !=
      0) {
    return 1;
  }

  prepared = make_prepared_no_result_memory_clobber_inline_asm_module();
  prepared.inline_asm_carriers.functions[0].carriers[0].side_effects = false;
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_inline_asm_fragment: RV64 object route requires a complete supported inline-asm carrier") !=
      0) {
    return 1;
  }

  prepared = make_prepared_no_result_memory_clobber_inline_asm_module();
  prepared.inline_asm_carriers.functions[0].carriers[0].constraints = "r,~{memory}";
  if (expect_prepared_rejection_diagnostic(
          prepared,
          "unsupported_inline_asm_fragment: RV64 object route requires a complete supported inline-asm carrier") !=
      0) {
    return 1;
  }

  return 0;
}

int rejects_prepared_symbol_address_imr_inline_asm_with_precise_diagnostic() {
  const auto prepared = make_prepared_symbol_address_imr_inline_asm_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (result.ok() || result.module.has_value()) {
    return fail("expected imr symbol-address inline-asm object path to reject");
  }
  constexpr const char* expected =
      "unsupported_inline_asm_fragment: RV64 object route requires a complete supported inline-asm carrier";
  if (result.diagnostic != expected) {
    return fail("expected imr symbol-address inline-asm diagnostic `" +
                std::string{expected} + "`, got `" + result.diagnostic + "`");
  }
  if (result.diagnostic.find("unsupported_instruction_fragment") !=
      std::string::npos) {
    return fail("expected imr symbol-address inline-asm not to reach generic unsupported instruction fallback");
  }
  return 0;
}

int rejects_prepared_empty_tied_scalar_gpr_inline_asm_fail_closed_shapes() {
  auto prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module(false);
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to require a complete carrier");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  prepared.inline_asm_carriers.functions[0].carriers[0].clobbers.push_back("memory");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject clobbers");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& non_empty_call =
      std::get<bir::CallInst>(prepared.module.functions[0].blocks[0].insts[0]);
  non_empty_call.inline_asm->asm_text = "addi %0, %0, 0";
  prepared.inline_asm_carriers.functions[0].carriers[0].asm_text = "addi %0, %0, 0";
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm packet to reject non-empty templates");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& named_carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  named_carrier.has_named_operand_references = true;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject named operands");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& modifier_carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  modifier_carrier.has_template_modifiers = true;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject template modifiers");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& missing_result_carrier =
      prepared.inline_asm_carriers.functions[0].carriers[0];
  missing_result_carrier.result_home = std::nullopt;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject missing result home");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& missing_home_carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  missing_home_carrier.operands[1].home = std::nullopt;
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject missing tied home");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& mismatch_carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  mismatch_carrier.operands[1].home =
      rv64_gpr_home(2,
                    mismatch_carrier.function_name,
                    *mismatch_carrier.operands[1].value_name,
                    "t1",
                    6);
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject mismatched tied home");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& vector_carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  vector_carrier.operands[1].home =
      rv64_vector_home(2,
                       vector_carrier.function_name,
                       *vector_carrier.operands[1].value_name,
                       "v1",
                       1);
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject vector homes");
  }

  prepared = make_prepared_empty_tied_scalar_gpr_inline_asm_module();
  auto& f128_call =
      std::get<bir::CallInst>(prepared.module.functions[0].blocks[0].insts[0]);
  f128_call.result = bir::Value::named(bir::TypeKind::F128, "%out");
  f128_call.return_type = bir::TypeKind::F128;
  auto& f128_carrier = prepared.inline_asm_carriers.functions[0].carriers[0];
  f128_carrier.result = bir::Value::named(bir::TypeKind::F128, "%out");
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected empty-template tied inline-asm to reject F128 result");
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

int rejects_prepared_f64_global_load_without_prepared_access() {
  return expect_prepared_rejection_diagnostic(
      make_prepared_global_f64_load_module(false),
      "unsupported_global_data: RV64 object route requires prepared direct global-symbol base-plus-offset memory addressing");
}

int emits_prepared_f64_global_load_from_explicit_facts() {
  const auto prepared = make_prepared_global_f64_load_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit F64 global load");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* rodata = object::find_section(*module, ".rodata");
  const auto* global_symbol = object::find_symbol(*module, "d");
  const auto* auipc_label = object::find_symbol(*module, ".Lpcrel_hi_global_load_1_1_0");
  if (text == nullptr || rodata == nullptr || global_symbol == nullptr ||
      auipc_label == nullptr) {
    return fail("expected text, rodata, F64 global symbol, and load AUIPC label");
  }
  if (text->bytes.size() < 12) {
    return fail("expected PC-relative address materialization and F64 load");
  }
  const auto load = read_u32(text->bytes, 8);
  if ((load & 0x7fU) != 0x07U || ((load >> 12) & 0x7U) != 3U ||
      ((load >> 7) & 0x1fU) != 0U || ((load >> 15) & 0x1fU) != 6U) {
    return fail("expected prepared F64 global load to encode fld ft0, 0(t1)");
  }
  if (global_symbol->binding != object::SymbolBinding::Global ||
      global_symbol->kind != object::SymbolKind::Object ||
      global_symbol->section != std::optional<object::SectionId>{rodata->id} ||
      global_symbol->value != 0 || global_symbol->size_bytes != 8) {
    return fail("expected F64 global load relocation target to be a defined object");
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
    return fail("expected prepared F64 global load PC-relative relocation pair");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize F64 global load");
  }
  return 0;
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

int builds_prepared_uitofp_i32_to_f32_then_fpext_object() {
  const auto prepared = make_prepared_uitofp_i32_to_f32_then_fpext_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared UIToFP i32 to F32 then FPExt RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function =
      object::find_symbol(*module, "uitofp_i32_to_f32_then_fpext");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared UIToFP/FPExt object to publish text/function");
  }
  if (text->bytes.size() != 12 || text->size_bytes != 12 ||
      function->value != 0 || function->size_bytes != 12 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared UIToFP/FPExt object text layout");
  }
  if (read_u32(text->bytes, 0) != 0xd0128053 ||
      read_u32(text->bytes, 4) != 0x420004d3 ||
      read_u32(text->bytes, 8) != 0x00008067) {
    return fail("expected fcvt.s.wu ft0, t0, rne; fcvt.d.s fs1, ft0, rne; ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared UIToFP/FPExt object to need no relocations");
  }
  return 0;
}

int builds_prepared_fp_to_int_casts_with_rtz_rounding_object() {
  struct TestCase {
    const char* function;
    bir::CastOpcode opcode;
    bir::TypeKind source_type;
    bir::TypeKind result_type;
    std::uint32_t instruction;
    const char* expected;
  };
  constexpr TestCase cases[] = {
      {"fptosi_f32_to_i32",
       bir::CastOpcode::FPToSI,
       bir::TypeKind::F32,
       bir::TypeKind::I32,
       0xc0051553,
       "fcvt.w.s a0, fa0, rtz"},
      {"fptoui_f32_to_i32",
       bir::CastOpcode::FPToUI,
       bir::TypeKind::F32,
       bir::TypeKind::I32,
       0xc0151553,
       "fcvt.wu.s a0, fa0, rtz"},
      {"fptosi_f32_to_i64",
       bir::CastOpcode::FPToSI,
       bir::TypeKind::F32,
       bir::TypeKind::I64,
       0xc0251553,
       "fcvt.l.s a0, fa0, rtz"},
      {"fptoui_f32_to_i64",
       bir::CastOpcode::FPToUI,
       bir::TypeKind::F32,
       bir::TypeKind::I64,
       0xc0351553,
       "fcvt.lu.s a0, fa0, rtz"},
      {"fptosi_f64_to_i32",
       bir::CastOpcode::FPToSI,
       bir::TypeKind::F64,
       bir::TypeKind::I32,
       0xc2051553,
       "fcvt.w.d a0, fa0, rtz"},
      {"fptoui_f64_to_i32",
       bir::CastOpcode::FPToUI,
       bir::TypeKind::F64,
       bir::TypeKind::I32,
       0xc2151553,
       "fcvt.wu.d a0, fa0, rtz"},
      {"fptosi_f64_to_i64",
       bir::CastOpcode::FPToSI,
       bir::TypeKind::F64,
       bir::TypeKind::I64,
       0xc2251553,
       "fcvt.l.d a0, fa0, rtz"},
      {"fptoui_f64_to_i64",
       bir::CastOpcode::FPToUI,
       bir::TypeKind::F64,
       bir::TypeKind::I64,
       0xc2351553,
       "fcvt.lu.d a0, fa0, rtz"},
  };
  for (const auto& test_case : cases) {
    const auto prepared =
        make_prepared_fp_to_int_cast_module(test_case.function,
                                            test_case.opcode,
                                            test_case.source_type,
                                            test_case.result_type);
    const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
    if (!module.has_value()) {
      return fail(std::string{"expected prepared "} + test_case.function +
                  " RV64 object module to build");
    }
    const auto* text = object::find_section(*module, ".text");
    const auto* function = object::find_symbol(*module, test_case.function);
    if (text == nullptr || function == nullptr) {
      return fail(std::string{"expected prepared "} + test_case.function +
                  " object to publish text/function");
    }
    if (text->bytes.size() != 8 || text->size_bytes != 8 ||
        function->value != 0 || function->size_bytes != 8 ||
        function->section != std::optional<object::SectionId>{text->id}) {
      return fail(std::string{"expected prepared "} + test_case.function +
                  " object text layout");
    }
    if (read_u32(text->bytes, 0) != test_case.instruction ||
        read_u32(text->bytes, 4) != 0x00008067) {
      return fail(std::string{"expected "} + test_case.expected +
                  " followed by ret");
    }
    if (!module->relocations.empty()) {
      return fail(std::string{"expected prepared "} + test_case.function +
                  " object to need no relocations");
    }
  }
  return 0;
}

int rejects_prepared_fp_to_int_cast_fail_closed_shapes() {
  constexpr const char* diagnostic =
      "unsupported_floating_cast: RV64 object route supports only prepared FPR width casts, I32/I64-to-F32/F64 integer-to-floating casts, and FPR-register-source F32/F64-to-I32/I64 floating-to-integer casts";

  auto prepared =
      make_prepared_fp_to_int_cast_module("fptosi_missing_source_home",
                                          bir::CastOpcode::FPToSI,
                                          bir::TypeKind::F64,
                                          bir::TypeKind::I64);
  prepared.value_locations.functions[0].value_homes.erase(
      prepared.value_locations.functions[0].value_homes.begin());
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_fp_to_int_cast_module("fptosi_missing_destination_home",
                                                 bir::CastOpcode::FPToSI,
                                                 bir::TypeKind::F64,
                                                 bir::TypeKind::I64);
  prepared.value_locations.functions[0].value_homes.pop_back();
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_fp_to_int_cast_module("fptosi_gpr_source_home",
                                                 bir::CastOpcode::FPToSI,
                                                 bir::TypeKind::F64,
                                                 bir::TypeKind::I64);
  auto& gpr_source = prepared.value_locations.functions[0].value_homes[0];
  gpr_source = rv64_gpr_home(gpr_source.value_id,
                             gpr_source.function_name,
                             gpr_source.value_name,
                             "a0",
                             10);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_fp_to_int_cast_module("fptosi_fpr_destination_home",
                                                 bir::CastOpcode::FPToSI,
                                                 bir::TypeKind::F64,
                                                 bir::TypeKind::I64);
  auto& fpr_destination = prepared.value_locations.functions[0].value_homes[1];
  fpr_destination = make_fpr_home(fpr_destination.function_name,
                                  fpr_destination.value_name,
                                  fpr_destination.value_id,
                                  "fa0",
                                  10);
  if (expect_prepared_rejection_diagnostic(prepared, diagnostic) != 0) {
    return 1;
  }

  if (expect_prepared_rejection_diagnostic(
          make_prepared_fp_to_int_cast_module("fptosi_f32_to_i16",
                                              bir::CastOpcode::FPToSI,
                                              bir::TypeKind::F32,
                                              bir::TypeKind::I16),
          diagnostic) != 0) {
    return 1;
  }

  if (expect_prepared_rejection_diagnostic(
          make_prepared_fp_to_int_cast_module("fptosi_f128_to_i64",
                                              bir::CastOpcode::FPToSI,
                                              bir::TypeKind::F128,
                                              bir::TypeKind::I64),
          "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering") != 0) {
    return 1;
  }

  if (expect_prepared_rejection_diagnostic(
          make_prepared_fp_to_int_cast_module("fp_bitcast_to_i32",
                                              bir::CastOpcode::Bitcast,
                                              bir::TypeKind::F32,
                                              bir::TypeKind::I32),
          diagnostic) != 0) {
    return 1;
  }

  return 0;
}

int builds_prepared_f64_immediate_fptrunc_object() {
  const auto prepared = make_prepared_f64_immediate_fptrunc_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared F64 immediate FPTrunc RV64 object module to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "f64_immediate_fptrunc");
  if (text == nullptr || function == nullptr) {
    return fail("expected prepared F64 immediate FPTrunc object to publish text/function");
  }
  if (text->bytes.size() != 56 || text->size_bytes != 56 ||
      function->value != 0 || function->size_bytes != 56 ||
      function->section != std::optional<object::SectionId>{text->id}) {
    return fail("expected prepared F64 immediate FPTrunc object text layout");
  }
  const std::uint32_t expected[] = {
      0x00400e13, 0x00ce1e13, 0xff2e0e13, 0x00ce1e13,
      0x99ae0e13, 0x00ce1e13, 0x99ae0e13, 0x00ce1e13,
      0x99ae0e13, 0x00ce1e13, 0x99ae0e13, 0xf20e02d3,
      0x40128053, 0x00008067,
  };
  for (std::size_t index = 0; index < std::size(expected); ++index) {
    if (read_u32(text->bytes, index * 4) != expected[index]) {
      return fail("expected materialized F64 bits, fmv.d.x, fcvt.s.d, and ret");
    }
  }
  if (!module->relocations.empty()) {
    return fail("expected prepared F64 immediate FPTrunc object to need no relocations");
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

int builds_prepared_direct_global_return_authority_object() {
  const auto prepared = make_prepared_direct_global_return_authority_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared direct-global return authority object to build");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* function = object::find_symbol(*module, "direct_global_return");
  if (text == nullptr || function == nullptr) {
    return fail("expected direct-global return object to publish text/function");
  }
  if (text->bytes.size() != 8 || function->size_bytes != 8) {
    return fail("expected direct-global return text layout");
  }
  if (read_u32(text->bytes, 0) != 0x00028513 ||
      read_u32(text->bytes, 4) != 0x00008067) {
    return fail("expected mv a0, t0 followed by ret");
  }
  if (!module->relocations.empty()) {
    return fail("expected direct-global return authority object to need no relocations");
  }
  return 0;
}

int rejects_prepared_direct_global_return_authority_fail_closed_shapes() {
  constexpr const char* terminator_diagnostic =
      "unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering";
  constexpr const char* move_diagnostic =
      "unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves";

  auto prepared = make_prepared_direct_global_return_authority_module();
  prepared.module.functions[0]
      .blocks[0]
      .terminator.value->pointer_symbol_link_name_id = c4c::kInvalidLinkName;
  if (expect_prepared_rejection_diagnostic(prepared, terminator_diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_direct_global_return_authority_module();
  prepared.value_locations.functions[0].move_bundles.clear();
  if (expect_prepared_rejection_diagnostic(prepared, terminator_diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_direct_global_return_authority_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .from_value_id = 99;
  if (expect_prepared_rejection_diagnostic(prepared, move_diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_direct_global_return_authority_module();
  prepared.value_locations.functions[0].value_homes[0].kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions[0].value_homes[0].register_name = std::nullopt;
  if (expect_prepared_rejection_diagnostic(prepared, move_diagnostic) != 0) {
    return 1;
  }

  prepared = make_prepared_direct_global_return_authority_module();
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_register_name = std::string{"fa0"};
  prepared.value_locations.functions[0].move_bundles[0]
      .moves[0]
      .destination_register_placement->bank = prepare::PreparedRegisterBank::Fpr;
  if (expect_prepared_rejection_diagnostic(prepared, move_diagnostic) != 0) {
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
      "unsupported_floating_cast: RV64 object route supports only prepared FPR width casts, I32/I64-to-F32/F64 integer-to-floating casts, and FPR-register-source F32/F64-to-I32/I64 floating-to-integer casts");
}

int emits_prepared_selected_symbol_pointer_global_object_storage() {
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

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected selected symbol-pointer global object data to build");
  }
  const auto* data = object::find_section(*module, ".data");
  if (data == nullptr || !data->writable || data->executable ||
      data->align_bytes != 8 ||
      data->bytes != std::vector<std::uint8_t>(8, 0)) {
    return fail("expected selected symbol-pointer object bytes in data");
  }
  const auto* global = object::find_symbol(*module, "g");
  const auto* target_symbol = object::find_symbol(*module, "target");
  if (global == nullptr ||
      global->binding != object::SymbolBinding::Global ||
      global->kind != object::SymbolKind::Object ||
      global->section != std::optional<object::SectionId>{data->id} ||
      global->value != 0 || global->size_bytes != 8 ||
      target_symbol == nullptr ||
      !object::is_undefined_symbol(*target_symbol)) {
    return fail("expected selected symbol-pointer object and target symbols");
  }
  const auto data_reloc = std::find_if(
      module->relocations.begin(),
      module->relocations.end(),
      [&](const object::RelocationRecord& relocation) {
        return relocation.section == data->id && relocation.offset == 0;
      });
  if (data_reloc == module->relocations.end() ||
      data_reloc->type != R_RISCV_64 ||
      data_reloc->symbol != target_symbol->id ||
      data_reloc->addend != 0) {
    return fail("expected selected symbol-pointer R_RISCV_64 relocation");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize selected symbol pointer");
  }
  (void)global_name;
  return 0;
}

int emits_prepared_selected_zero_pointer_global_bss_storage() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("cursor");
  prepared.module.globals.push_back(bir::Global{
      .name = "cursor",
      .link_name_id = link_name,
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = null_pointer_value(),
  });
  publish_prepared_object_data(prepared);

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected selected zero pointer global object data to build");
  }
  const auto* bss = object::find_section(*module, ".bss");
  if (bss == nullptr || !bss->writable || bss->executable ||
      bss->align_bytes != 8 || !bss->bytes.empty() ||
      bss->size_bytes != 8) {
    return fail("expected selected zero pointer object reservation in BSS");
  }
  const auto* symbol = object::find_symbol(*module, "cursor");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Global ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{bss->id} ||
      symbol->value != 0 || symbol->size_bytes != 8) {
    return fail("expected selected zero pointer object symbol in BSS");
  }
  const auto bss_reloc = std::find_if(
      module->relocations.begin(),
      module->relocations.end(),
      [&](const object::RelocationRecord& relocation) {
        return relocation.section == bss->id;
      });
  if (bss_reloc != module->relocations.end()) {
    return fail("expected selected zero pointer BSS object to need no relocation");
  }
  return 0;
}

int publishes_implicit_const_pointer_array_zero_fill_object_data_facts() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name =
      prepared.module.names.link_names.intern("implicit_pointer_table");
  prepared.module.globals.push_back(bir::Global{
      .name = "implicit_pointer_table",
      .link_name_id = link_name,
      .type = bir::TypeKind::Ptr,
      .is_constant = true,
      .has_integer_array_layout_authority = true,
      .integer_array_element_size_bytes = 8,
      .integer_array_element_count = 3,
      .size_bytes = 24,
      .align_bytes = 8,
      .initializer_elements =
          {
              null_pointer_value(),
              null_pointer_value(),
              null_pointer_value(),
          },
  });
  publish_prepared_object_data(prepared);

  const auto* object_data =
      prepare::find_prepared_global_object_data(prepared.object_data, link_name);
  if (object_data == nullptr) {
    return fail("expected implicit pointer-table object-data facts");
  }
  if (object_data->object_label != link_name ||
      object_data->object_label_text != "implicit_pointer_table" ||
      object_data->section_kind != prepare::PreparedObjectDataSectionKind::Bss ||
      object_data->object_byte_offset != 0 ||
      object_data->object_size_bytes != 24 ||
      object_data->align_bytes != 8 || !object_data->emitted_bytes.empty() ||
      object_data->zero_fill_byte_count != object_data->object_size_bytes ||
      !object_data->has_object_label ||
      !object_data->has_publication_identity ||
      !object_data->has_object_byte_range ||
      object_data->requires_emitted_bytes ||
      object_data->has_emitted_bytes ||
      !object_data->requires_zero_fill ||
      !object_data->has_zero_fill ||
      object_data->requires_unsupported_marker ||
      object_data->has_unsupported_marker ||
      object_data->unsupported_but_coherent) {
    return fail("expected full-extent zero-fill prepared object-data authority");
  }
  return 0;
}

int rejects_unsupported_selected_global_object_data_shapes() {
  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name = prepared.module.names.link_names.intern("rawptr");
    prepared.module.globals.push_back(bir::Global{
        .name = "rawptr",
        .link_name_id = link_name,
        .type = bir::TypeKind::Ptr,
        .size_bytes = 8,
        .align_bytes = 8,
        .initializer = nonnull_pointer_immediate(4096),
    });
    publish_prepared_object_data(prepared);

    if (expect_prepared_rejection_diagnostic(
            prepared,
            "unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=" +
                std::to_string(link_name) +
                " object_size_bytes=8 emitted_byte_count=0 zero_fill_byte_count=0") !=
        0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name = prepared.module.names.link_names.intern("tls_cursor");
    prepared.module.globals.push_back(bir::Global{
        .name = "tls_cursor",
        .link_name_id = link_name,
        .type = bir::TypeKind::Ptr,
        .is_thread_local = true,
        .size_bytes = 8,
        .align_bytes = 8,
        .initializer = null_pointer_value(),
    });
    publish_prepared_object_data(prepared);

    if (expect_prepared_rejection_diagnostic(
            prepared,
            "unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=" +
                std::to_string(link_name) +
                " object_size_bytes=8 emitted_byte_count=0 zero_fill_byte_count=0") !=
        0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name = prepared.module.names.link_names.intern("got_cursor");
    prepared.module.globals.push_back(bir::Global{
        .name = "got_cursor",
        .link_name_id = link_name,
        .type = bir::TypeKind::Ptr,
        .size_bytes = 8,
        .align_bytes = 8,
        .initializer = null_pointer_value(),
        .address_materialization_policy =
            bir::GlobalAddressMaterializationPolicy::GotRequired,
    });
    publish_prepared_object_data(prepared);

    if (expect_prepared_rejection_diagnostic(
            prepared,
            "unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent object_label_id=" +
                std::to_string(link_name) +
                " object_size_bytes=8 emitted_byte_count=0 zero_fill_byte_count=0") !=
        0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name = prepared.module.names.link_names.intern("no_extent");
    prepared.module.globals.push_back(bir::Global{
        .name = "no_extent",
        .link_name_id = link_name,
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .initializer = bir::Value::immediate_i32(7),
    });
    publish_prepared_object_data(prepared);
    prepared.object_data.globals.front().has_object_byte_range = false;

    if (expect_prepared_rejection_diagnostic_contains(
            prepared,
            {"unsupported_global_data:",
             "status=missing_object_byte_range",
             "object_label_id=" + std::to_string(link_name)}) != 0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    prepared.module.globals.push_back(bir::Global{
        .name = "missing_bytes",
        .link_name_id = prepared.module.names.link_names.intern("missing_bytes"),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .initializer = bir::Value::immediate_i32(7),
    });
    publish_prepared_object_data(prepared);
    auto& object_data = prepared.object_data.globals.front();
    object_data.requires_emitted_bytes = false;
    object_data.has_emitted_bytes = false;
    object_data.emitted_bytes.clear();

    if (expect_prepared_rejection_diagnostic_contains(
            prepared,
            {"unsupported_global_data:",
             "missing emitted-byte authority"}) != 0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    prepared.module.globals.push_back(bir::Global{
        .name = "missing_zero_fill",
        .link_name_id = prepared.module.names.link_names.intern("missing_zero_fill"),
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
    auto& object_data = prepared.object_data.globals.front();
    object_data.requires_zero_fill = false;
    object_data.has_zero_fill = false;
    object_data.zero_fill_byte_count = 0;

    if (expect_prepared_rejection_diagnostic_contains(
            prepared,
            {"unsupported_global_data:",
             "missing zero-fill authority"}) != 0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name = prepared.module.names.link_names.intern("needs_reloc");
    prepared.module.globals.push_back(bir::Global{
        .name = "needs_reloc",
        .link_name_id = link_name,
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .initializer = bir::Value::immediate_i32(7),
    });
    publish_prepared_object_data(prepared);
    auto& object_data = prepared.object_data.globals.front();
    object_data.requires_relocation = true;
    object_data.has_relocation = false;

    if (expect_prepared_rejection_diagnostic_contains(
            prepared,
            {"unsupported_global_data:",
             "status=missing_relocation",
             "object_label_id=" + std::to_string(link_name)}) != 0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name =
        prepared.module.names.link_names.intern("ambiguous_reloc");
    prepared.module.globals.push_back(bir::Global{
        .name = "ambiguous_reloc",
        .link_name_id = link_name,
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .initializer = bir::Value::immediate_i32(7),
    });
    publish_prepared_object_data(prepared);
    prepared.object_data.globals.front().conflicting_relocation = true;

    if (expect_prepared_rejection_diagnostic_contains(
            prepared,
            {"unsupported_global_data:",
             "status=conflicting_relocation",
             "object_label_id=" + std::to_string(link_name)}) != 0) {
      return 1;
    }
  }

  {
    auto prepared = make_prepared_direct_call_module();
    const auto link_name = prepared.module.names.link_names.intern("invalid_init");
    prepared.module.globals.push_back(bir::Global{
        .name = "invalid_init",
        .link_name_id = link_name,
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .initializer = bir::Value::immediate_i32(7),
    });
    publish_prepared_object_data(prepared);
    prepared.object_data.globals.front()
        .invalid_pre_prepared_initializer_semantics = true;

    if (expect_prepared_rejection_diagnostic_contains(
            prepared,
            {"unsupported_global_data:",
             "status=invalid_pre_prepared_initializer_semantics",
             "object_label_id=" + std::to_string(link_name)}) != 0) {
      return 1;
    }
  }

  return 0;
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

int emits_prepared_global_object_storage_from_prepared_record_authority() {
  auto prepared = make_prepared_direct_call_module();
  const auto link_name = prepared.module.names.link_names.intern("raw_counter");
  prepared.module.globals.push_back(bir::Global{
      .name = "raw_counter",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(7),
  });
  publish_prepared_object_data(prepared);

  auto& object_data = prepared.object_data.globals.front();
  object_data.object_label_text = "prepared_counter";
  object_data.section_kind = prepare::PreparedObjectDataSectionKind::ReadOnlyData;
  object_data.align_bytes = 16;
  object_data.emitted_bytes = {0xaa, 0xbb, 0xcc, 0xdd};
  object_data.public_symbol = false;

  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object path to emit from prepared record authority");
  }
  if (object::find_symbol(*module, "raw_counter") != nullptr) {
    return fail("expected raw global spelling not to define object storage");
  }
  const auto* rodata = object::find_section(*module, ".rodata");
  if (rodata == nullptr || rodata->writable || rodata->executable ||
      rodata->align_bytes != 16 ||
      rodata->bytes != std::vector<std::uint8_t>{0xaa, 0xbb, 0xcc, 0xdd}) {
    return fail("expected prepared section, alignment, and bytes to drive object data");
  }
  const auto* symbol = object::find_symbol(*module, "prepared_counter");
  if (symbol == nullptr ||
      symbol->binding != object::SymbolBinding::Local ||
      symbol->kind != object::SymbolKind::Object ||
      symbol->section != std::optional<object::SectionId>{rodata->id} ||
      symbol->value != 0 || symbol->size_bytes != 4) {
    return fail("expected prepared object label, visibility, and extent on symbol");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize prepared-authority object");
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

int emits_prepared_global_symbol_address_prior_preserved_arg_relocation() {
  const auto prepared =
      make_prepared_global_symbol_address_prior_preserved_call_module();
  const auto result =
      rv64::build_rv64_prepared_text_object_module_with_diagnostics(prepared);
  if (!result.module.has_value()) {
    return fail("expected prepared symbol-address prior-preserved call object to build, got `" +
                result.diagnostic + "`");
  }
  const auto& module = *result.module;
  const auto* text = object::find_section(module, ".text");
  const auto* main =
      object::find_symbol(module, "symbol_address_survives_call");
  const auto* global_symbol = object::find_symbol(module, "global_state");
  const auto* capture = object::find_symbol(module, "capture");
  const auto* consume = object::find_symbol(module, "consume");
  if (text == nullptr || main == nullptr || global_symbol == nullptr ||
      capture == nullptr || consume == nullptr) {
    return fail("expected symbol-address prior-preserved call object symbols");
  }

  std::optional<std::uint64_t> first_call_offset;
  std::optional<std::uint64_t> second_call_offset;
  bool saw_later_symbol_hi = false;
  bool saw_later_symbol_lo = false;
  for (const auto& relocation : module.relocations) {
    if (relocation.section != text->id) {
      continue;
    }
    if (relocation.type == R_RISCV_CALL_PLT &&
        relocation.symbol == capture->id) {
      first_call_offset = relocation.offset;
      continue;
    }
    if (relocation.type == R_RISCV_CALL_PLT &&
        relocation.symbol == consume->id) {
      second_call_offset = relocation.offset;
      continue;
    }
  }
  if (!first_call_offset.has_value() || !second_call_offset.has_value() ||
      *first_call_offset >= *second_call_offset ||
      *first_call_offset < main->value ||
      *second_call_offset >= main->value + main->size_bytes) {
    return fail("expected ordered calls around symbol-address preservation");
  }

  for (const auto& relocation : module.relocations) {
    if (relocation.section != text->id ||
        relocation.offset <= *first_call_offset ||
        relocation.offset >= *second_call_offset) {
      continue;
    }
    if (relocation.type == R_RISCV_PCREL_HI20 &&
        relocation.symbol == global_symbol->id) {
      saw_later_symbol_hi = true;
      const auto* auipc_label =
          object::find_symbol(module, ".Lpcrel_call_arg_symbol_address_survives_call_0_1_0");
      saw_later_symbol_lo =
          auipc_label != nullptr &&
          std::any_of(module.relocations.begin(),
                      module.relocations.end(),
                      [&](const object::RelocationRecord& lo) {
                        return lo.section == text->id &&
                               lo.offset == relocation.offset + 4 &&
                               lo.type == R_RISCV_PCREL_LO12_I &&
                               lo.symbol == auipc_label->id;
                      });
    }
  }
  if (!saw_later_symbol_hi || !saw_later_symbol_lo) {
    return fail("expected later call argument to rematerialize the global symbol address instead of trusting a prior callee-saved home");
  }
  if (read_u32(text->bytes, *second_call_offset - 4) == 0x00048513) {
    return fail("expected later call argument not to copy an uninitialized/stale s1 home into a0");
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

int emits_prepared_pointer_cast_gpr_movement_object() {
  const auto inttoptr_prepared = make_prepared_pointer_cast_module();
  const auto inttoptr_module =
      rv64::build_rv64_prepared_text_object_module(inttoptr_prepared);
  if (!inttoptr_module.has_value()) {
    return fail("expected prepared inttoptr GPR cast to build");
  }
  const auto* inttoptr_text = object::find_section(*inttoptr_module, ".text");
  if (inttoptr_text == nullptr || inttoptr_text->bytes.size() < 8) {
    return fail("expected inttoptr GPR copy and return text");
  }
  const auto inttoptr_copy = read_u32(inttoptr_text->bytes, 0);
  if ((inttoptr_copy & 0x7fU) != 0x13U ||
      ((inttoptr_copy >> 7) & 0x1fU) != 18U ||
      ((inttoptr_copy >> 12) & 0x7U) != 0U ||
      ((inttoptr_copy >> 15) & 0x1fU) != 5U ||
      ((inttoptr_copy >> 20) & 0xfffU) != 0U) {
    return fail("expected inttoptr to publish prepared GPR copy");
  }

  const auto ptrtoint_prepared = make_prepared_pointer_cast_module(
      bir::CastOpcode::PtrToInt,
      bir::TypeKind::Ptr,
      bir::TypeKind::I64);
  const auto ptrtoint_module =
      rv64::build_rv64_prepared_text_object_module(ptrtoint_prepared);
  if (!ptrtoint_module.has_value()) {
    return fail("expected prepared ptrtoint GPR cast to build");
  }
  const auto* ptrtoint_text = object::find_section(*ptrtoint_module, ".text");
  if (ptrtoint_text == nullptr || ptrtoint_text->bytes.size() < 8) {
    return fail("expected ptrtoint GPR copy and return text");
  }
  const auto ptrtoint_copy = read_u32(ptrtoint_text->bytes, 0);
  if ((ptrtoint_copy & 0x7fU) != 0x13U ||
      ((ptrtoint_copy >> 7) & 0x1fU) != 18U ||
      ((ptrtoint_copy >> 12) & 0x7U) != 0U ||
      ((ptrtoint_copy >> 15) & 0x1fU) != 5U ||
      ((ptrtoint_copy >> 20) & 0xfffU) != 0U) {
    return fail("expected ptrtoint to publish prepared GPR copy");
  }
  const auto stack_result_prepared = make_prepared_pointer_cast_module(
      bir::CastOpcode::PtrToInt,
      bir::TypeKind::Ptr,
      bir::TypeKind::I64,
      prepare::PreparedValueHomeKind::Register,
      prepare::PreparedValueHomeKind::StackSlot);
  const auto stack_result_module =
      rv64::build_rv64_prepared_text_object_module(stack_result_prepared);
  if (!stack_result_module.has_value()) {
    return fail("expected prepared ptrtoint stack-result cast to build");
  }
  const auto* stack_result_text = object::find_section(*stack_result_module, ".text");
  if (stack_result_text == nullptr || stack_result_text->bytes.size() < 12) {
    return fail("expected ptrtoint stack-result copy, store, and return text");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*ptrtoint_module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to serialize pointer cast object");
  }
  return 0;
}

int emits_prepared_pointer_cast_rematerialized_source_object() {
  const auto immediate_prepared = make_prepared_pointer_cast_module(
      bir::CastOpcode::IntToPtr,
      bir::TypeKind::I32,
      bir::TypeKind::Ptr,
      prepare::PreparedValueHomeKind::Register,
      prepare::PreparedValueHomeKind::Register,
      true);
  const auto immediate_module =
      rv64::build_rv64_prepared_text_object_module(immediate_prepared);
  if (!immediate_module.has_value()) {
    return fail("expected prepared literal inttoptr source to build");
  }
  const auto* immediate_text = object::find_section(*immediate_module, ".text");
  if (immediate_text == nullptr || immediate_text->bytes.size() < 8) {
    return fail("expected literal inttoptr materialization and return text");
  }
  const auto immediate_load = read_u32(immediate_text->bytes, 0);
  if ((immediate_load & 0x7fU) != 0x13U ||
      ((immediate_load >> 7) & 0x1fU) != 18U ||
      ((immediate_load >> 12) & 0x7U) != 0U ||
      ((immediate_load >> 15) & 0x1fU) != 0U ||
      ((immediate_load >> 20) & 0xfffU) != 12U) {
    return fail("expected literal inttoptr to materialize prepared result GPR");
  }

  const auto remat_prepared = make_prepared_pointer_cast_module(
      bir::CastOpcode::IntToPtr,
      bir::TypeKind::I32,
      bir::TypeKind::Ptr,
      prepare::PreparedValueHomeKind::RematerializableImmediate);
  const auto remat_module =
      rv64::build_rv64_prepared_text_object_module(remat_prepared);
  if (!remat_module.has_value()) {
    return fail("expected prepared rematerialized inttoptr source to build");
  }
  const auto* remat_text = object::find_section(*remat_module, ".text");
  if (remat_text == nullptr || remat_text->bytes.size() < 8) {
    return fail("expected rematerialized inttoptr materialization and return text");
  }
  const auto remat_load = read_u32(remat_text->bytes, 0);
  if ((remat_load & 0x7fU) != 0x13U ||
      ((remat_load >> 7) & 0x1fU) != 18U ||
      ((remat_load >> 12) & 0x7U) != 0U ||
      ((remat_load >> 15) & 0x1fU) != 0U ||
      ((remat_load >> 20) & 0xfffU) != 12U) {
    return fail("expected rematerialized inttoptr to materialize prepared GPR");
  }
  return 0;
}

int rejects_prepared_pointer_cast_fail_closed_shapes() {
  const std::string unsupported_instruction =
      "unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering";
  if (expect_prepared_rejection_diagnostic(
          make_prepared_pointer_cast_module(bir::CastOpcode::IntToPtr,
                                            bir::TypeKind::I64,
                                            bir::TypeKind::Ptr,
                                            prepare::PreparedValueHomeKind::Register,
                                            prepare::PreparedValueHomeKind::Register,
                                            false,
                                            true),
          "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes") !=
      0) {
    return 1;
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_pointer_cast_module(bir::CastOpcode::IntToPtr,
                                            bir::TypeKind::I64,
                                            bir::TypeKind::Ptr,
                                            prepare::PreparedValueHomeKind::Register,
                                            prepare::PreparedValueHomeKind::Register,
                                            false,
                                            false,
                                            true),
          unsupported_instruction) != 0) {
    return fail("missing result home pointer cast shape should reject");
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_pointer_cast_module(
              bir::CastOpcode::IntToPtr,
              bir::TypeKind::I64,
              bir::TypeKind::Ptr,
              prepare::PreparedValueHomeKind::StackSlot),
          "unsupported_param_home: RV64 object route requires all parameters in supported GPR or prepared FPR register homes") !=
      0) {
    return fail("stack source home pointer cast shape should reject");
  }
  if (expect_prepared_rejection_diagnostic(
          make_prepared_pointer_cast_module(
              bir::CastOpcode::IntToPtr,
              bir::TypeKind::I16,
              bir::TypeKind::Ptr),
          unsupported_instruction) != 0) {
    return fail("narrow inttoptr pointer cast shape should reject");
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
      text->bytes[0] != 0x13 || text->bytes[1] != 0x03 ||
      text->bytes[4] != 0x97 || text->bytes[5] != 0x02 ||
      text->bytes[8] != 0x93 || text->bytes[9] != 0x82 ||
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
      module->relocations[0].offset != 4 ||
      module->relocations[0].type != R_RISCV_PCREL_HI20 ||
      module->relocations[0].symbol != global_symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 8 ||
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
  status |= builds_prepared_fused_ne_ptr_register_compare_branch_object();
  status |= builds_prepared_fused_eq_ptr_register_compare_branch_object();
  status |= builds_prepared_fused_ult_ptr_register_compare_branch_object();
  status |= builds_prepared_fused_uge_ptr_register_compare_branch_object();
  status |= builds_prepared_fused_ugt_ptr_register_compare_branch_object();
  status |= builds_prepared_fused_ule_ptr_register_compare_branch_object();
  status |= rejects_prepared_fused_compare_branch_fail_closed_shapes();
  status |= builds_prepared_rematerialized_nonzero_return_object();
  status |= builds_prepared_traversed_wide_rematerialized_return_object();
  status |= rejects_prepared_rematerialized_return_without_typed_immediate_fact();
  status |=
      reports_generic_fallback_context_for_prepared_rematerialized_instruction();
  status |= builds_prepared_scalar_same_module_call_object();
  status |= builds_prepared_immediate_null_same_module_call_object();
  status |= rejects_prepared_scalar_register_result_call_fail_closed_shapes();
  status |= builds_prepared_byval_stack_copy_same_module_call_object();
  status |= rejects_prepared_byval_stack_copy_call_fail_closed_shapes();
  status |= builds_prepared_same_module_sret_call_object();
  status |= builds_representative_prepared_same_module_sret_call_object();
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
  status |= rejects_variadic_va_start_stack_backed_destination_address();
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
  status |= builds_prepared_prior_result_multi_gpr_same_module_call_object();
  status |= builds_prepared_prior_preserved_arg_call_object();
  status |=
      builds_prepared_ptrtoint_param_survives_nested_same_module_call_object();
  status |= rejects_prepared_prior_preserved_arg_call_fail_closed_shapes();
  status |= builds_byval_stack_slot_param_home_object();
  status |= rejects_byval_stack_slot_param_home_fail_closed_shapes();
  status |= builds_scalar_gpr_stack_slot_param_home_object();
  status |= rejects_scalar_gpr_stack_slot_param_home_fail_closed_shapes();
  status |= builds_stack_passed_scalar_param_home_object();
  status |= rejects_stack_passed_scalar_param_home_fail_closed_shapes();
  status |= rejects_byval_stack_slot_pointer_access_fail_closed_shapes();
  status |= builds_prepared_fpr_formal_param_home_with_target_identity_object();
  status |= rejects_raw_fpr_formal_param_home_without_target_identity();
  status |= records_prepared_fpr_callee_saved_frame_slot_facts();
  status |= rejects_malformed_prepared_fpr_callee_saved_frame_slot_facts();
  status |= records_prepared_gpr_callee_saved_frame_slot_facts();
  status |= rejects_malformed_prepared_gpr_callee_saved_frame_slot_facts();
  status |= materializes_large_offset_prepared_gpr_callee_saved_frame_slots();
  status |= materializes_prepared_fpr_callee_saved_frame_slots();
  status |= builds_prepared_scalar_local_frame_object();
  status |= builds_prepared_large_fixed_stack_frame_adjustment_object();
  status |= builds_prepared_large_fixed_slot_addressing_object();
  status |= builds_prepared_frame_slot_address_local_store_object();
  status |=
      builds_prepared_pointer_result_frame_slot_address_materialization_object();
  status |=
      rejects_prepared_pointer_result_frame_slot_address_fail_closed_shapes();
  status |= builds_prepared_f64_local_frame_object();
  status |= builds_prepared_f32_local_frame_object();
  status |= builds_prepared_f32_i32_local_overlay_object();
  status |= builds_prepared_scalar_local_subobject_frame_object();
  status |= rejects_prepared_f64_local_frame_fail_closed_shapes();
  status |= rejects_prepared_scalar_local_subobject_fail_closed_shapes();
  status |= builds_prepared_pointer_value_scalar_local_object();
  status |= builds_prepared_pointer_value_scalar_stack_home_local_object();
  status |= builds_prepared_pointer_value_scalar_local_store_with_t1_base_object();
  status |= builds_prepared_pointer_value_i8_local_store_object();
  status |= builds_prepared_pointer_value_f64_local_object();
  status |= builds_prepared_sret_stack_pointer_store_object();
  status |= rejects_prepared_sret_stack_pointer_store_fail_closed_shapes();
  status |= rejects_prepared_pointer_value_i8_local_store_fail_closed_shapes();
  status |= rejects_prepared_pointer_value_scalar_local_fail_closed_shapes();
  status |= builds_prepared_stack_slot_scalar_flow_object();
  status |= builds_prepared_stack_slot_to_gpr_move_bundle_object();
  status |= builds_prepared_before_return_stack_to_register_abi_move_object();
  status |= keeps_generic_return_load_for_mismatched_before_return_stack_move();
  status |= rejects_prepared_before_return_stack_to_register_abi_move_fail_closed_shapes();
  status |= builds_prepared_register_to_stack_before_instruction_move_bundle_object();
  status |= rejects_prepared_register_to_stack_move_bundle_fail_closed_shapes();
  status |=
      rejects_ambiguous_non_parallel_multi_source_stack_destination_move_bundle();
  status |= builds_prepared_stack_to_stack_before_instruction_move_bundle_object();
  status |= builds_prepared_mixed_stack_destination_move_bundle_object();
  status |=
      builds_explicit_prepared_stack_widening_authority_move_bundle_object();
  status |= rejects_non_integer_prepared_stack_widening_authority_shapes();
  status |= rejects_prepared_stack_to_stack_move_bundle_fail_closed_shapes();
  status |= rejects_prepared_stack_slot_to_gpr_move_bundle_fail_closed_shapes();
  status |= builds_prepared_out_of_ssa_phi_join_register_move_object();
  status |=
      builds_prepared_out_of_ssa_phi_join_immediate_materialization_object();
  status |=
      builds_prepared_out_of_ssa_phi_join_i64_zero_materialization_object();
  status |= builds_prepared_out_of_ssa_edge_preservation_register_move_object();
  status |= rejects_prepared_out_of_ssa_edge_preservation_fail_closed_shapes();
  status |= builds_prepared_out_of_ssa_edge_preservation_stack_move_object();
  status |= rejects_prepared_out_of_ssa_edge_preservation_stack_fail_closed_shapes();
  status |= rejects_prepared_out_of_ssa_phi_join_register_move_fail_closed_shapes();
  status |=
      rejects_prepared_out_of_ssa_phi_join_immediate_materialization_fail_closed_shapes();
  status |= reports_prepared_move_bundle_coordinate_diagnostic();
  status |= builds_prepared_scalar_compare_trunc_object();
  status |= builds_prepared_scalar_ordered_compare_return_object();
  status |= builds_prepared_scalar_ashr_register_object();
  status |= builds_prepared_scalar_ashr_immediate_object();
  status |= builds_prepared_scalar_ashr_i64_register_object();
  status |= builds_prepared_scalar_ashr_i64_immediate_object();
  status |= rejects_prepared_scalar_ashr_invalid_immediate_object();
  status |= reports_generic_fallback_context_for_prepared_traversal_instruction();
  status |= builds_prepared_scalar_divrem_object();
  status |= builds_prepared_scalar_f64_binary_object();
  status |= builds_prepared_scalar_f32_binary_object();
  status |= rejects_prepared_scalar_fp_binary_fail_closed_shapes();
  status |= rejects_prepared_scalar_division_fail_closed_shapes();
  status |= rejects_prepared_scalar_remainder_fail_closed_shapes();
  status |= rejects_prepared_pointer_arithmetic_with_precise_diagnostic();
  status |= rejects_prepared_scalar_compare_publication_missing_home();
  status |= builds_prepared_f32_scalar_compare_publication_object();
  status |= builds_prepared_f32_scalar_compare_zero_publication_object();
  status |= builds_prepared_f64_scalar_compare_select_consumer_publication_object();
  status |= builds_prepared_join_transfer_select_materialization_object();
  status |= builds_prepared_normalized_sle_select_materialization_object();
  status |= builds_prepared_small_integer_ordinary_select_materialization_objects();
  status |=
      materializes_nested_i32_ordinary_select_without_intermediate_home_object();
  status |=
      materializes_tree_i32_ordinary_select_without_intermediate_home_object();
  status |=
      rejects_reused_nested_i32_ordinary_select_without_intermediate_home_object();
  status |= skips_published_prepared_join_transfer_select_carrier_object();
  status |= materializes_published_prepared_join_transfer_select_stack_carrier_object();
  status |= materializes_published_prepared_join_transfer_select_edge_compare_source_object();
  status |=
      materializes_published_prepared_join_transfer_select_dependent_edge_compare_source_object();
  status |=
      materializes_published_prepared_join_transfer_select_cast_dependency_source_object();
  status |=
      materializes_carrier_authorized_prepared_join_transfer_select_ule_source_object();
  status |=
      materializes_carrier_alias_prepared_join_transfer_select_ne_source_object();
  status |=
      materializes_predecessor_compare_publication_select_carrier_object();
  status |=
      materializes_only_carrier_use_prepared_join_transfer_select_ne_source_object();
  status |=
      suppresses_authorized_prepared_select_edge_source_producer_setup_object();
  status |= rejects_prepared_join_transfer_select_cast_dependency_fail_closed_shapes();
  status |= rejects_prepared_join_transfer_select_carrier_alias_ule_fail_closed_shapes();
  status |=
      keeps_unauthorized_prepared_select_edge_source_producer_suppression_fail_closed();
  status |= rejects_published_prepared_join_transfer_select_ambiguous_publications_object();
  status |= publishes_select_publication_stack_home_move_intent_fields();
  status |= builds_prepared_local_register_arg_call_object();
  status |= builds_prepared_frame_slot_value_arg_call_object();
  status |=
      builds_prepared_frame_slot_value_and_prior_preserved_arg_call_object();
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
  status |= builds_prepared_empty_tied_scalar_gpr_inline_asm_object();
  status |= builds_prepared_no_result_memory_clobber_inline_asm_object();
  status |= rejects_prepared_no_result_memory_clobber_inline_asm_fail_closed_shapes();
  status |= rejects_prepared_symbol_address_imr_inline_asm_with_precise_diagnostic();
  status |= rejects_prepared_empty_tied_scalar_gpr_inline_asm_fail_closed_shapes();
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
  status |= rejects_prepared_f64_global_load_without_prepared_access();
  status |= emits_prepared_f64_global_load_from_explicit_facts();
  status |= emits_prepared_global_aggregate_lane_load_from_explicit_facts();
  status |= rejects_raw_load_local_global_address_lane_without_prepared_access();
  status |= builds_prepared_i16_local_store_object();
  status |= builds_prepared_fpr_fpext_object();
  status |= builds_prepared_fpr_fptrunc_object();
  status |= builds_prepared_formal_fpr_fpext_to_ft0_object();
  status |= builds_prepared_sitofp_i32_immediate_to_f64_object();
  status |= builds_prepared_uitofp_i32_to_f32_then_fpext_object();
  status |= builds_prepared_fp_to_int_casts_with_rtz_rounding_object();
  status |= rejects_prepared_fp_to_int_cast_fail_closed_shapes();
  status |= builds_prepared_f64_immediate_fptrunc_object();
  status |= builds_prepared_before_return_fpr_f32_abi_move_object();
  status |= builds_prepared_before_return_fpr_f64_abi_move_object();
  status |= rejects_prepared_before_return_fpr_abi_move_fail_closed_shapes();
  status |= builds_prepared_direct_global_return_authority_object();
  status |= rejects_prepared_direct_global_return_authority_fail_closed_shapes();
  status |= builds_prepared_fpr_immediate_return_objects();
  status |= rejects_prepared_fpr_immediate_return_fail_closed_shapes();
  status |= rejects_unsupported_prepared_floating_cast_with_precise_diagnostic();
  status |= emits_prepared_selected_symbol_pointer_global_object_storage();
  status |= emits_prepared_selected_zero_pointer_global_bss_storage();
  status |= publishes_implicit_const_pointer_array_zero_fill_object_data_facts();
  status |= rejects_unsupported_selected_global_object_data_shapes();
  status |= emits_prepared_writable_i32_global_object_storage();
  status |= emits_prepared_global_object_storage_from_prepared_record_authority();
  status |= rejects_prepared_global_object_storage_without_prepared_data_facts();
  status |=
      rejects_prepared_global_object_storage_incoherent_prepared_data_facts();
  status |= emits_prepared_linear_i8_global_object_storage();
  status |= emits_prepared_zero_global_bss_storage();
  status |= emits_prepared_constant_f64_global_object_storage();
  status |= emits_prepared_string_address_relocations_to_object_symbol();
  status |= emits_prepared_string_call_argument_relocation_to_object_symbol();
  status |=
      emits_prepared_global_symbol_address_prior_preserved_arg_relocation();
  status |= emits_prepared_global_address_relocations_to_object_symbol();
  status |= emits_prepared_global_load_relocations_and_instruction();
  status |= emits_prepared_global_i8_load_and_zext_instruction();
  status |= emits_prepared_same_width_i32_zext_gpr_copy();
  status |= rejects_prepared_same_width_zext_fail_closed_shapes();
  status |= emits_prepared_pointer_cast_gpr_movement_object();
  status |= emits_prepared_pointer_cast_rematerialized_source_object();
  status |= rejects_prepared_pointer_cast_fail_closed_shapes();
  status |= emits_prepared_global_store_relocations_and_instruction();
  status |= emits_prepared_global_i16_store_instruction();
  status |= serializes_rv64_relocatable_elf_contract();
  status |= serializes_pcrel_hi_lo_relocations_with_auipc_label_symbol();
  status |= builds_prepared_runtime_abort_external_call_object();
  status |= builds_prepared_runtime_exit_external_call_object();
  status |= emits_prepared_runtime_external_module_text_policy();
  status |= rejects_prepared_runtime_external_module_text_policy_fail_closed_shapes();
  status |= writes_prepared_rv64_relocatable_elf_object_file();
  return status;
}
