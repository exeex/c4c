#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
#include "src/backend/prealloc/prealloc.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

c4c::TargetProfile x86_target_profile() {
  return c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
}

prepare::PreparedBirModule prepare_module(bir::Module module) {
  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = x86_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedControlFlowFunction* find_control_flow_function(prepare::PreparedBirModule& prepared,
                                                                 const char* function_name) {
  for (auto& function : prepared.control_flow.functions) {
    if (prepare::prepared_function_name(prepared.names, function.function_name) == function_name) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule legalize_short_circuit_or_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "short_circuit_or_prepare_contract";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.u.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.addr",
      .value = bir::Value::immediate_i32(1),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t1.addr",
      .value = bir::Value::immediate_i32(3),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .slot_name = "%t3.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .true_label = "logic.skip.8",
      .false_label = "logic.rhs.7",
  };

  bir::Block logic_rhs;
  logic_rhs.label = "logic.rhs.7";
  logic_rhs.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t12"),
      .slot_name = "%t12.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  logic_rhs.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t13"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t12"),
      .rhs = bir::Value::immediate_i32(3),
  });
  logic_rhs.terminator = bir::BranchTerminator{.target_label = "logic.rhs.end.9"};

  bir::Block logic_rhs_end;
  logic_rhs_end.label = "logic.rhs.end.9";
  logic_rhs_end.terminator = bir::BranchTerminator{.target_label = "logic.end.10"};

  bir::Block logic_skip;
  logic_skip.label = "logic.skip.8";
  logic_skip.terminator = bir::BranchTerminator{.target_label = "logic.end.10"};

  bir::Block logic_end;
  logic_end.label = "logic.end.10";
  logic_end.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t17"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::immediate_i32(3),
      .true_value = bir::Value::immediate_i32(1),
      .false_value = bir::Value::named(bir::TypeKind::I32, "%t13"),
  });
  logic_end.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t18"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t17"),
      .rhs = bir::Value::immediate_i32(0),
  });
  logic_end.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t18"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks = {
      std::move(entry),
      std::move(logic_rhs),
      std::move(logic_rhs_end),
      std::move(logic_skip),
      std::move(logic_end),
      std::move(block_1),
      std::move(block_2),
  };
  module.functions.push_back(std::move(function));

  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_plain_route_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "plain_route_miss";
  function.return_type = bir::TypeKind::F32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_f32_bits(0)};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_missing_short_circuit_contract_module() {
  auto prepared = legalize_short_circuit_or_guard_module();
  auto* control_flow = find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow != nullptr) {
    control_flow->branch_conditions.clear();
  }
  return prepared;
}

bool expect_contains(const std::string& text,
                     const std::string& needle,
                     const char* description) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] missing " << description << ": " << needle << "\n";
  std::cerr << "--- text ---\n" << text << "\n";
  return false;
}

}  // namespace

int main() {
  const auto prepared = legalize_short_circuit_or_guard_module();
  const auto plain_miss = legalize_plain_route_miss_module();
  const auto missing_short_circuit_contract = legalize_missing_short_circuit_contract_module();
  const std::string summary = c4c::backend::x86::summarize_prepared_module_routes(prepared);
  const std::string trace = c4c::backend::x86::trace_prepared_module_routes(prepared);
  const std::string plain_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(plain_miss);
  const std::string plain_miss_trace = c4c::backend::x86::trace_prepared_module_routes(plain_miss);
  const std::string missing_short_circuit_contract_summary =
      c4c::backend::x86::summarize_prepared_module_routes(missing_short_circuit_contract);
  const std::string missing_short_circuit_contract_trace =
      c4c::backend::x86::trace_prepared_module_routes(missing_short_circuit_contract);

  if (!expect_contains(summary, "x86 handoff summary", "summary header") ||
      !expect_contains(summary, "function short_circuit_or_prepare_contract", "function name") ||
      !expect_contains(summary, "- top-level lane:", "summary lane line") ||
      !expect_contains(trace, "x86 handoff trace", "trace header") ||
      !expect_contains(trace, "try lane local-slot-guard-chain", "trace lane") ||
      !expect_contains(trace, "result: ", "trace result") ||
      !expect_contains(plain_miss_summary,
                       "- final rejection: current x86 lanes did not recognize this prepared function shape",
                       "plain miss summary final rejection") ||
      !expect_contains(plain_miss_summary,
                       "- next inspect: inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane",
                       "plain miss summary next inspect") ||
      !expect_contains(plain_miss_trace,
                       "final: rejected: current x86 lanes did not recognize this prepared function shape",
                       "plain miss trace final rejection") ||
      !expect_contains(plain_miss_trace,
                       "next inspect: inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane",
                       "plain miss trace next inspect") ||
      !expect_contains(missing_short_circuit_contract_summary,
                       "- final rejection: local-slot-guard-chain is missing prepared handoff data required by the current x86 route",
                       "missing contract summary final rejection") ||
      !expect_contains(missing_short_circuit_contract_trace,
                       "final detail: x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff",
                       "missing contract trace detail") ||
      !expect_contains(missing_short_circuit_contract_trace,
                       "next inspect: inspect the prepared control-flow handoff consumed in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "missing contract trace next inspect")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
