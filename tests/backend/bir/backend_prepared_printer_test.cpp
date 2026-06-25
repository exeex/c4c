#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/backend/mir/x86/x86.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

c4c::TargetProfile riscv_target_profile() {
  return c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");
}

c4c::TargetProfile aarch64_target_profile() {
  return c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
}

void set_register_group_override(prepare::PreparedBirModule& prepared,
                                 std::string_view function_name,
                                 std::string_view value_name,
                                 prepare::PreparedRegisterClass register_class,
                                 std::size_t contiguous_width) {
  const auto function_id = prepared.names.function_names.find(function_name);
  const auto value_id = prepared.names.value_names.find(value_name);
  if (function_id == c4c::kInvalidFunctionName || value_id == c4c::kInvalidValueName) {
    return;
  }
  prepared.register_group_overrides.values.push_back(prepare::PreparedRegisterGroupOverride{
      .function_name = function_id,
      .value_name = value_id,
      .register_class = register_class,
      .contiguous_width = contiguous_width,
  });
}

prepare::PreparedBirModule legalize_short_circuit_or_guard_module() {
  bir::Module module;

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

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule legalize_loop_countdown_module() {
  bir::Module module;

  bir::Function function;
  function.name = "loop_countdown_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "counter"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(3),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "next"),
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .true_label = "body",
      .false_label = "exit",
  };

  bir::Block body;
  body.label = "body";
  body.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "next"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(1),
  });
  body.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "counter"),
  };

  function.blocks = {
      std::move(entry),
      std::move(loop),
      std::move(body),
      std::move(exit),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule legalize_parallel_copy_cycle_module() {
  bir::Module module;

  bir::Function function;
  function.name = "parallel_copy_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "a"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(1),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "b"),
          },
      },
  });
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "b"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(2),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "a"),
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "a"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .true_label = "body",
      .false_label = "exit",
  };

  bir::Block body;
  body.label = "body";
  body.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "a"),
  };

  function.blocks = {
      std::move(entry),
      std::move(loop),
      std::move(body),
      std::move(exit),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

enum class BlockEntryPublicationRouteEvidence {
  None,
  Agreeing,
  MissingPhi,
  WrongDestination,
  WrongSuccessorBlock,
  WrongInstructionIndex,
  Duplicate,
};

prepare::PreparedBirModule prepared_block_entry_publication_printer_row_module(
    BlockEntryPublicationRouteEvidence route_evidence =
        BlockEntryPublicationRouteEvidence::None) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();

  const auto function_name =
      prepared.names.function_names.intern("block_entry_publication_dump_contract");
  const auto successor_label = prepared.names.block_labels.intern("join");
  const auto value_name = prepared.names.value_names.intern("published");

  prepare::PreparedValueLocationFunction function_locations;
  function_locations.function_name = function_name;
  function_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 42,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"r9"},
  });

  prepare::PreparedMoveBundle bundle;
  bundle.function_name = function_name;
  bundle.phase = prepare::PreparedMovePhase::BlockEntry;
  bundle.authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy;
  bundle.block_index = 3;
  bundle.instruction_index = 5;
  bundle.source_parallel_copy_successor_label = successor_label;
  bundle.moves.push_back(prepare::PreparedMoveResolution{
      .to_value_id = 42,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = std::string{"r9"},
      .block_index = 3,
      .instruction_index = 5,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .source_parallel_copy_successor_label = successor_label,
  });
  function_locations.move_bundles.push_back(std::move(bundle));

  prepared.value_locations.functions.push_back(std::move(function_locations));

  if (route_evidence != BlockEntryPublicationRouteEvidence::None) {
    bir::Function function;
    function.name = "block_entry_publication_dump_contract";

    bir::Block join;
    if (route_evidence ==
        BlockEntryPublicationRouteEvidence::WrongSuccessorBlock) {
      join.label = "not_join";
      join.label_id = prepared.names.block_labels.intern("not_join");
    } else {
      join.label = "join";
      join.label_id = successor_label;
    }

    auto append_phi = [&](std::string name) {
      join.insts.push_back(bir::PhiInst{
          .result = bir::Value::named(bir::TypeKind::I32, std::move(name)),
          .incomings = {
              bir::PhiIncoming{
                  .label = "entry",
                  .value = bir::Value::named(bir::TypeKind::I32, "source"),
              },
          },
      });
    };

    if (route_evidence != BlockEntryPublicationRouteEvidence::MissingPhi) {
      if (route_evidence ==
          BlockEntryPublicationRouteEvidence::WrongInstructionIndex) {
        append_phi("published");
      } else {
        append_phi("route4.pad0");
        append_phi("route4.pad1");
        append_phi("route4.pad2");
        append_phi("route4.pad3");
        append_phi("route4.pad4");
        append_phi(route_evidence ==
                           BlockEntryPublicationRouteEvidence::WrongDestination
                       ? "not_published"
                       : "published");
        if (route_evidence == BlockEntryPublicationRouteEvidence::Duplicate) {
          append_phi("published");
        }
      }
    }

    function.blocks.push_back(std::move(join));
    prepared.module.functions.push_back(std::move(function));
  }

  return prepared;
}

prepare::PreparedCurrentBlockEntryPublication
find_block_entry_publication_printer_row_agreement(
    const prepare::PreparedBirModule& prepared) {
  const auto function_name =
      prepared.names.function_names.find("block_entry_publication_dump_contract");
  const auto successor_label = prepared.names.block_labels.find("join");
  const auto* locations =
      prepare::find_prepared_value_location_function(prepared, function_name);
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(locations);

  const bir::Block* successor_block = nullptr;
  const bir::Value* destination_value = nullptr;
  for (const auto& function : prepared.module.functions) {
    if (function.name != "block_entry_publication_dump_contract") {
      continue;
    }
    for (const auto& block : function.blocks) {
      if (block.label_id != successor_label && block.label != "join") {
        continue;
      }
      successor_block = &block;
      for (const auto& inst : block.insts) {
        const auto* phi = std::get_if<bir::PhiInst>(&inst);
        if (phi == nullptr) {
          break;
        }
        if (phi->result.name == "published") {
          destination_value = &phi->result;
          break;
        }
      }
      break;
    }
    break;
  }

  return prepare::find_prepared_current_block_entry_publication(
      prepare::PreparedCurrentBlockEntryPublicationQueryInputs{
          .names = &prepared.names,
          .value_locations = locations,
          .value_home_lookups = &value_home_lookups,
          .successor_label = successor_label,
          .route4_successor_block = successor_block,
          .route4_destination_value = destination_value,
      },
      prepare::PreparedValueId{42});
}

prepare::PreparedBirModule legalize_critical_edge_parallel_copy_module() {
  bir::Module module;

  bir::Function function;
  function.name = "critical_edge_parallel_copy_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(0),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .true_label = "left",
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .true_label = "join",
      .false_label = "exit",
  };

  bir::Block right;
  right.label = "right";
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(-1)};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{.label = "left", .value = bir::Value::immediate_i32(11)},
          bir::PhiIncoming{.label = "right", .value = bir::Value::immediate_i32(22)},
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks = {
      std::move(entry),
      std::move(left),
      std::move(right),
      std::move(exit),
      std::move(join),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_call_wrapper_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  const c4c::LinkNameId same_module_link_name_id =
      module.names.link_names.intern("same_module_i32");

  bir::Function same_module_callee;
  same_module_callee.name = "same_module_i32";
  same_module_callee.link_name_id = same_module_link_name_id;
  same_module_callee.return_type = bir::TypeKind::I32;
  same_module_callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  bir::Block same_module_entry;
  same_module_entry.label = "entry";
  same_module_entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "value")};
  same_module_callee.blocks.push_back(std::move(same_module_entry));
  module.functions.push_back(std::move(same_module_callee));

  bir::Function fixed_extern;
  fixed_extern.name = "extern_fixed_i32";
  fixed_extern.is_declaration = true;
  fixed_extern.return_type = bir::TypeKind::I32;
  fixed_extern.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(fixed_extern));

  bir::Function variadic_extern;
  variadic_extern.name = "extern_variadic_i32";
  variadic_extern.is_declaration = true;
  variadic_extern.is_variadic = true;
  variadic_extern.return_type = bir::TypeKind::I32;
  variadic_extern.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "head",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(variadic_extern));

  bir::Function variadic_entry;
  variadic_entry.name = "variadic_entry_dump_contract";
  variadic_entry.is_variadic = true;
  variadic_entry.return_type = bir::TypeKind::I32;
  variadic_entry.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "head",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  variadic_entry.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "scale",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });
  bir::Block variadic_entry_block;
  variadic_entry_block.label = "entry";
  variadic_entry_block.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  variadic_entry.blocks.push_back(std::move(variadic_entry_block));
  module.functions.push_back(std::move(variadic_entry));

  bir::Function caller;
  caller.name = "call_wrapper_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "callee.ptr",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.same_module"),
      .callee_link_name_id = same_module_link_name_id,
      .args = {bir::Value::immediate_i32(1)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.extern_fixed"),
      .callee = "extern_fixed_i32",
      .args = {bir::Value::immediate_i32(2)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.extern_variadic"),
      .callee = "extern_variadic_i32",
      .args = {bir::Value::immediate_i32(3), bir::Value::immediate_f32_bits(0x40800000)},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::F32},
      .arg_abi = {
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::I32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::F32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Sse,
              .passed_in_register = true,
          },
      },
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .is_variadic = true,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.indirect_wrapper"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "callee.ptr"),
      .args = {bir::Value::immediate_i32(5)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .is_indirect = true,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "tmp.indirect_wrapper")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_aapcs64_variadic_entry_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "aapcs64_variadic_entry_dump_contract";
  function.is_variadic = true;
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "head",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "scale",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_start.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      options);
}

prepare::PreparedBirModule prepare_aapcs64_variadic_entry_helper_family_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "aapcs64_variadic_entry_helper_family_dump_contract";
  function.is_variadic = true;
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "head",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "scale",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "aggregate.byval",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "next.hfa.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_start.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "next.i32"),
      .callee = "llvm.va_arg.i32",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::F64, "next.f64"),
      .callee = "llvm.va_arg.f64",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "double",
      .return_type = bir::TypeKind::F64,
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_arg.aggregate",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "next.aggregate"),
               bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi =
          {bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 8,
               .align_bytes = 4,
               .primary_class = bir::AbiValueClass::Memory,
               .sret_pointer = true,
           },
           bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 8,
               .align_bytes = 8,
               .primary_class = bir::AbiValueClass::Integer,
               .passed_in_register = true,
           }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_arg.aggregate",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "next.hfa"),
               bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi =
          {bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 4,
               .align_bytes = 4,
               .primary_class = bir::AbiValueClass::Memory,
               .sret_pointer = true,
           },
           bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 8,
               .align_bytes = 8,
               .primary_class = bir::AbiValueClass::Integer,
               .passed_in_register = true,
           }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      },
      .va_arg_hfa_lane_count = 1,
      .va_arg_hfa_lane_size_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "next.hfa.lane0"),
      .slot_name = "next.hfa.0",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "next.hfa.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "missing.aggregate"),
      .callee = "llvm.va_arg.aggregate",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_copy.p0.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "dst.ap"),
               bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      options);
}

prepare::PreparedBirModule prepare_memory_return_call_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_make_pair";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "ret.sret",
      .size_bytes = 8,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      },
      .is_sret = true,
  });
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "seed",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "memory_return_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.call.sret.storage",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "extern_make_pair",
      .args = {
          bir::Value::named(bir::TypeKind::Ptr, "lv.call.sret.storage"),
          bir::Value::immediate_i32(13),
      },
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .arg_abi = {
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Memory,
              .sret_pointer = true,
          },
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::I32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
      },
      .return_type_name = "pair",
      .return_type = bir::TypeKind::Void,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Void,
          .primary_class = bir::AbiValueClass::Memory,
          .returned_in_memory = true,
      },
      .sret_storage_name = "lv.call.sret.storage",
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_call_argument_source_shape_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  const c4c::LinkNameId extern_data_id = module.names.link_names.intern("extern_data");
  module.globals.push_back(bir::Global{
      .name = "extern_data",
      .link_name_id = extern_data_id,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Function callee;
  callee.name = "extern_consume_ptr_pair";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::I32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "lhs",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "rhs",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "call_argument_source_shape_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "base.ptr",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.scratch",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "base.ptr"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
      .slot_name = "lv.ptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "touch"),
      .slot_name = "lv.scratch",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "derived.seed"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "derived.seed"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "arg.ptr"),
      .slot_name = "lv.ptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.call"),
      .callee = "extern_consume_ptr_pair",
      .args = {
          bir::Value::named_symbol_pointer("", extern_data_id),
          bir::Value::named(bir::TypeKind::Ptr, "arg.ptr"),
      },
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi = {
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 8,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 8,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
      },
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "tmp.call")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_select_chain_direct_global_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  const c4c::LinkNameId extern_data_id =
      module.names.link_names.intern("extern_select_source");
  module.globals.push_back(bir::Global{
      .name = "extern_select_source",
      .link_name_id = extern_data_id,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Function callee;
  callee.name = "extern_consume_select_i32";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::I32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "select_chain_direct_global_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.selected",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.global"),
      .global_name = "extern_select_source",
      .global_name_id = extern_data_id,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "selected.arg"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = bir::Value::named(bir::TypeKind::I32, "loaded.global"),
      .false_value = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.selected",
      .value = bir::Value::named(bir::TypeKind::I32, "selected.arg"),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.result"),
      .callee = "extern_consume_select_i32",
      .args = {bir::Value::named(bir::TypeKind::I32, "selected.arg")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "call.result")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_cross_call_preservation_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "boundary_helper";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::I32;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "cross_call_preservation_dump_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "pre.only"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(3),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.out"),
      .callee = "boundary_helper",
      .args = {bir::Value::named(bir::TypeKind::I32, "pre.only")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.out.2"),
      .callee = "boundary_helper",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "after"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "call.out"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "call.out.2"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "after")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(module, riscv_target_profile(), options);
}

prepare::PreparedBirModule prepare_stack_cross_call_preservation_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "stack_boundary_helper";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::I32;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "stack_cross_call_preservation_dump_contract";
  function.return_type = bir::TypeKind::I32;
  constexpr int carry_count = 16;
  for (int index = 0; index < carry_count; ++index) {
    function.params.push_back(bir::Param{
        .type = bir::TypeKind::I32,
        .name = "p" + std::to_string(index),
        .size_bytes = 4,
        .align_bytes = 4,
        .abi = bir::CallArgAbiInfo{
            .type = bir::TypeKind::I32,
            .size_bytes = 4,
            .align_bytes = 4,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = true,
        },
    });
  }

  bir::Block entry;
  entry.label = "entry";
  for (int index = 0; index < carry_count; ++index) {
    entry.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "carry.stack." + std::to_string(index)),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "p" + std::to_string(index)),
        .rhs = bir::Value::immediate_i32(index + 2),
    });
  }
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.stack.out"),
      .callee = "stack_boundary_helper",
      .args = {bir::Value::immediate_i32(17)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "stack.sum.0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "call.stack.out"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "carry.stack.0"),
  });
  for (int index = 1; index < carry_count; ++index) {
    entry.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "stack.sum." + std::to_string(index)),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "stack.sum." + std::to_string(index - 1)),
        .rhs = bir::Value::named(bir::TypeKind::I32, "carry.stack." + std::to_string(index)),
    });
  }
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "stack.sum." + std::to_string(carry_count - 1))};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_grouped_cross_call_preservation_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "vec_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "grouped_cross_call_preservation_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.vcarry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.local",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.vcarry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.arg"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "vec_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "post.local"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "out"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "post.local"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "out")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "grouped_cross_call_preservation_dump_contract",
                              "carry.pre",
                              prepare::PreparedRegisterClass::Vector,
                              2);
  set_register_group_override(prepared,
                              "grouped_cross_call_preservation_dump_contract",
                              "post.local",
                              prepare::PreparedRegisterClass::Vector,
                              1);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule manual_stack_prior_preservation_source_selection_dump_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name =
      prepared.names.function_names.intern("manual_stack_prior_selection_dump_contract");
  const auto value_name = prepared.names.value_names.intern("stack.saved.arg");

  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 8,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .direct_callee_name = std::string{"manual_stack_sink"},
                  .outgoing_stack_argument_area =
                      prepare::PreparedOutgoingStackArgumentArea{.size_bytes = 32},
                  .arguments =
                      {
                          prepare::PreparedCallArgumentPlan{
                              .arg_index = 0,
                              .value_bank = prepare::PreparedRegisterBank::Gpr,
                              .source_encoding = prepare::PreparedStorageEncodingKind::Register,
                              .source_value_id = prepare::PreparedValueId{77},
                              .destination_stack_offset_bytes = std::size_t{8},
                              .destination_stack_size_bytes = std::size_t{8},
                              .source_selection =
                                  prepare::PreparedCallArgumentSourceSelection{
                                      .kind = prepare::PreparedCallArgumentSourceSelectionKind::
                                          PriorPreservation,
                                      .source_value_id = prepare::PreparedValueId{77},
                                      .source_value_name = value_name,
                                      .preserved_call_block_index = std::size_t{0},
                                      .preserved_call_instruction_index = std::size_t{5},
                                      .preservation_route =
                                          prepare::PreparedCallPreservationRoute::StackSlot,
                                      .preserved_stack_slot_id =
                                          prepare::PreparedFrameSlotId{9},
                                      .preserved_stack_offset_bytes = std::size_t{64},
                                      .preserved_stack_size_bytes = std::size_t{4},
                                      .preserved_stack_align_bytes = std::size_t{4},
                                  },
                          },
                      },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule prepare_grouped_spill_reload_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "vec_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "grouped_spill_reload_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.carry"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "seed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(99),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "vec_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "local0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(6),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "local0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "merge0"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "merge")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "grouped_spill_reload_dump_contract",
                              "carry",
                              prepare::PreparedRegisterClass::Vector,
                              16);
  set_register_group_override(prepared,
                              "grouped_spill_reload_dump_contract",
                              "local0",
                              prepare::PreparedRegisterClass::Vector,
                              16);
  set_register_group_override(prepared,
                              "grouped_spill_reload_dump_contract",
                              "hot",
                              prepare::PreparedRegisterClass::Vector,
                              16);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_general_grouped_spill_reload_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "spill_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "general_grouped_spill_reload_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.seed",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.carry"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.seed"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "spill_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix5"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix6"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix5"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix6"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "merge")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "general_grouped_spill_reload_dump_contract",
                              "carry",
                              prepare::PreparedRegisterClass::General,
                              2);
  set_register_group_override(prepared,
                              "general_grouped_spill_reload_dump_contract",
                              "hot",
                              prepare::PreparedRegisterClass::General,
                              2);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_float_grouped_spill_reload_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "spill_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "float_grouped_spill_reload_dump_contract";
  function.return_type = bir::TypeKind::F32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.seed",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "carry"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.carry"),
      .rhs = bir::Value::immediate_f32_bits(0x3f800000U),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.seed"),
      .rhs = bir::Value::immediate_f32_bits(0x40000000U),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "spill_sink",
      .args = {bir::Value::named(bir::TypeKind::F32, "p.arg")},
      .arg_types = {bir::TypeKind::F32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix0"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot"),
      .rhs = bir::Value::immediate_f32_bits(0x40e00000U),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix1"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix2"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix3"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix4"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix5"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix4"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix6"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix5"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "merge"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot.mix6"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::F32, "merge")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "float_grouped_spill_reload_dump_contract",
                              "carry",
                              prepare::PreparedRegisterClass::Float,
                              2);
  set_register_group_override(prepared,
                              "float_grouped_spill_reload_dump_contract",
                              "hot",
                              prepare::PreparedRegisterClass::Float,
                              2);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

const prepare::PreparedRegallocValue* find_regalloc_value(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId value_id) {
  for (const auto& value : function.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule prepare_stack_argument_slot_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_take_byval";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "pair",
      .size_bytes = 8,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
      .is_byval = true,
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "stack_argument_slot_dump_contract";
  caller.return_type = bir::TypeKind::Void;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.byval",
      .size_bytes = 8,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
      .is_byval = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "extern_take_byval",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "p.byval")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_stack_result_slot_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_spill_i32";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::I32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "stack_result_slot_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  for (int index = 0; index < 8; ++index) {
    caller.params.push_back(bir::Param{
        .type = bir::TypeKind::I32,
        .name = "p" + std::to_string(index),
        .size_bytes = 4,
        .align_bytes = 4,
        .abi = bir::CallArgAbiInfo{
            .type = bir::TypeKind::I32,
            .size_bytes = 4,
            .align_bytes = 4,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = true,
        },
    });
  }

  bir::Block entry;
  entry.label = "entry";
  for (int index = 0; index < 8; ++index) {
    entry.insts.push_back(bir::CallInst{
        .result = bir::Value::named(bir::TypeKind::I32, "tmp.call." + std::to_string(index)),
        .callee = "extern_spill_i32",
        .args = {bir::Value::named(bir::TypeKind::I32, "p" + std::to_string(index))},
        .arg_types = {bir::TypeKind::I32},
        .arg_abi = {bir::CallArgAbiInfo{
            .type = bir::TypeKind::I32,
            .size_bytes = 4,
            .align_bytes = 4,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = true,
        }},
        .return_type_name = "i32",
        .return_type = bir::TypeKind::I32,
        .result_abi = bir::CallResultAbiInfo{
            .type = bir::TypeKind::I32,
            .primary_class = bir::AbiValueClass::Integer,
        },
    });
  }
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum.0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "tmp.call.0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "tmp.call.1"),
  });
  for (int index = 2; index < 8; ++index) {
    entry.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "sum." + std::to_string(index - 1)),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "sum." + std::to_string(index - 2)),
        .rhs = bir::Value::named(bir::TypeKind::I32, "tmp.call." + std::to_string(index)),
    });
  }
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum.6")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& prepared,
                                                      const char* object_name) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (prepare::prepared_stack_object_name(prepared.names, object) == object_name) {
      return &object;
    }
  }
  return nullptr;
}

const prepare::PreparedStoragePlanFunction* find_storage_plan_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_storage_plan(prepared, function_id);
}

const prepare::PreparedCallPlansFunction* find_call_plans_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_call_plans(prepared, function_id);
}

const prepare::PreparedStoragePlanValue* find_storage_value(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedStoragePlanFunction& function,
    const char* value_name) {
  const auto value_id = prepared.names.value_names.find(value_name);
  if (value_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& value : function.values) {
    if (value.value_name == value_id) {
      return &value;
    }
  }
  return nullptr;
}

const prepare::PreparedF128Carrier* find_f128_carrier(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    std::string_view value_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  const auto value_id = prepared.names.value_names.find(value_name);
  if (function_id == c4c::kInvalidFunctionName || value_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* function_carriers = prepare::find_prepared_f128_carriers(prepared, function_id);
  if (function_carriers == nullptr) {
    return nullptr;
  }
  return prepare::find_prepared_f128_carrier(*function_carriers, value_id);
}

const prepare::PreparedIntrinsicCarrierFunction* find_intrinsic_carriers(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_intrinsic_carriers(prepared, function_id);
}

const prepare::PreparedInlineAsmCarrierFunction* find_inline_asm_carriers(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_inline_asm_carriers(prepared, function_id);
}

const prepare::PreparedF128RuntimeHelper* find_first_f128_runtime_helper(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  const auto* function_helpers =
      prepare::find_prepared_f128_runtime_helpers(prepared, function_id);
  if (function_helpers == nullptr || function_helpers->helpers.empty()) {
    return nullptr;
  }
  return &function_helpers->helpers.front();
}

std::string clobber_summary(const prepare::PreparedClobberedRegister& clobber) {
  std::string summary = std::string(prepare::prepared_register_bank_name(clobber.bank)) + ":" +
                        clobber.register_name + "/w" + std::to_string(clobber.contiguous_width);
  if (!clobber.occupied_register_names.empty()) {
    summary += "[";
    for (std::size_t index = 0; index < clobber.occupied_register_names.size(); ++index) {
      if (index != 0) {
        summary += ",";
      }
      summary += clobber.occupied_register_names[index];
    }
    summary += "]";
  }
  return summary;
}

std::string clobber_detail(const prepare::PreparedClobberedRegister& clobber) {
  std::string detail = "clobber bank=" +
                       std::string(prepare::prepared_register_bank_name(clobber.bank));
  if (clobber.placement.has_value()) {
    detail += " placement=" +
              std::string(prepare::prepared_register_bank_name(clobber.placement->bank)) +
              ":" + std::string(prepare::prepared_register_slot_pool_name(clobber.placement->pool)) +
              "#" + std::to_string(clobber.placement->slot_index) + "/w" +
              std::to_string(clobber.placement->contiguous_width);
  }
  detail += " reg=" + clobber.register_name;
  detail += " width=" + std::to_string(clobber.contiguous_width);
  if (!clobber.occupied_register_names.empty()) {
    detail += " units=";
    for (std::size_t index = 0; index < clobber.occupied_register_names.size(); ++index) {
      if (index != 0) {
        detail += ",";
      }
      detail += clobber.occupied_register_names[index];
    }
  }
  return detail;
}

std::string register_placement_text(
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    std::string_view label = "placement") {
  if (!placement.has_value()) {
    return std::string(label) + "=<missing>";
  }
  return std::string(label) + "=" +
         std::string(prepare::prepared_register_bank_name(placement->bank)) + ":" +
         std::string(prepare::prepared_register_slot_pool_name(placement->pool)) + "#" +
         std::to_string(placement->slot_index) + "/w" +
         std::to_string(placement->contiguous_width);
}

std::string spill_slot_placement_text(
    const std::optional<prepare::PreparedSpillSlotPlacement>& placement,
    std::string_view label = "spill_slot") {
  if (!placement.has_value()) {
    return std::string(label) + "=<missing>";
  }
  return std::string(label) + "=slot#" + std::to_string(placement->slot_id) +
         "+stack" + std::to_string(placement->offset_bytes);
}

std::string saved_register_slot_placement_text(
    const std::optional<prepare::PreparedSavedRegisterSlotPlacement>& placement) {
  if (!placement.has_value()) {
    return "slot_placement=<missing>";
  }
  std::string text = "slot_placement=";
  text += placement->slot_id.has_value() ? "slot#" + std::to_string(*placement->slot_id)
                                         : "<none>";
  text += "+stack";
  text += placement->stack_offset_bytes.has_value()
              ? std::to_string(*placement->stack_offset_bytes)
              : "<unknown>";
  text += " slot_size=";
  text += placement->size_bytes.has_value() ? std::to_string(*placement->size_bytes)
                                            : "<unknown>";
  text += " slot_align=";
  text += placement->align_bytes.has_value() ? std::to_string(*placement->align_bytes)
                                             : "<unknown>";
  text += " fixed_location=";
  text += placement->fixed_location ? "yes" : "no";
  text += " slot_reg=";
  text += std::string(prepare::prepared_register_bank_name(placement->bank)) + ":" +
          placement->register_name;
  text += " slot_save_index=" + std::to_string(placement->save_index);
  text += " slot_width=" + std::to_string(placement->contiguous_width);
  if (!placement->occupied_register_names.empty()) {
    text += " slot_units=";
    for (std::size_t index = 0; index < placement->occupied_register_names.size(); ++index) {
      if (index != 0) {
        text += ",";
      }
      text += placement->occupied_register_names[index];
    }
  }
  text += " " + register_placement_text(placement->register_placement,
                                         "slot_register_placement");
  return text;
}

const prepare::PreparedFrameSlot* find_frame_slot(const prepare::PreparedBirModule& prepared,
                                                  prepare::PreparedObjectId object_id) {
  for (const auto& slot : prepared.stack_layout.frame_slots) {
    if (slot.object_id == object_id) {
      return &slot;
    }
  }
  return nullptr;
}

bool expect_contains(const std::string& text,
                     const std::string& needle,
                     const char* description) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] missing " << description << ": " << needle << "\n";
  std::cerr << "--- dump ---\n" << text << "\n";
  return false;
}

bool expect_not_contains(const std::string& text,
                         const std::string& needle,
                         const char* description) {
  if (text.find(needle) == std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] unexpected " << description << ": " << needle << "\n";
  std::cerr << "--- dump ---\n" << text << "\n";
  return false;
}

bool expect_equal_text(const std::string& actual,
                       const std::string& expected,
                       const char* description) {
  if (actual == expected) {
    return true;
  }
  std::cerr << "[FAIL] unexpected " << description << "\n";
  std::cerr << "--- expected ---\n" << expected << "\n";
  std::cerr << "--- actual ---\n" << actual << "\n";
  return false;
}

prepare::PreparedBirModule prepared_module_printer_body_fixture() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();
  return prepared;
}

bir::Function prepared_module_printer_void_function(std::string_view name) {
  bir::Function function;
  function.name = name;
  function.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));

  return function;
}

prepare::PreparedBirModule prepared_module_printer_function_body_fixture() {
  auto prepared = prepared_module_printer_body_fixture();
  prepared.module.functions.push_back(
      prepared_module_printer_void_function("module_text_contract"));
  return prepared;
}

prepare::PreparedBirModule prepared_module_printer_multi_function_body_fixture() {
  auto prepared = prepared_module_printer_body_fixture();
  prepared.module.functions.push_back(
      prepared_module_printer_void_function("module_text_contract"));
  prepared.module.functions.push_back(
      prepared_module_printer_void_function("module_text_contract_second"));
  return prepared;
}

prepare::PreparedBirModule prepared_module_printer_global_body_fixture() {
  auto prepared = prepared_module_printer_body_fixture();
  prepared.module.globals.push_back(bir::Global{
      .name = "global0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  return prepared;
}

prepare::PreparedBirModule prepared_module_printer_string_body_fixture() {
  auto prepared = prepared_module_printer_body_fixture();
  prepared.module.string_constants.push_back(bir::StringConstant{
      .name = ".str.0",
      .bytes = "hello",
      .align_bytes = 1,
  });
  return prepared;
}

prepare::PreparedBirModule prepared_module_printer_function_global_string_body_fixture() {
  auto prepared = prepared_module_printer_function_body_fixture();
  prepared.module.globals.push_back(bir::Global{
      .name = "global0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.module.string_constants.push_back(bir::StringConstant{
      .name = ".str.0",
      .bytes = "hello",
      .align_bytes = 1,
  });
  return prepared;
}

prepare::PreparedBirModule prepared_module_printer_phase_note_fixture() {
  auto prepared = prepared_module_printer_body_fixture();
  prepared.completed_phases = {"legalize", "out_of_ssa"};
  prepared.notes.push_back(prepare::PrepareNote{
      .phase = "printer",
      .message = "complete module body text retained",
  });
  return prepared;
}

prepare::PreparedBirModule prepared_module_printer_invariant_fixture() {
  auto prepared = prepared_module_printer_body_fixture();
  prepared.invariants.push_back(prepare::PreparedBirInvariant::NoTargetFacingI1);
  prepared.invariants.push_back(prepare::PreparedBirInvariant::NoPhiNodes);
  return prepared;
}

int complete_module_body_text_printer_rows_are_byte_stable() {
  const std::string header =
      "prepared.module target=riscv64-unknown-linux-gnu "
      "route=semantic_bir_shared\n";
  const std::string empty_prepared_metadata_tail =
      "--- prepared-function-summaries ---\n"
      "--- prepared-control-flow ---\n"
      "--- prepared-value-locations ---\n"
      "--- prepared-block-entry-publications ---\n"
      "--- prepared-stack-layout ---\n"
      "frame_size=0 frame_alignment=0\n"
      "--- prepared-frame-plan ---\n"
      "--- prepared-dynamic-stack-plan ---\n"
      "--- prepared-call-plans ---\n"
      "--- prepared-store-source-publications ---\n"
      "--- prepared-select-chain-materializations ---\n"
      "--- prepared-variadic-entry-plans ---\n"
      "--- prepared-regalloc ---\n"
      "--- prepared-storage-plans ---\n"
      "--- prepared-i128-carriers ---\n"
      "--- prepared-f128-carriers ---\n"
      "--- prepared-atomic-operations ---\n"
      "--- prepared-intrinsic-carriers ---\n"
      "--- prepared-inline-asm-carriers ---\n"
      "--- prepared-f128-runtime-helpers ---\n"
      "--- prepared-i128-runtime-helpers ---\n"
      "--- prepared-addressing ---\n";

  const auto empty = prepared_module_printer_body_fixture();
  if (!expect_equal_text(prepare::print(empty),
                         header + "--- prepared-bir ---\n" +
                             empty_prepared_metadata_tail,
                         "empty prepared module output")) {
    return EXIT_FAILURE;
  }

  const auto function_only = prepared_module_printer_function_body_fixture();
  if (!expect_equal_text(
          prepare::print(function_only),
          header +
              "--- prepared-bir ---\n"
              "bir.func @module_text_contract() -> void {\n"
              "entry:\n"
              "  bir.ret\n"
              "}\n"
              "\n" +
              empty_prepared_metadata_tail,
          "function-only prepared module output")) {
    return EXIT_FAILURE;
  }

  const auto multi_function = prepared_module_printer_multi_function_body_fixture();
  if (!expect_equal_text(
          prepare::print(multi_function),
          header +
              "--- prepared-bir ---\n"
              "bir.func @module_text_contract() -> void {\n"
              "entry:\n"
              "  bir.ret\n"
              "}\n"
              "\n"
              "bir.func @module_text_contract_second() -> void {\n"
              "entry:\n"
              "  bir.ret\n"
              "}\n"
              "\n" +
              empty_prepared_metadata_tail,
          "multiple-function BIR body blank-line output")) {
    return EXIT_FAILURE;
  }

  const auto global_only = prepared_module_printer_global_body_fixture();
  if (!expect_equal_text(prepare::print(global_only),
                         header + "--- prepared-bir ---\n\n" +
                             empty_prepared_metadata_tail,
                         "global-only compatibility blank-line output")) {
    return EXIT_FAILURE;
  }

  const auto string_only = prepared_module_printer_string_body_fixture();
  if (!expect_equal_text(prepare::print(string_only),
                         header + "--- prepared-bir ---\n\n" +
                             empty_prepared_metadata_tail,
                         "string-only compatibility blank-line output")) {
    return EXIT_FAILURE;
  }

  const auto function_global_string =
      prepared_module_printer_function_global_string_body_fixture();
  if (!expect_equal_text(
          prepare::print(function_global_string),
          header +
              "--- prepared-bir ---\n"
              "bir.func @module_text_contract() -> void {\n"
              "entry:\n"
              "  bir.ret\n"
              "}\n"
              "\n" +
              empty_prepared_metadata_tail,
          "function with global and string compatibility blank-line output")) {
    return EXIT_FAILURE;
  }

  const auto phase_note = prepared_module_printer_phase_note_fixture();
  if (!expect_equal_text(prepare::print(phase_note),
                         header +
                             "completed_phases: legalize out_of_ssa\n"
                             "notes:\n"
                             "  - [printer] complete module body text retained\n"
                             "--- prepared-bir ---\n" +
                             empty_prepared_metadata_tail,
                         "phase and note header output")) {
    return EXIT_FAILURE;
  }

  const auto invariants = prepared_module_printer_invariant_fixture();
  if (!expect_equal_text(prepare::print(invariants),
                         header +
                             "invariants:\n"
                             "  - no_target_facing_i1\n"
                             "  - no_phi_nodes\n"
                             "--- prepared-bir ---\n" +
                             empty_prepared_metadata_tail,
                         "invariant header placement output")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

prepare::PreparedBirModule prepare_atomic_carrier_dump_module(bool complete) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = complete ? "atomic_carrier_dump_contract"
                           : "atomic_incomplete_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "ptr",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "expected",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "desired",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));

  if (complete) {
    function.atomic_operations = {
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::Load,
            .block_label = "entry",
            .inst_index = 0,
            .value_type = bir::TypeKind::I32,
            .width_bytes = 4,
            .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
            .pointer = bir::Value::named(bir::TypeKind::Ptr, "ptr"),
            .ordering = bir::AtomicOrdering::Acquire,
            .result_mode = bir::AtomicResultMode::LoadedValue,
        },
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::Store,
            .block_label = "entry",
            .inst_index = 1,
            .value_type = bir::TypeKind::I32,
            .width_bytes = 4,
            .pointer = bir::Value::named(bir::TypeKind::Ptr, "ptr"),
            .value = bir::Value::named(bir::TypeKind::I32, "value"),
            .ordering = bir::AtomicOrdering::Release,
        },
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::Fence,
            .block_label = "entry",
            .inst_index = 2,
            .ordering = bir::AtomicOrdering::SeqCst,
        },
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::Rmw,
            .block_label = "entry",
            .inst_index = 3,
            .value_type = bir::TypeKind::I32,
            .width_bytes = 4,
            .result = bir::Value::named(bir::TypeKind::I32, "old"),
            .pointer = bir::Value::named(bir::TypeKind::Ptr, "ptr"),
            .value = bir::Value::named(bir::TypeKind::I32, "value"),
            .ordering = bir::AtomicOrdering::AcqRel,
            .rmw_opcode = bir::AtomicRmwOpcode::Add,
            .result_mode = bir::AtomicResultMode::OldValue,
        },
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::CompareExchange,
            .block_label = "entry",
            .inst_index = 4,
            .value_type = bir::TypeKind::I32,
            .width_bytes = 4,
            .result = bir::Value::named(bir::TypeKind::I1, "success"),
            .pointer = bir::Value::named(bir::TypeKind::Ptr, "ptr"),
            .expected = bir::Value::named(bir::TypeKind::I32, "expected"),
            .desired = bir::Value::named(bir::TypeKind::I32, "desired"),
            .ordering = bir::AtomicOrdering::SeqCst,
            .failure_ordering = bir::AtomicOrdering::Acquire,
            .result_mode = bir::AtomicResultMode::BooleanSuccess,
        },
    };
  } else {
    function.atomic_operations = {
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::Rmw,
            .block_label = "entry",
            .inst_index = 0,
            .value_type = bir::TypeKind::I32,
            .result = bir::Value::named(bir::TypeKind::I32, "old"),
            .pointer = bir::Value::named(bir::TypeKind::Ptr, "ptr"),
        },
        bir::AtomicOperation{
            .kind = bir::AtomicOperationKind::CompareExchange,
            .block_label = "entry",
            .inst_index = 1,
            .value_type = bir::TypeKind::I32,
            .width_bytes = 4,
            .result = bir::Value::named(bir::TypeKind::I1, "success"),
            .pointer = bir::Value::named(bir::TypeKind::Ptr, "ptr"),
            .desired = bir::Value::named(bir::TypeKind::I32, "desired"),
            .ordering = bir::AtomicOrdering::Acquire,
        },
    };
  }

  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

int atomic_carrier_facts_preserve_fields_and_printer_visibility() {
  const auto prepared = prepare_atomic_carrier_dump_module(true);
  const c4c::FunctionNameId function_id =
      prepared.names.function_names.find("atomic_carrier_dump_contract");
  const auto* function_operations =
      function_id == c4c::kInvalidFunctionName
          ? nullptr
          : prepare::find_prepared_atomic_operations(prepared, function_id);
  if (function_operations == nullptr || function_operations->operations.size() != 5) {
    std::cerr << "[FAIL] expected five prepared atomic carrier facts\n";
    return EXIT_FAILURE;
  }
  const auto& compare_exchange = function_operations->operations[4];
  if (compare_exchange.carrier_kind !=
          prepare::PreparedAtomicOperationCarrierKind::Complete ||
      compare_exchange.operation_kind != bir::AtomicOperationKind::CompareExchange ||
      compare_exchange.value_type != bir::TypeKind::I32 ||
      compare_exchange.width_bytes != 4 ||
      compare_exchange.ordering != bir::AtomicOrdering::SeqCst ||
      compare_exchange.failure_ordering != bir::AtomicOrdering::Acquire ||
      compare_exchange.result_mode != bir::AtomicResultMode::BooleanSuccess ||
      !compare_exchange.pointer_value_name.has_value() ||
      !compare_exchange.expected_value_name.has_value() ||
      !compare_exchange.desired_value_name.has_value() ||
      !compare_exchange.missing_required_facts.empty()) {
    std::cerr << "[FAIL] prepared compare-exchange carrier lost required fields\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "--- prepared-atomic-operations ---",
                       "prepared atomic operations section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "atomic_operation kind=load block=entry inst_index=0 type=i32 "
                       "width=4 ordering=acquire",
                       "complete load atomic carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "atomic_operation kind=fence block=entry inst_index=2 type=void "
                       "width=0 ordering=seq_cst",
                       "complete fence atomic carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "atomic_operation kind=compare_exchange block=entry inst_index=4 "
                       "type=i32 width=4 ordering=seq_cst failure_ordering=acquire "
                       "result_mode=boolean_success",
                       "complete compare-exchange atomic carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "pointer=ptr expected=expected desired=desired",
                       "compare-exchange operand identity")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int incomplete_atomic_carrier_facts_fail_closed() {
  const auto prepared = prepare_atomic_carrier_dump_module(false);
  const c4c::FunctionNameId function_id =
      prepared.names.function_names.find("atomic_incomplete_carrier_dump_contract");
  const auto* function_operations =
      function_id == c4c::kInvalidFunctionName
          ? nullptr
          : prepare::find_prepared_atomic_operations(prepared, function_id);
  if (function_operations == nullptr || function_operations->operations.size() != 2) {
    std::cerr << "[FAIL] expected two incomplete prepared atomic carrier facts\n";
    return EXIT_FAILURE;
  }
  const auto& rmw = function_operations->operations[0];
  if (rmw.carrier_kind != prepare::PreparedAtomicOperationCarrierKind::Missing ||
      rmw.missing_required_facts.empty()) {
    std::cerr << "[FAIL] expected incomplete RMW carrier diagnostics\n";
    return EXIT_FAILURE;
  }
  const auto& compare_exchange = function_operations->operations[1];
  if (compare_exchange.carrier_kind !=
          prepare::PreparedAtomicOperationCarrierKind::Missing ||
      compare_exchange.missing_required_facts.empty()) {
    std::cerr << "[FAIL] expected incomplete compare-exchange carrier diagnostics\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "missing fact=inst#0:missing_atomic_width",
                       "RMW missing width diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#0:rmw_requires_value",
                       "RMW missing value diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#1:compare_exchange_requires_expected",
                       "compare-exchange missing expected diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#1:compare_exchange_requires_failure_ordering",
                       "compare-exchange missing failure ordering diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "atomic_operation kind=rmw block=entry inst_index=0",
                           "incomplete RMW carrier printer record")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "atomic_operation kind=compare_exchange block=entry inst_index=1",
                           "incomplete compare-exchange carrier printer record")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

prepare::PreparedBirModule prepare_intrinsic_carrier_dump_module(
    bir::TypeKind value_type,
    bool complete,
    bool structured_intrinsic = true) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = complete ? "intrinsic_carrier_dump_contract"
                           : "intrinsic_incomplete_carrier_dump_contract";
  function.return_type = value_type;
  function.params.push_back(bir::Param{
      .type = value_type,
      .name = "x",
      .size_bytes = value_type == bir::TypeKind::F64 ? std::size_t{8} : std::size_t{4},
      .align_bytes = value_type == bir::TypeKind::F64 ? std::size_t{8} : std::size_t{4},
  });

  bir::CallInst call{
      .result = complete ? std::optional<bir::Value>{bir::Value::named(value_type, "abs")}
                         : std::nullopt,
      .callee = value_type == bir::TypeKind::F64 ? "llvm.fabs.double" : "llvm.fabs.float",
      .args = {bir::Value::named(value_type, "x")},
      .arg_types = {value_type},
      .arg_abi = {*prepare::infer_call_arg_abi(aarch64_target_profile(), value_type)},
      .return_type = value_type,
      .result_abi = bir::CallResultAbiInfo{
          .type = value_type,
          .primary_class = bir::AbiValueClass::Sse,
      },
  };
  if (structured_intrinsic) {
    call.intrinsic = bir::IntrinsicOperation{
        .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
        .operation = bir::IntrinsicOperationKind::FAbs,
        .operand_type = value_type,
        .result_type = value_type,
        .has_side_effects = false,
    };
  }

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = complete
                         ? bir::ReturnTerminator{
                               .value = bir::Value::named(value_type, "abs"),
                           }
                         : bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_unsupported_intrinsic_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "intrinsic_unsupported_carrier_dump_contract";
  function.return_type = bir::TypeKind::F128;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "x",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  bir::CallInst call{
      .result = bir::Value::named(bir::TypeKind::F128, "abs"),
      .callee = "llvm.fabs.x86_fp80",
      .args = {bir::Value::named(bir::TypeKind::F128, "x")},
      .arg_types = {bir::TypeKind::F128},
      .arg_abi = {*prepare::infer_call_arg_abi(aarch64_target_profile(), bir::TypeKind::F128)},
      .return_type = bir::TypeKind::F128,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::F128,
          .primary_class = bir::AbiValueClass::Sse,
      },
      .intrinsic = bir::IntrinsicOperation{
          .family = bir::IntrinsicFamilyKind::ScalarFpUnary,
          .operation = bir::IntrinsicOperationKind::FAbs,
          .operand_type = bir::TypeKind::F128,
          .result_type = bir::TypeKind::F128,
          .has_side_effects = false,
      },
  };
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F128, "abs"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

int scalar_fp_unary_intrinsic_carrier_preserves_fields_and_printer_visibility() {
  const auto prepared = prepare_intrinsic_carrier_dump_module(bir::TypeKind::F64, true);
  const auto* function_carriers =
      find_intrinsic_carriers(prepared, "intrinsic_carrier_dump_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one prepared intrinsic carrier fact\n";
    return EXIT_FAILURE;
  }
  const auto& carrier = function_carriers->carriers.front();
  if (carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      carrier.family != bir::IntrinsicFamilyKind::ScalarFpUnary ||
      carrier.operation != bir::IntrinsicOperationKind::FAbs ||
      carrier.operand_type != bir::TypeKind::F64 ||
      carrier.result_type != bir::TypeKind::F64 ||
      !carrier.operand_value_name.has_value() ||
      !carrier.result_value_name.has_value() ||
      carrier.has_side_effects ||
      carrier.requires_feature ||
      !carrier.has_prepared_call_plan ||
      carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] prepared intrinsic carrier lost required fields\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "--- prepared-intrinsic-carriers ---",
                       "prepared intrinsic carriers section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "intrinsic_carrier family=scalar_fp_unary operation=fabs "
                       "feature=none block_index=0 inst_index=0 operand_type=f64 "
                       "result_type=f64 signedness=none memory_access=none "
                       "side_effects=no requires_feature=no prepared_call_plan=yes "
                       "source_callee=llvm.fabs.double operand=x result=abs",
                       "complete scalar fabs carrier")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int partial_and_unsupported_intrinsic_carriers_fail_closed() {
  const auto missing_result =
      prepare_intrinsic_carrier_dump_module(bir::TypeKind::F32, false);
  const auto* missing_result_carriers =
      find_intrinsic_carriers(missing_result, "intrinsic_incomplete_carrier_dump_contract");
  if (missing_result_carriers == nullptr ||
      missing_result_carriers->carriers.size() != 1 ||
      missing_result_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] expected incomplete intrinsic carrier diagnostic\n";
    return EXIT_FAILURE;
  }
  const std::string missing_result_dump = prepare::print(missing_result);
  if (!expect_contains(missing_result_dump,
                       "missing fact=inst#0:scalar_fp_unary_requires_result",
                       "intrinsic missing result diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(missing_result_dump,
                       "missing fact=inst#0:prepared_call_plan_requires_result",
                       "intrinsic missing prepared result diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(missing_result_dump,
                           "intrinsic_carrier family=scalar_fp_unary operation=fabs",
                           "incomplete intrinsic carrier printer record")) {
    return EXIT_FAILURE;
  }

  const auto unsupported = prepare_unsupported_intrinsic_carrier_dump_module();
  const auto* unsupported_carriers =
      find_intrinsic_carriers(unsupported, "intrinsic_unsupported_carrier_dump_contract");
  if (unsupported_carriers == nullptr ||
      unsupported_carriers->carriers.size() != 1 ||
      unsupported_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] expected unsupported x86/F128 intrinsic carrier diagnostic\n";
    return EXIT_FAILURE;
  }
  const std::string unsupported_dump = prepare::print(unsupported);
  if (!expect_contains(unsupported_dump,
                       "missing fact=inst#0:unsupported_scalar_fp_unary_type",
                       "unsupported x86/F128 intrinsic diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(unsupported_dump,
                           "source_callee=llvm.fabs.x86_fp80 operand=x result=abs",
                           "unsupported x86/F128 intrinsic carrier printer record")) {
    return EXIT_FAILURE;
  }

  const auto call_only =
      prepare_intrinsic_carrier_dump_module(bir::TypeKind::F64, true, false);
  if (find_intrinsic_carriers(call_only, "intrinsic_carrier_dump_contract") != nullptr) {
    std::cerr << "[FAIL] ordinary call plan fabricated intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

bir::CallArgAbiInfo scalar_arg_abi(bir::TypeKind type);

bir::InlineAsmOperandMetadata inline_asm_operand(
    bir::InlineAsmOperandKind kind,
    std::size_t constraint_index,
    std::string constraint,
    std::optional<std::size_t> arg_index = std::nullopt,
    std::optional<std::size_t> output_index = std::nullopt,
    std::optional<std::size_t> tied_output_index = std::nullopt,
    std::optional<std::string> name = std::nullopt,
    std::optional<bir::MemoryAddress> memory_address = std::nullopt,
    std::optional<bir::MemoryAddress> address = std::nullopt,
    bir::InlineAsmRegisterClass register_class = bir::InlineAsmRegisterClass::None,
    std::size_t register_group_width = 1) {
  return bir::InlineAsmOperandMetadata{
      .kind = kind,
      .constraint_index = constraint_index,
      .constraint = std::move(constraint),
      .arg_index = arg_index,
      .output_index = output_index,
      .tied_output_index = tied_output_index,
      .register_class = register_class,
      .register_group_width = register_group_width,
      .name = std::move(name),
      .memory_address = std::move(memory_address),
      .address = std::move(address),
  };
}

prepare::PreparedBirModule prepare_inline_asm_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "inline_asm_carrier_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "x")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "mov %0, %0",
          .constraints = "r",
          .side_effects = true,
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::RegisterInput, 0, "r", std::size_t{0})},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "out"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "x"),
               bir::Value::immediate_i32(7)},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32),
                  scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "add %w0, %x0, #7",
          .constraints = "=r,0,I",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                 0,
                                 "=r",
                                 std::nullopt,
                                 std::size_t{0}),
              inline_asm_operand(bir::InlineAsmOperandKind::TiedInput,
                                 1,
                                 "0",
                                 std::size_t{0},
                                 std::nullopt,
                                 std::size_t{0}),
              inline_asm_operand(bir::InlineAsmOperandKind::IntegerImmediateInput,
                                 2,
                                 "I",
                                 std::size_t{1}),
          },
          .has_template_modifiers = true,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "x")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "mov %0, %0",
          .constraints = "r,~{memory}",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::RegisterInput,
                                 0,
                                 "r",
                                 std::size_t{0}),
              inline_asm_operand(bir::InlineAsmOperandKind::Clobber,
                                 1,
                                 "~{memory}",
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 std::string("memory")),
          },
          .clobbers = {"memory"},
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "out"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_rv64_inline_asm_scalar_carrier_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  const c4c::TargetProfile target = riscv_target_profile();
  const auto i32_arg_abi = *prepare::infer_call_arg_abi(target, bir::TypeKind::I32);
  const bir::CallResultAbiInfo i32_result_abi{
      .type = bir::TypeKind::I32,
      .primary_class = bir::AbiValueClass::Integer,
  };

  bir::Function function;
  function.name = "rv64_inline_asm_scalar_carrier_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "x")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {i32_arg_abi},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, %0, %0",
          .constraints = "r",
          .side_effects = true,
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::RegisterInput, 0, "r", std::size_t{0})},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "out"),
      .callee = "llvm.inline_asm",
      .return_type = bir::TypeKind::I32,
      .result_abi = i32_result_abi,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, zero, zero",
          .constraints = "=r",
          .side_effects = true,
          .operands = {inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                          0,
                                          "=r",
                                          std::nullopt,
                                          std::size_t{0})},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tie"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "tie")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {i32_arg_abi},
      .return_type = bir::TypeKind::I32,
      .result_abi = i32_result_abi,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, %0, zero",
          .constraints = "=r,0",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                 0,
                                 "=r",
                                 std::nullopt,
                                 std::size_t{0}),
              inline_asm_operand(bir::InlineAsmOperandKind::TiedInput,
                                 1,
                                 "0",
                                 std::size_t{0},
                                 std::nullopt,
                                 std::size_t{0}),
          },
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "tie"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, target, prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_rv64_insn_r_structured_scalar_carrier_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  const c4c::TargetProfile target = riscv_target_profile();
  const auto i32_arg_abi = *prepare::infer_call_arg_abi(target, bir::TypeKind::I32);
  const bir::CallResultAbiInfo i32_result_abi{
      .type = bir::TypeKind::I32,
      .primary_class = bir::AbiValueClass::Integer,
  };

  bir::Function function;
  function.name = "rv64_insn_r_structured_scalar_carrier_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{.type = bir::TypeKind::I32,
                                       .name = "lhs",
                                       .size_bytes = 4,
                                       .align_bytes = 4});
  function.params.push_back(bir::Param{.type = bir::TypeKind::I32,
                                       .name = "rhs",
                                       .size_bytes = 4,
                                       .align_bytes = 4});

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "lhs"),
               bir::Value::named(bir::TypeKind::I32, "rhs")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {i32_arg_abi, i32_arg_abi},
      .return_type = bir::TypeKind::I32,
      .result_abi = i32_result_abi,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, %1, %2",
          .constraints = "=r,r,r",
          .side_effects = true,
          .operands =
              {
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                     0,
                                     "=r",
                                     std::nullopt,
                                     std::size_t{0},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterInput,
                                     1,
                                     "r",
                                     std::size_t{0},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterInput,
                                     2,
                                     "r",
                                     std::size_t{1},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
              },
          .insn_r = bir::InlineAsmInsnRMetadata{.opcode = 0x33,
                                                .funct3 = 0,
                                                .funct7 = 0,
                                                .operand_indices = {0, 1, 2}},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tie"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "tie"),
               bir::Value::named(bir::TypeKind::I32, "rhs")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {i32_arg_abi, i32_arg_abi},
      .return_type = bir::TypeKind::I32,
      .result_abi = i32_result_abi,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, %1, %2",
          .constraints = "=r,0,r",
          .side_effects = true,
          .operands =
              {
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                     0,
                                     "=r",
                                     std::nullopt,
                                     std::size_t{0},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
                  inline_asm_operand(bir::InlineAsmOperandKind::TiedInput,
                                     1,
                                     "0",
                                     std::size_t{0},
                                     std::nullopt,
                                     std::size_t{0}),
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterInput,
                                     2,
                                     "r",
                                     std::size_t{1},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
              },
          .insn_r = bir::InlineAsmInsnRMetadata{.opcode = 0x33,
                                                .funct3 = 0,
                                                .funct7 = 0,
                                                .operand_indices = {0, 1, 2}},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "rw"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "rw"),
               bir::Value::named(bir::TypeKind::I32, "rhs")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {i32_arg_abi, i32_arg_abi},
      .return_type = bir::TypeKind::I32,
      .result_abi = i32_result_abi,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, %0, %1",
          .constraints = "+r,r",
          .side_effects = true,
          .operands =
              {
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                     0,
                                     "+r",
                                     std::size_t{0},
                                     std::size_t{0},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterInput,
                                     1,
                                     "r",
                                     std::size_t{1},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
              },
          .insn_r = bir::InlineAsmInsnRMetadata{.opcode = 0x33,
                                                .funct3 = 0,
                                                .funct7 = 0,
                                                .operand_indices = {0, 0, 1}},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "bad"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::immediate_i32(7), bir::Value::named(bir::TypeKind::I32, "rhs")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {i32_arg_abi, i32_arg_abi},
      .return_type = bir::TypeKind::I32,
      .result_abi = i32_result_abi,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = ".insn r 0x33, 0, 0, %0, %1, %2",
          .constraints = "=r,I,r",
          .side_effects = true,
          .operands =
              {
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                     0,
                                     "=r",
                                     std::nullopt,
                                     std::size_t{0},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
                  inline_asm_operand(bir::InlineAsmOperandKind::IntegerImmediateInput,
                                     1,
                                     "I",
                                     std::size_t{0}),
                  inline_asm_operand(bir::InlineAsmOperandKind::RegisterInput,
                                     2,
                                     "r",
                                     std::size_t{1},
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     bir::InlineAsmRegisterClass::General),
              },
          .unsupported_facts = {"insn_r_operand1_requires_scalar_gpr"},
          .insn_r = bir::InlineAsmInsnRMetadata{.opcode = 0x33,
                                                .funct3 = 0,
                                                .funct7 = 0,
                                                .operand_indices = {0, 1, 2}},
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "rw"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, target, prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_rv64_inline_asm_vector_carrier_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  const c4c::TargetProfile target = riscv_target_profile();
  bir::Function function;
  function.name = "rv64_inline_asm_vector_carrier_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Vrm2,
      .name = "tmp",
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Vrm4,
      .name = "out",
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Vrm2,
      .name = "tie",
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Vrm2, "tmp")},
      .arg_types = {bir::TypeKind::Vrm2},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "vmv.v.v %0, %0",
          .constraints = "VRM2",
          .side_effects = true,
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::RegisterInput,
              0,
              "VRM2",
              std::size_t{0},
              std::nullopt,
              std::nullopt,
              std::nullopt,
              std::nullopt,
              std::nullopt,
              bir::InlineAsmRegisterClass::Vector,
              2)},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Vrm4, "out"),
      .callee = "llvm.inline_asm",
      .return_type = bir::TypeKind::Vrm4,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "vmv.v.i %0, 0",
          .constraints = "=VRM4",
          .side_effects = true,
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::RegisterOutput,
              0,
              "=VRM4",
              std::nullopt,
              std::size_t{0},
              std::nullopt,
              std::nullopt,
              std::nullopt,
              std::nullopt,
              bir::InlineAsmRegisterClass::Vector,
              4)},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Vrm2, "tie"),
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Vrm2, "tie")},
      .arg_types = {bir::TypeKind::Vrm2},
      .return_type = bir::TypeKind::Vrm2,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "vadd.vv %0, %0, %0",
          .constraints = "=VRM2,0",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::RegisterOutput,
                                 0,
                                 "=VRM2",
                                 std::nullopt,
                                 std::size_t{0},
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 bir::InlineAsmRegisterClass::Vector,
                                 2),
              inline_asm_operand(bir::InlineAsmOperandKind::TiedInput,
                                 1,
                                 "0",
                                 std::size_t{0},
                                 std::nullopt,
                                 std::size_t{0}),
          },
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, target, prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_rv64_inline_asm_vector_impossible_carrier_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  const c4c::TargetProfile target = riscv_target_profile();
  bir::Function function;
  function.name = "rv64_inline_asm_vector_impossible_carrier_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  bir::CallInst inline_asm;
  inline_asm.callee = "llvm.inline_asm";
  inline_asm.return_type = bir::TypeKind::Void;
  inline_asm.inline_asm = bir::InlineAsmMetadata{
      .asm_text = "vpressure",
      .constraints = "",
      .side_effects = true,
  };

  for (std::size_t index = 0; index < 33; ++index) {
    const std::string value_name = "v" + std::to_string(index);
    function.params.push_back(bir::Param{
        .type = bir::TypeKind::Vrm1,
        .name = value_name,
    });
    inline_asm.args.push_back(bir::Value::named(bir::TypeKind::Vrm1, value_name));
    inline_asm.arg_types.push_back(bir::TypeKind::Vrm1);
    if (!inline_asm.inline_asm->constraints.empty()) {
      inline_asm.inline_asm->constraints += ",";
    }
    inline_asm.inline_asm->constraints += "VR";
    inline_asm.inline_asm->operands.push_back(inline_asm_operand(
        bir::InlineAsmOperandKind::RegisterInput,
        index,
        "VR",
        index,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        bir::InlineAsmRegisterClass::Vector,
        1));
  }

  entry.insts.push_back(std::move(inline_asm));
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, target, prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_inline_asm_fail_closed_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "inline_asm_fail_closed_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  const bir::MemoryAddress pointer_address{
      .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
      .base_name = "x",
      .base_value = bir::Value::named(bir::TypeKind::Ptr, "x"),
      .byte_offset = 0,
      .size_bytes = 4,
      .align_bytes = 4,
  };
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::I32, "x"),
               bir::Value::named(bir::TypeKind::Ptr, "x")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::Ptr},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32),
                  scalar_arg_abi(bir::TypeKind::Ptr)},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "%q0 %[src]",
          .constraints = "m,p,~{memory}",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::MemoryInput,
                                 0,
                                 "m",
                                 std::size_t{0}),
              inline_asm_operand(bir::InlineAsmOperandKind::AddressInput,
                                 1,
                                 "p",
                                 std::size_t{1}),
              inline_asm_operand(bir::InlineAsmOperandKind::Clobber,
                                 2,
                                 "~{memory}"),
          },
          .unsupported_facts = {
              "unsupported_clobber_constraint2",
              "unsupported_named_operands",
              "unsupported_template_modifiers",
          },
          .has_named_operand_references = true,
          .has_template_modifiers = true,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "x"),
               bir::Value::named(bir::TypeKind::Ptr, "x")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::Ptr),
                  scalar_arg_abi(bir::TypeKind::Ptr)},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "",
          .constraints = "m,p",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::MemoryInput,
                                 0,
                                 "m",
                                 std::size_t{0},
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 pointer_address),
              inline_asm_operand(bir::InlineAsmOperandKind::AddressInput,
                                 1,
                                 "p",
                                 std::size_t{1},
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 pointer_address),
          },
      },
  });
  const bir::MemoryAddress local_slot_address{
      .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
      .base_name = "slot.invalid",
      .byte_offset = 4,
      .size_bytes = 4,
      .align_bytes = 4,
  };
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "x"),
               bir::Value::named(bir::TypeKind::Ptr, "x")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::Ptr),
                  scalar_arg_abi(bir::TypeKind::Ptr)},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "",
          .constraints = "m,p",
          .side_effects = true,
          .operands = {
              inline_asm_operand(bir::InlineAsmOperandKind::MemoryInput,
                                 0,
                                 "m",
                                 std::size_t{0},
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 local_slot_address),
              inline_asm_operand(bir::InlineAsmOperandKind::AddressInput,
                                 1,
                                 "p",
                                 std::size_t{1},
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 std::nullopt,
                                 local_slot_address),
          },
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::immediate_i32(9)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "",
          .constraints = "r",
          .side_effects = true,
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::RegisterInput, 0, "r", std::size_t{0})},
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "x"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

int inline_asm_carriers_preserve_supported_facts_and_printer_visibility() {
  const auto prepared = prepare_inline_asm_carrier_dump_module();
  const auto* function_carriers =
      find_inline_asm_carriers(prepared, "inline_asm_carrier_dump_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 3) {
    std::cerr << "[FAIL] expected three prepared inline asm carrier facts\n";
    return EXIT_FAILURE;
  }
  const auto& input_carrier = function_carriers->carriers[0];
  const auto& tied_carrier = function_carriers->carriers[1];
  const auto& clobber_carrier = function_carriers->carriers[2];
  if (input_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      input_carrier.operands.size() != 1 ||
      input_carrier.operands.front().kind !=
          bir::InlineAsmOperandKind::RegisterInput ||
      !input_carrier.operands.front().home.has_value() ||
      !input_carrier.missing_required_facts.empty()) {
    std::cerr << "[FAIL] positional register input inline asm carrier incomplete\n";
    return EXIT_FAILURE;
  }
  if (tied_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Missing ||
      tied_carrier.operands.size() != 3 ||
      tied_carrier.operands[0].kind !=
          bir::InlineAsmOperandKind::RegisterOutput ||
      tied_carrier.operands[1].kind != bir::InlineAsmOperandKind::TiedInput ||
      tied_carrier.operands[1].tied_output_index.value_or(99) != 0 ||
      tied_carrier.operands[2].kind !=
          bir::InlineAsmOperandKind::IntegerImmediateInput ||
      tied_carrier.operands[2].immediate_value.value_or(0) != 7 ||
      !tied_carrier.has_template_modifiers ||
      !tied_carrier.result_home.has_value() ||
      tied_carrier.missing_required_facts.size() != 1 ||
      tied_carrier.missing_required_facts.front() !=
          "tied_input_output_home_mismatch" ||
      tied_carrier.operands[1].tied_home_authority.has_value()) {
    std::cerr << "[FAIL] output/tie/immediate inline asm carrier should fail closed without proven coallocation\n";
    return EXIT_FAILURE;
  }
  if (clobber_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      clobber_carrier.operands.size() != 2 ||
      clobber_carrier.operands[1].kind != bir::InlineAsmOperandKind::Clobber ||
      clobber_carrier.operands[1].name.value_or("") != "memory" ||
      clobber_carrier.clobbers.size() != 1 ||
      clobber_carrier.clobbers[0] != "memory" ||
      !clobber_carrier.missing_required_facts.empty()) {
    std::cerr << "[FAIL] structured clobber inline asm carrier incomplete\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "--- prepared-inline-asm-carriers ---",
                       "prepared inline asm carriers section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#1:tied_input_output_home_mismatch",
                       "unproven tied inline asm carrier missing fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "inline_asm_carrier asm=\"mov %0, %0\" "
                       "constraints=\"r,~{memory}\" block_index=0 inst_index=2 "
                       "side_effects=yes operands=2 result_home=no clobbers=1",
                       "complete inline asm clobber carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "operand1[kind=clobber,constraint=\"~{memory}\","
                       "name=\"memory\",home=no] clobber0=\"memory\"",
                       "structured inline asm clobber detail")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int rv64_inline_asm_carriers_preserve_scalar_register_identities() {
  const auto prepared = prepare_rv64_inline_asm_scalar_carrier_module();
  const auto* function_carriers =
      find_inline_asm_carriers(prepared, "rv64_inline_asm_scalar_carrier_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 3) {
    std::cerr << "[FAIL] expected three RV64 prepared inline asm carriers\n";
    return EXIT_FAILURE;
  }

  const auto& input_carrier = function_carriers->carriers[0];
  const auto& output_carrier = function_carriers->carriers[1];
  const auto& tied_carrier = function_carriers->carriers[2];
  if (input_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      input_carrier.operands.size() != 1 ||
      !input_carrier.operands[0].home.has_value() ||
      !input_carrier.operands[0].home->target_register_identity.has_value() ||
      input_carrier.operands[0].home->target_register_identity->target_arch !=
          c4c::TargetArch::Riscv64 ||
      input_carrier.operands[0].home->target_register_identity->bank !=
          prepare::PreparedRegisterBank::Gpr ||
      input_carrier.operands[0].home->target_register_identity->register_class !=
          prepare::PreparedRegisterClass::General) {
    std::cerr << "[FAIL] RV64 r inline asm carrier lacks concrete GPR identity\n";
    return EXIT_FAILURE;
  }
  if (output_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      output_carrier.operands.size() != 1 ||
      output_carrier.operands[0].kind != bir::InlineAsmOperandKind::RegisterOutput ||
      !output_carrier.result_home.has_value() ||
      !output_carrier.result_home->target_register_identity.has_value() ||
      output_carrier.result_home->target_register_identity->target_arch !=
          c4c::TargetArch::Riscv64 ||
      output_carrier.result_home->target_register_identity->bank !=
          prepare::PreparedRegisterBank::Gpr ||
      output_carrier.result_home->target_register_identity->register_class !=
          prepare::PreparedRegisterClass::General) {
    std::cerr << "[FAIL] RV64 =r inline asm carrier lacks concrete GPR identity\n";
    return EXIT_FAILURE;
  }
  if (tied_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      tied_carrier.operands.size() != 2 ||
      tied_carrier.operands[0].kind != bir::InlineAsmOperandKind::RegisterOutput ||
      tied_carrier.operands[1].kind != bir::InlineAsmOperandKind::TiedInput ||
      tied_carrier.operands[1].tied_output_index.value_or(99) != 0 ||
      !tied_carrier.result_home.has_value() ||
      !tied_carrier.result_home->target_register_identity.has_value() ||
      !tied_carrier.operands[1].home.has_value() ||
      !tied_carrier.operands[1].home->target_register_identity.has_value() ||
      !tied_carrier.operands[1].tied_home_authority.has_value() ||
      tied_carrier.operands[1].tied_home_authority->tied_output_index != 0 ||
      tied_carrier.operands[1].tied_home_authority->shared_register !=
          *tied_carrier.result_home->target_register_identity ||
      *tied_carrier.operands[1].home->target_register_identity !=
          *tied_carrier.result_home->target_register_identity ||
      !tied_carrier.missing_required_facts.empty()) {
    std::cerr << "[FAIL] RV64 tied numeric inline asm carrier lacks shared GPR authority\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "inline_asm_carrier asm=\".insn r 0x33, 0, 0, %0, %0, zero\" "
                       "constraints=\"=r,0\" block_index=0 inst_index=2 "
                       "side_effects=yes operands=2 result=tie result_home=yes",
                       "complete RV64 tied inline asm carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "target_invalid_tied_input_register_home",
                           "RV64 tied input invalid-register diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "target_invalid_tied_output_register_home",
                           "RV64 tied output invalid-register diagnostic")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int rv64_insn_r_structured_scalar_carriers_bind_gpr_operands() {
  const auto prepared = prepare_rv64_insn_r_structured_scalar_carrier_module();
  const auto* function_carriers =
      find_inline_asm_carriers(prepared,
                               "rv64_insn_r_structured_scalar_carrier_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 4) {
    std::cerr << "[FAIL] expected four structured RV64 .insn r carriers\n";
    return EXIT_FAILURE;
  }

  const auto& readwrite = function_carriers->carriers[2];
  const auto& invalid = function_carriers->carriers[3];
  if (readwrite.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      readwrite.operands.size() != 2 ||
      readwrite.operands[0].kind != bir::InlineAsmOperandKind::RegisterOutput ||
      readwrite.operands[0].constraint != "+r" ||
      readwrite.operands[0].arg_index.value_or(99) != 0 ||
      readwrite.operands[0].output_index.value_or(99) != 0 ||
      !readwrite.result_home.has_value() ||
      !readwrite.result_home->target_register_identity.has_value() ||
      !readwrite.operands[0].home.has_value() ||
      !readwrite.operands[0].home->target_register_identity.has_value() ||
      *readwrite.operands[0].home->target_register_identity !=
          *readwrite.result_home->target_register_identity ||
      !readwrite.missing_required_facts.empty()) {
    std::cerr << "[FAIL] structured RV64 .insn r +r scalar carrier incomplete\n";
    return EXIT_FAILURE;
  }
  if (invalid.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Missing ||
      invalid.missing_required_facts.empty()) {
    std::cerr << "[FAIL] structured RV64 .insn r invalid constraint should fail closed\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "inst#3:insn_r_operand1_requires_scalar_gpr",
                       "structured RV64 .insn r invalid scalar constraint fact")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int rv64_inline_asm_carriers_preserve_vector_register_group_facts() {
  const auto prepared = prepare_rv64_inline_asm_vector_carrier_module();
  const auto* function_carriers =
      find_inline_asm_carriers(prepared, "rv64_inline_asm_vector_carrier_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 3) {
    std::cerr << "[FAIL] expected three RV64 vector prepared inline asm carriers\n";
    return EXIT_FAILURE;
  }

  const auto function_id =
      prepared.names.function_names.find("rv64_inline_asm_vector_carrier_contract");
  const auto value_id = [&](std::string_view name) {
    return prepared.names.value_names.find(name);
  };
  const auto* x_override = prepare::find_prepared_register_group_override(
      prepared, function_id, value_id("tmp"));
  const auto* out_override = prepare::find_prepared_register_group_override(
      prepared, function_id, value_id("out"));
  const auto* tie_override = prepare::find_prepared_register_group_override(
      prepared, function_id, value_id("tie"));
  if (x_override == nullptr || out_override == nullptr || tie_override == nullptr ||
      x_override->register_class != prepare::PreparedRegisterClass::Vector ||
      x_override->contiguous_width != 2 ||
      out_override->register_class != prepare::PreparedRegisterClass::Vector ||
      out_override->contiguous_width != 4 ||
      tie_override->register_class != prepare::PreparedRegisterClass::Vector ||
      tie_override->contiguous_width != 2) {
    std::cerr << "[FAIL] RV64 vector inline asm constraints did not publish group overrides\n";
    return EXIT_FAILURE;
  }

  const auto& input_carrier = function_carriers->carriers[0];
  const auto& output_carrier = function_carriers->carriers[1];
  const auto& tied_carrier = function_carriers->carriers[2];
  const auto is_vector_home = [](const std::optional<prepare::PreparedValueHome>& home,
                                 std::size_t width) {
    return home.has_value() &&
           home->kind == prepare::PreparedValueHomeKind::Register &&
           home->register_name.has_value() &&
           home->target_register_identity.has_value() &&
           home->target_register_identity->target_arch == c4c::TargetArch::Riscv64 &&
           home->target_register_identity->bank == prepare::PreparedRegisterBank::Vreg &&
           home->target_register_identity->register_class ==
               prepare::PreparedRegisterClass::Vector &&
           width > 0;
  };
  if (input_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      input_carrier.operands.size() != 1 ||
      input_carrier.operands[0].register_class !=
          bir::InlineAsmRegisterClass::Vector ||
      input_carrier.operands[0].register_group_width != 2 ||
      !is_vector_home(input_carrier.operands[0].home, 2)) {
    std::cerr << "[FAIL] RV64 VRM2 input carrier lacks vector home facts\n";
    return EXIT_FAILURE;
  }
  if (output_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      output_carrier.operands.size() != 1 ||
      output_carrier.operands[0].register_class !=
          bir::InlineAsmRegisterClass::Vector ||
      output_carrier.operands[0].register_group_width != 4 ||
      !is_vector_home(output_carrier.result_home, 4)) {
    std::cerr << "[FAIL] RV64 =VRM4 output carrier lacks vector home facts\n";
    return EXIT_FAILURE;
  }
  if (tied_carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      tied_carrier.operands.size() != 2 ||
      tied_carrier.operands[0].register_group_width != 2 ||
      !tied_carrier.operands[1].tied_home_authority.has_value() ||
      !is_vector_home(tied_carrier.result_home, 2) ||
      !is_vector_home(tied_carrier.operands[1].home, 2) ||
      tied_carrier.operands[1].tied_home_authority->shared_register.bank !=
          prepare::PreparedRegisterBank::Vreg ||
      !tied_carrier.missing_required_facts.empty()) {
    std::cerr << "[FAIL] RV64 tied vector inline asm carrier lacks shared vector authority\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "operand0[kind=register_output,constraint=\"=VRM2\","
                       "class=vector,width=2,output=0",
                       "prepared vector inline asm class/width dump")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "tied_input_output_home_incompatible_register_class",
                           "RV64 tied vector register-class diagnostic")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int rv64_inline_asm_impossible_vector_allocation_diagnoses_missing_home() {
  const auto prepared = prepare_rv64_inline_asm_vector_impossible_carrier_module();
  const auto* function_carriers = find_inline_asm_carriers(
      prepared, "rv64_inline_asm_vector_impossible_carrier_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one impossible RV64 vector inline asm carrier\n";
    return EXIT_FAILURE;
  }
  const auto& carrier = function_carriers->carriers.front();
  if (carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Missing ||
      carrier.missing_required_facts.size() != 1 ||
      carrier.missing_required_facts.front().find("_requires_register_home") ==
          std::string::npos) {
    std::cerr << "[FAIL] impossible RV64 vector inline asm allocation should fail closed with a register-home diagnostic\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "missing fact=inst#0:operand32_requires_register_home",
                       "impossible RV64 vector inline asm missing carrier fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "_requires_register_home",
                       "impossible RV64 vector inline asm register-home diagnostic")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int inline_asm_carriers_fail_closed_without_required_facts() {
  const auto prepared = prepare_inline_asm_fail_closed_dump_module();
  const auto* function_carriers =
      find_inline_asm_carriers(prepared, "inline_asm_fail_closed_dump_contract");
  if (function_carriers == nullptr || function_carriers->carriers.size() != 4 ||
      function_carriers->carriers[0].carrier_kind !=
          prepare::PreparedInlineAsmCarrierKind::Missing ||
      function_carriers->carriers[1].carrier_kind !=
          prepare::PreparedInlineAsmCarrierKind::Complete ||
      function_carriers->carriers[2].carrier_kind !=
          prepare::PreparedInlineAsmCarrierKind::Missing ||
      function_carriers->carriers[3].carrier_kind !=
          prepare::PreparedInlineAsmCarrierKind::Missing) {
    std::cerr << "[FAIL] expected fail-closed inline asm carrier diagnostics\n";
    return EXIT_FAILURE;
  }
  const auto& memory_address_carrier = function_carriers->carriers[0];
  if (memory_address_carrier.operands.size() != 3 ||
      memory_address_carrier.operands[0].kind !=
          bir::InlineAsmOperandKind::MemoryInput ||
      memory_address_carrier.operands[0].arg_index.value_or(99) != 0 ||
      memory_address_carrier.operands[0].memory_address.has_value() ||
      memory_address_carrier.operands[1].kind !=
          bir::InlineAsmOperandKind::AddressInput ||
      memory_address_carrier.operands[1].arg_index.value_or(99) != 1 ||
      memory_address_carrier.operands[1].address.has_value()) {
    std::cerr << "[FAIL] expected structured memory/address inline asm operands\n";
    return EXIT_FAILURE;
  }
  const auto& populated_authority_carrier = function_carriers->carriers[1];
  if (populated_authority_carrier.operands.size() != 2 ||
      !populated_authority_carrier.operands[0].memory_address.has_value() ||
      !populated_authority_carrier.operands[1].address.has_value() ||
      !populated_authority_carrier.missing_required_facts.empty()) {
    std::cerr << "[FAIL] expected selectable populated memory/address authority to be retained\n";
    return EXIT_FAILURE;
  }
  const std::string dump = prepare::print(prepared);
  if (!expect_contains(dump,
                       "missing fact=inst#0:missing_operand0_memory_address_authority",
                       "missing inline asm memory authority diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#0:missing_operand1_address_authority",
                       "missing inline asm address authority diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#0:unsupported_named_operands",
                       "unsupported inline asm named operand diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#0:unsupported_template_modifiers",
                       "unsupported inline asm template modifier diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#0:unsupported_clobber_constraint2",
                       "unsupported inline asm clobber diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#0:unsupported_clobber_operand2",
                       "unsupported inline asm clobber operand diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "inline_asm_carrier asm=\"\" constraints=\"m,p\" "
                       "block_index=0 inst_index=1 side_effects=yes operands=2 "
                       "result_home=no clobbers=0",
                       "complete inline asm memory/address carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "operand0[kind=memory_input,constraint=\"m\",arg=0,"
                       "value=x,memory_address=yes,home=yes]",
                       "selectable inline asm memory operand")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "operand1[kind=address_input,constraint=\"p\",arg=1,"
                       "value=x,address=yes,home=yes]",
                       "selectable inline asm address operand")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "missing fact=inst#1:unsupported_operand",
                           "unsupported inline asm memory/address selection diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#2:unsupported_operand0_memory_address_selection",
                       "invalid inline asm memory address diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#2:unsupported_operand1_address_selection",
                       "invalid inline asm address diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "missing fact=inst#3:missing_operand0_home",
                       "missing inline asm register home diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(dump,
                           "inline_asm_carrier asm=\"%q0 %[src]\"",
                           "incomplete inline asm carrier printer record")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

bir::CallArgAbiInfo scalar_arg_abi(bir::TypeKind type) {
  return *prepare::infer_call_arg_abi(aarch64_target_profile(), type);
}

bir::CallArgAbiInfo vector_arg_abi() {
  return bir::CallArgAbiInfo{
      .type = bir::TypeKind::I128,
      .size_bytes = 16,
      .align_bytes = 16,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_in_register = true,
  };
}

bir::CallResultAbiInfo result_abi(bir::TypeKind type) {
  return bir::CallResultAbiInfo{
      .type = type,
      .primary_class = bir::AbiValueClass::Integer,
  };
}

prepare::PreparedBirModule prepare_crc_intrinsic_carrier_dump_module(
    prepare::PrepareOptions options = prepare::PrepareOptions{},
    bool immediate_accumulator = false) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "crc_intrinsic_carrier_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{.type = bir::TypeKind::I32, .name = "acc"});
  function.params.push_back(bir::Param{.type = bir::TypeKind::I32, .name = "data"});

  bir::CallInst call{
      .result = bir::Value::named(bir::TypeKind::I32, "crc"),
      .callee = "llvm.aarch64.crc32w",
      .args = {immediate_accumulator ? bir::Value::immediate_i32(0)
                                     : bir::Value::named(bir::TypeKind::I32, "acc"),
               bir::Value::named(bir::TypeKind::I32, "data")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32),
                  scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::I32,
      .result_abi = result_abi(bir::TypeKind::I32),
      .intrinsic = bir::IntrinsicOperation{
          .family = bir::IntrinsicFamilyKind::Crc,
          .operation = bir::IntrinsicOperationKind::Crc32W,
          .required_feature = bir::IntrinsicFeatureKind::AArch64Crc,
          .operand_type = bir::TypeKind::I32,
          .result_type = bir::TypeKind::I32,
          .operand_roles = {bir::IntrinsicOperandRole::Accumulator,
                            bir::IntrinsicOperandRole::Data},
          .signedness = bir::IntrinsicSignedness::Unsigned,
          .memory_access = bir::IntrinsicMemoryAccessKind::None,
          .has_side_effects = false,
      },
  };

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "crc"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), options);
}

prepare::PreparedBirModule prepare_vector_load_intrinsic_carrier_dump_module(
    bool complete = true) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = complete ? "vector_load_intrinsic_carrier_dump_contract"
                           : "vector_load_intrinsic_incomplete_carrier_dump_contract";
  function.return_type = bir::TypeKind::I128;
  function.params.push_back(bir::Param{.type = bir::TypeKind::Ptr, .name = "p"});

  bir::CallInst call{
      .result = bir::Value::named(bir::TypeKind::I128, "vec"),
      .callee = "llvm.aarch64.neon.ld1.v16i8.p0i8",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "p")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::Ptr)},
      .return_type = bir::TypeKind::I128,
      .result_abi = result_abi(bir::TypeKind::I128),
      .intrinsic = bir::IntrinsicOperation{
          .family = bir::IntrinsicFamilyKind::VectorMemory,
          .operation = bir::IntrinsicOperationKind::VectorLoad,
          .required_feature = bir::IntrinsicFeatureKind::AArch64Neon,
          .operand_type = bir::TypeKind::Ptr,
          .result_type = bir::TypeKind::I128,
          .operand_roles = {bir::IntrinsicOperandRole::Pointer},
          .vector_element_type = bir::TypeKind::I8,
          .vector_element_width_bytes = 1,
          .vector_lane_count = complete ? std::size_t{16} : std::size_t{8},
          .vector_total_width_bytes = complete ? std::size_t{16} : std::size_t{8},
          .signedness = bir::IntrinsicSignedness::Unsigned,
          .memory_operand = bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_name = "p",
              .size_bytes = 16,
              .align_bytes = 16,
          },
          .memory_access = bir::IntrinsicMemoryAccessKind::Read,
          .has_side_effects = false,
      },
  };

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I128, "vec"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_vector_add_intrinsic_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "vector_add_intrinsic_carrier_dump_contract";
  function.return_type = bir::TypeKind::I128;
  function.params.push_back(bir::Param{.type = bir::TypeKind::I128, .name = "lhs"});
  function.params.push_back(bir::Param{.type = bir::TypeKind::I128, .name = "rhs"});

  bir::CallInst call{
      .result = bir::Value::named(bir::TypeKind::I128, "sum"),
      .callee = "llvm.aarch64.neon.add.v16i8",
      .args = {bir::Value::named(bir::TypeKind::I128, "lhs"),
               bir::Value::named(bir::TypeKind::I128, "rhs")},
      .arg_types = {bir::TypeKind::I128, bir::TypeKind::I128},
      .arg_abi = {vector_arg_abi(), vector_arg_abi()},
      .return_type = bir::TypeKind::I128,
      .result_abi = result_abi(bir::TypeKind::I128),
      .intrinsic = bir::IntrinsicOperation{
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
          .has_side_effects = false,
      },
  };

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I128, "sum"),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_barrier_dmb_intrinsic_carrier_dump_module(
    bool complete = true,
    bool immediate_domain = true,
    bool side_effects = true) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = complete ? "barrier_dmb_intrinsic_carrier_dump_contract"
                           : "barrier_dmb_intrinsic_incomplete_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;

  bir::CallInst call{
      .callee = "llvm.aarch64.dmb",
      .args = {immediate_domain ? bir::Value::immediate_i32(15)
                                : bir::Value::named(bir::TypeKind::I32, "domain")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::Void,
      .intrinsic = bir::IntrinsicOperation{
          .family = bir::IntrinsicFamilyKind::Barrier,
          .operation = bir::IntrinsicOperationKind::BarrierDmb,
          .operand_type = bir::TypeKind::I32,
          .result_type = bir::TypeKind::Void,
          .operand_roles = {bir::IntrinsicOperandRole::BarrierDomain},
          .memory_access = bir::IntrinsicMemoryAccessKind::None,
          .barrier_domain = complete ? bir::IntrinsicBarrierDomainKind::Sy
                                     : bir::IntrinsicBarrierDomainKind::None,
          .has_immediate_operand = immediate_domain,
          .requires_immediate_operand = true,
          .immediate_value = immediate_domain ? std::optional<std::int64_t>{15}
                                             : std::nullopt,
          .has_side_effects = side_effects,
      },
  };

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_cache_dc_cvau_intrinsic_carrier_dump_module(
    bool complete = true,
    bool side_effects = true) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = complete ? "cache_dc_cvau_intrinsic_carrier_dump_contract"
                           : "cache_dc_cvau_intrinsic_incomplete_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{.type = bir::TypeKind::Ptr, .name = "p"});

  bir::CallInst call{
      .callee = "llvm.aarch64.dc.cvau",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "p")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::Ptr)},
      .return_type = bir::TypeKind::Void,
      .intrinsic = bir::IntrinsicOperation{
          .family = bir::IntrinsicFamilyKind::CacheMaintenance,
          .operation = bir::IntrinsicOperationKind::CacheDcCvau,
          .operand_type = complete ? bir::TypeKind::Ptr : bir::TypeKind::I32,
          .result_type = bir::TypeKind::Void,
          .operand_roles = {bir::IntrinsicOperandRole::CacheAddress},
          .memory_operand = bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_name = "p",
              .size_bytes = 0,
              .align_bytes = 1,
              .address_space = bir::AddressSpace::Default,
          },
          .memory_access = bir::IntrinsicMemoryAccessKind::None,
          .has_side_effects = side_effects,
      },
  };

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

prepare::PreparedBirModule prepare_hint_yield_intrinsic_carrier_dump_module(
    bool complete = true,
    bool immediate_hint = true,
    bool side_effects = true) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = complete ? "hint_yield_intrinsic_carrier_dump_contract"
                           : "hint_yield_intrinsic_incomplete_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;

  bir::CallInst call{
      .callee = "llvm.aarch64.hint",
      .args = {immediate_hint ? bir::Value::immediate_i32(1)
                              : bir::Value::named(bir::TypeKind::I32, "hint")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {scalar_arg_abi(bir::TypeKind::I32)},
      .return_type = bir::TypeKind::Void,
      .intrinsic = bir::IntrinsicOperation{
          .family = bir::IntrinsicFamilyKind::PauseHint,
          .operation = bir::IntrinsicOperationKind::HintYield,
          .operand_type = complete ? bir::TypeKind::I32 : bir::TypeKind::Ptr,
          .result_type = bir::TypeKind::Void,
          .operand_roles = {bir::IntrinsicOperandRole::HintImmediate},
          .memory_access = bir::IntrinsicMemoryAccessKind::None,
          .has_immediate_operand = immediate_hint,
          .requires_immediate_operand = true,
          .immediate_value = immediate_hint ? std::optional<std::int64_t>{1}
                                           : std::nullopt,
          .has_side_effects = side_effects,
      },
  };

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), prepare::PrepareOptions{});
}

int crc_and_vector_intrinsic_carriers_preserve_semantic_facts() {
  const auto crc = prepare_crc_intrinsic_carrier_dump_module();
  const auto* crc_carriers =
      find_intrinsic_carriers(crc, "crc_intrinsic_carrier_dump_contract");
  if (crc_carriers == nullptr || crc_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one CRC prepared intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  const auto& crc_carrier = crc_carriers->carriers.front();
  if (crc_carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      crc_carrier.family != bir::IntrinsicFamilyKind::Crc ||
      crc_carrier.operation != bir::IntrinsicOperationKind::Crc32W ||
      crc_carrier.required_feature != bir::IntrinsicFeatureKind::AArch64Crc ||
      crc_carrier.operand_value_names.size() != 2 ||
      crc_carrier.operand_homes.size() != 2 ||
      !crc_carrier.operand_homes[0].has_value() ||
      !crc_carrier.operand_homes[1].has_value() ||
      !crc_carrier.result_home.has_value() ||
      crc_carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] CRC carrier lost required semantic/home fields\n";
    return EXIT_FAILURE;
  }
  const std::string crc_dump = prepare::print(crc);
  if (!expect_contains(crc_dump,
                       "intrinsic_carrier family=crc operation=crc32w "
                       "feature=aarch64_crc block_index=0 inst_index=0 "
                       "operand_type=i32 result_type=i32 roles=accumulator,data "
                       "signedness=unsigned memory_access=none side_effects=no "
                       "requires_feature=yes prepared_call_plan=yes "
                       "source_callee=llvm.aarch64.crc32w operand=acc "
                       "operands=acc,data result=crc operand_homes=2 result_home=yes",
                       "complete CRC carrier")) {
    return EXIT_FAILURE;
  }

  const auto vector_load = prepare_vector_load_intrinsic_carrier_dump_module();
  const auto* load_carriers =
      find_intrinsic_carriers(vector_load, "vector_load_intrinsic_carrier_dump_contract");
  if (load_carriers == nullptr || load_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one vector-load prepared intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  const auto& load_carrier = load_carriers->carriers.front();
  if (load_carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      load_carrier.family != bir::IntrinsicFamilyKind::VectorMemory ||
      load_carrier.operation != bir::IntrinsicOperationKind::VectorLoad ||
      load_carrier.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      load_carrier.vector_element_type != bir::TypeKind::I8 ||
      load_carrier.vector_element_width_bytes != 1 ||
      load_carrier.vector_lane_count != 16 ||
      load_carrier.vector_total_width_bytes != 16 ||
      !load_carrier.memory_operand.has_value() ||
      load_carrier.memory_access != bir::IntrinsicMemoryAccessKind::Read ||
      load_carrier.operand_homes.size() != 1 ||
      !load_carrier.operand_homes[0].has_value() ||
      !load_carrier.result_home.has_value() ||
      load_carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] vector-load carrier lost required semantic/home fields\n";
    return EXIT_FAILURE;
  }
  const std::string load_dump = prepare::print(vector_load);
  if (!expect_contains(load_dump,
                       "intrinsic_carrier family=vector_memory operation=vector_load "
                       "feature=aarch64_neon block_index=0 inst_index=0 "
                       "operand_type=ptr result_type=i128 roles=pointer "
                       "vector_element_type=i8 vector_element_width=1 vector_lanes=16 "
                       "vector_width=16 signedness=unsigned memory_access=read "
                       "memory_size=16 memory_align=16 memory_address_space=default "
                       "memory_volatile=no "
                       "side_effects=no requires_feature=yes prepared_call_plan=yes "
                       "source_callee=llvm.aarch64.neon.ld1.v16i8.p0i8 operand=p "
                       "result=vec operand_homes=1 result_home=yes",
                       "complete vector-load carrier")) {
    return EXIT_FAILURE;
  }

  const auto vector_add = prepare_vector_add_intrinsic_carrier_dump_module();
  const auto* add_carriers =
      find_intrinsic_carriers(vector_add, "vector_add_intrinsic_carrier_dump_contract");
  if (add_carriers == nullptr || add_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one vector-add prepared intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  const auto& add_carrier = add_carriers->carriers.front();
  if (add_carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      add_carrier.family != bir::IntrinsicFamilyKind::VectorOperation ||
      add_carrier.operation != bir::IntrinsicOperationKind::VectorAdd ||
      add_carrier.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      add_carrier.operand_roles.size() != 2 ||
      add_carrier.vector_total_width_bytes != 16 ||
      add_carrier.operand_homes.size() != 2 ||
      !add_carrier.operand_homes[0].has_value() ||
      !add_carrier.operand_homes[1].has_value() ||
      !add_carrier.result_home.has_value() ||
      add_carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] vector-add carrier lost required semantic/home fields\n";
    return EXIT_FAILURE;
  }

  const std::string dump = prepare::print(vector_add);
  if (!expect_contains(dump,
                       "intrinsic_carrier family=vector_operation operation=vector_add "
                       "feature=aarch64_neon block_index=0 inst_index=0 "
                       "operand_type=i128 result_type=i128 roles=vector_lhs,vector_rhs "
                       "vector_element_type=i8 vector_element_width=1 vector_lanes=16 "
                       "vector_width=16 signedness=unsigned memory_access=none "
                       "side_effects=no requires_feature=yes prepared_call_plan=yes "
                       "source_callee=llvm.aarch64.neon.add.v16i8 operand=lhs "
                       "operands=lhs,rhs result=sum operand_homes=2 result_home=yes",
                       "complete vector-add carrier")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int barrier_dmb_intrinsic_carrier_preserves_semantic_facts() {
  const auto barrier = prepare_barrier_dmb_intrinsic_carrier_dump_module();
  const auto* barrier_carriers =
      find_intrinsic_carriers(barrier, "barrier_dmb_intrinsic_carrier_dump_contract");
  if (barrier_carriers == nullptr || barrier_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one barrier DMB prepared intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  const auto& carrier = barrier_carriers->carriers.front();
  if (carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      carrier.family != bir::IntrinsicFamilyKind::Barrier ||
      carrier.operation != bir::IntrinsicOperationKind::BarrierDmb ||
      carrier.required_feature != bir::IntrinsicFeatureKind::None ||
      carrier.operand_type != bir::TypeKind::I32 ||
      carrier.result_type != bir::TypeKind::Void ||
      carrier.operand_roles.size() != 1 ||
      carrier.operand_roles.front() != bir::IntrinsicOperandRole::BarrierDomain ||
      carrier.barrier_domain != bir::IntrinsicBarrierDomainKind::Sy ||
      !carrier.has_immediate_operand || !carrier.requires_immediate_operand ||
      !carrier.immediate_value.has_value() || *carrier.immediate_value != 15 ||
      carrier.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !carrier.has_side_effects ||
      carrier.result.has_value() ||
      carrier.result_home.has_value() ||
      carrier.operand_homes.size() != 1 ||
      carrier.operand_homes.front().has_value() ||
      !carrier.has_prepared_call_plan ||
      carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] barrier DMB carrier lost required semantic facts\n";
    return EXIT_FAILURE;
  }
  const std::string dump = prepare::print(barrier);
  if (!expect_contains(dump,
                       "intrinsic_carrier family=barrier operation=barrier_dmb "
                       "feature=none block_index=0 inst_index=0 operand_type=i32 "
                       "result_type=void roles=barrier_domain signedness=none "
                       "memory_access=none barrier_domain=sy immediate=15 "
                       "side_effects=yes requires_feature=no prepared_call_plan=yes "
                       "source_callee=llvm.aarch64.dmb operand_homes=1 result_home=no",
                       "complete barrier DMB carrier")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int cache_dc_cvau_intrinsic_carrier_preserves_semantic_facts() {
  const auto cache = prepare_cache_dc_cvau_intrinsic_carrier_dump_module();
  const auto* cache_carriers =
      find_intrinsic_carriers(cache, "cache_dc_cvau_intrinsic_carrier_dump_contract");
  if (cache_carriers == nullptr || cache_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one cache DC CVAU prepared intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  const auto& carrier = cache_carriers->carriers.front();
  if (carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      carrier.family != bir::IntrinsicFamilyKind::CacheMaintenance ||
      carrier.operation != bir::IntrinsicOperationKind::CacheDcCvau ||
      carrier.required_feature != bir::IntrinsicFeatureKind::None ||
      carrier.operand_type != bir::TypeKind::Ptr ||
      carrier.result_type != bir::TypeKind::Void ||
      carrier.operand_roles.size() != 1 ||
      carrier.operand_roles.front() != bir::IntrinsicOperandRole::CacheAddress ||
      !carrier.memory_operand.has_value() ||
      carrier.memory_operand->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      carrier.memory_operand->address_space != bir::AddressSpace::Default ||
      carrier.memory_operand->size_bytes != 0 ||
      carrier.memory_operand->align_bytes != 1 ||
      carrier.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !carrier.has_side_effects ||
      carrier.result.has_value() ||
      carrier.result_home.has_value() ||
      carrier.operand_homes.size() != 1 ||
      !carrier.operand_homes.front().has_value() ||
      !carrier.has_prepared_call_plan ||
      carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] cache DC CVAU carrier lost required semantic facts\n";
    return EXIT_FAILURE;
  }
  const std::string dump = prepare::print(cache);
  if (!expect_contains(dump,
                       "intrinsic_carrier family=cache_maintenance "
                       "operation=cache_dc_cvau feature=none block_index=0 "
                       "inst_index=0 operand_type=ptr result_type=void "
                       "roles=cache_address signedness=none memory_access=none "
                       "memory_size=0 memory_align=1 memory_address_space=default "
                       "memory_volatile=no side_effects=yes requires_feature=no "
                       "prepared_call_plan=yes source_callee=llvm.aarch64.dc.cvau "
                       "operand=p operand_homes=1 result_home=no",
                       "complete cache DC CVAU carrier")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int hint_yield_intrinsic_carrier_preserves_semantic_facts() {
  const auto hint = prepare_hint_yield_intrinsic_carrier_dump_module();
  const auto* hint_carriers =
      find_intrinsic_carriers(hint, "hint_yield_intrinsic_carrier_dump_contract");
  if (hint_carriers == nullptr || hint_carriers->carriers.size() != 1) {
    std::cerr << "[FAIL] expected one hint yield prepared intrinsic carrier\n";
    return EXIT_FAILURE;
  }
  const auto& carrier = hint_carriers->carriers.front();
  if (carrier.carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete ||
      carrier.family != bir::IntrinsicFamilyKind::PauseHint ||
      carrier.operation != bir::IntrinsicOperationKind::HintYield ||
      carrier.required_feature != bir::IntrinsicFeatureKind::None ||
      carrier.operand_type != bir::TypeKind::I32 ||
      carrier.result_type != bir::TypeKind::Void ||
      carrier.operand_roles.size() != 1 ||
      carrier.operand_roles.front() != bir::IntrinsicOperandRole::HintImmediate ||
      !carrier.has_immediate_operand || !carrier.requires_immediate_operand ||
      !carrier.immediate_value.has_value() || *carrier.immediate_value != 1 ||
      carrier.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      carrier.barrier_domain != bir::IntrinsicBarrierDomainKind::None ||
      !carrier.has_side_effects ||
      carrier.result.has_value() ||
      carrier.result_home.has_value() ||
      carrier.operand_homes.size() != 1 ||
      carrier.operand_homes.front().has_value() ||
      !carrier.has_prepared_call_plan ||
      carrier.missing_required_facts.empty() == false) {
    std::cerr << "[FAIL] hint yield carrier lost required semantic facts\n";
    return EXIT_FAILURE;
  }
  const std::string dump = prepare::print(hint);
  if (!expect_contains(dump,
                       "intrinsic_carrier family=pause_hint operation=hint_yield "
                       "feature=none block_index=0 inst_index=0 operand_type=i32 "
                       "result_type=void roles=hint_immediate signedness=none "
                       "memory_access=none immediate=1 side_effects=yes "
                       "requires_feature=no prepared_call_plan=yes "
                       "source_callee=llvm.aarch64.hint operand_homes=1 result_home=no",
                       "complete hint yield carrier")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int crc_and_vector_intrinsic_carriers_fail_closed_without_required_facts() {
  const auto crc_without_homes = prepare_crc_intrinsic_carrier_dump_module(
      prepare::PrepareOptions{}, true);
  const auto* crc_carriers =
      find_intrinsic_carriers(crc_without_homes, "crc_intrinsic_carrier_dump_contract");
  if (crc_carriers == nullptr ||
      crc_carriers->carriers.size() != 1 ||
      crc_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing ||
      !crc_carriers->carriers.front().has_prepared_call_plan) {
    std::cerr << "[FAIL] CRC carrier should remain missing with call plan but no homes\n";
    return EXIT_FAILURE;
  }
  const std::string crc_dump = prepare::print(crc_without_homes);
  if (!expect_contains(crc_dump,
                       "missing fact=inst#0:missing_operand0_home",
                       "CRC missing operand home diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(crc_dump,
                           "intrinsic_carrier family=crc operation=crc32w",
                           "incomplete CRC carrier printer record")) {
    return EXIT_FAILURE;
  }

  const auto bad_vector_load = prepare_vector_load_intrinsic_carrier_dump_module(false);
  const auto* bad_load_carriers = find_intrinsic_carriers(
      bad_vector_load, "vector_load_intrinsic_incomplete_carrier_dump_contract");
  if (bad_load_carriers == nullptr ||
      bad_load_carriers->carriers.size() != 1 ||
      bad_load_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] malformed vector-load carrier should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string bad_load_dump = prepare::print(bad_vector_load);
  if (!expect_contains(bad_load_dump,
                       "missing fact=inst#0:vector_load_requires_v16i8_shape",
                       "vector-load malformed shape diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(bad_load_dump,
                           "intrinsic_carrier family=vector_memory operation=vector_load",
                           "incomplete vector-load carrier printer record")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int cache_dc_cvau_intrinsic_carriers_fail_closed_without_required_facts() {
  const auto wrong_type = prepare_cache_dc_cvau_intrinsic_carrier_dump_module(false, true);
  const auto* wrong_type_carriers = find_intrinsic_carriers(
      wrong_type, "cache_dc_cvau_intrinsic_incomplete_carrier_dump_contract");
  if (wrong_type_carriers == nullptr ||
      wrong_type_carriers->carriers.size() != 1 ||
      wrong_type_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] malformed cache DC CVAU type should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string wrong_type_dump = prepare::print(wrong_type);
  if (!expect_contains(wrong_type_dump,
                       "missing fact=inst#0:cache_dc_cvau_requires_ptr_to_void_types",
                       "cache DC CVAU malformed type diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(wrong_type_dump,
                           "intrinsic_carrier family=cache_maintenance operation=cache_dc_cvau",
                           "incomplete cache DC CVAU carrier printer record")) {
    return EXIT_FAILURE;
  }

  const auto no_side_effects =
      prepare_cache_dc_cvau_intrinsic_carrier_dump_module(true, false);
  const auto* no_side_effect_carriers = find_intrinsic_carriers(
      no_side_effects, "cache_dc_cvau_intrinsic_carrier_dump_contract");
  if (no_side_effect_carriers == nullptr ||
      no_side_effect_carriers->carriers.size() != 1 ||
      no_side_effect_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] cache DC CVAU without side effects should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string no_side_effect_dump = prepare::print(no_side_effects);
  if (!expect_contains(no_side_effect_dump,
                       "missing fact=inst#0:cache_dc_cvau_requires_address_side_effect_facts",
                       "cache DC CVAU missing side effect diagnostic")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int hint_yield_intrinsic_carriers_fail_closed_without_required_facts() {
  const auto wrong_type =
      prepare_hint_yield_intrinsic_carrier_dump_module(false, true, true);
  const auto* wrong_type_carriers = find_intrinsic_carriers(
      wrong_type, "hint_yield_intrinsic_incomplete_carrier_dump_contract");
  if (wrong_type_carriers == nullptr ||
      wrong_type_carriers->carriers.size() != 1 ||
      wrong_type_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] malformed hint yield type should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string wrong_type_dump = prepare::print(wrong_type);
  if (!expect_contains(wrong_type_dump,
                       "missing fact=inst#0:hint_yield_requires_i32_to_void_types",
                       "hint yield malformed type diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(wrong_type_dump,
                           "intrinsic_carrier family=pause_hint operation=hint_yield",
                           "incomplete hint yield carrier printer record")) {
    return EXIT_FAILURE;
  }

  const auto missing_immediate =
      prepare_hint_yield_intrinsic_carrier_dump_module(true, false, true);
  const auto* missing_immediate_carriers = find_intrinsic_carriers(
      missing_immediate, "hint_yield_intrinsic_carrier_dump_contract");
  if (missing_immediate_carriers == nullptr ||
      missing_immediate_carriers->carriers.size() != 1 ||
      missing_immediate_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] non-immediate hint yield should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string missing_immediate_dump = prepare::print(missing_immediate);
  if (!expect_contains(missing_immediate_dump,
                       "missing fact=inst#0:hint_yield_requires_immediate_1",
                       "hint yield missing immediate diagnostic")) {
    return EXIT_FAILURE;
  }

  const auto no_side_effects =
      prepare_hint_yield_intrinsic_carrier_dump_module(true, true, false);
  const auto* no_side_effect_carriers = find_intrinsic_carriers(
      no_side_effects, "hint_yield_intrinsic_carrier_dump_contract");
  if (no_side_effect_carriers == nullptr ||
      no_side_effect_carriers->carriers.size() != 1 ||
      no_side_effect_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] hint yield without side effects should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string no_side_effect_dump = prepare::print(no_side_effects);
  if (!expect_contains(no_side_effect_dump,
                       "missing fact=inst#0:hint_yield_requires_side_effect_only_semantics",
                       "hint yield missing side effect diagnostic")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int barrier_dmb_intrinsic_carriers_fail_closed_without_required_facts() {
  const auto missing_domain =
      prepare_barrier_dmb_intrinsic_carrier_dump_module(false, true, true);
  const auto* missing_domain_carriers = find_intrinsic_carriers(
      missing_domain, "barrier_dmb_intrinsic_incomplete_carrier_dump_contract");
  if (missing_domain_carriers == nullptr ||
      missing_domain_carriers->carriers.size() != 1 ||
      missing_domain_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] malformed barrier domain should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string missing_domain_dump = prepare::print(missing_domain);
  if (!expect_contains(missing_domain_dump,
                       "missing fact=inst#0:barrier_dmb_requires_sy_domain",
                       "barrier missing domain diagnostic")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(missing_domain_dump,
                           "intrinsic_carrier family=barrier operation=barrier_dmb",
                           "incomplete barrier carrier printer record")) {
    return EXIT_FAILURE;
  }

  const auto missing_immediate =
      prepare_barrier_dmb_intrinsic_carrier_dump_module(true, false, true);
  const auto* missing_immediate_carriers = find_intrinsic_carriers(
      missing_immediate, "barrier_dmb_intrinsic_carrier_dump_contract");
  if (missing_immediate_carriers == nullptr ||
      missing_immediate_carriers->carriers.size() != 1 ||
      missing_immediate_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] non-immediate barrier domain should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string missing_immediate_dump = prepare::print(missing_immediate);
  if (!expect_contains(missing_immediate_dump,
                       "missing fact=inst#0:barrier_dmb_requires_immediate_15",
                       "barrier missing immediate diagnostic")) {
    return EXIT_FAILURE;
  }

  const auto no_side_effects =
      prepare_barrier_dmb_intrinsic_carrier_dump_module(true, true, false);
  const auto* no_side_effect_carriers = find_intrinsic_carriers(
      no_side_effects, "barrier_dmb_intrinsic_carrier_dump_contract");
  if (no_side_effect_carriers == nullptr ||
      no_side_effect_carriers->carriers.size() != 1 ||
      no_side_effect_carriers->carriers.front().carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Missing) {
    std::cerr << "[FAIL] barrier without side effects should remain missing\n";
    return EXIT_FAILURE;
  }
  const std::string no_side_effect_dump = prepare::print(no_side_effects);
  if (!expect_contains(no_side_effect_dump,
                       "missing fact=inst#0:barrier_dmb_requires_side_effect_only_semantics",
                       "barrier missing side effect diagnostic")) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

prepare::PreparedControlFlowFunction* find_control_flow_function(
    prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const c4c::FunctionNameId function_name_id =
      prepared.names.function_names.find(function_name);
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  for (auto& function : prepared.control_flow.functions) {
    if (function.function_name == function_name_id) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedParallelCopyBundle* find_parallel_copy_bundle(
    prepare::PreparedBirModule& prepared,
    const char* function_name,
    const char* predecessor_label,
    const char* successor_label) {
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return nullptr;
  }
  const auto predecessor_id = prepared.names.block_labels.find(predecessor_label);
  const auto successor_id = prepared.names.block_labels.find(successor_label);
  if (predecessor_id == c4c::kInvalidBlockLabel ||
      successor_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  for (auto& bundle : control_flow->parallel_copy_bundles) {
    if (bundle.predecessor_label == predecessor_id &&
        bundle.successor_label == successor_id) {
      return &bundle;
    }
  }
  return nullptr;
}

bir::Function* find_function(prepare::PreparedBirModule& prepared, const char* function_name) {
  for (auto& function : prepared.module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

bir::Block* find_block(bir::Function& function, const char* block_label) {
  for (auto& block : function.blocks) {
    if (block.label == block_label) {
      return &block;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule prepare_i128_runtime_helper_mapping_dump_module() {
  prepare::PreparedBirModule seeded;
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "i128_runtime_helper_mapping_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I128,
      .name = "lhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I128,
      .name = "rhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "fp",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::SDiv,
      .result = bir::Value::named(bir::TypeKind::I128, "wide.div"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "rhs"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::URem,
      .result = bir::Value::named(bir::TypeKind::I128, "wide.rem"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "wide.div"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "rhs"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPToSI,
      .result = bir::Value::named(bir::TypeKind::I128, "from.fp"),
      .operand = bir::Value::named(bir::TypeKind::F64, "fp"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::UIToFP,
      .result = bir::Value::named(bir::TypeKind::F64, "to.fp"),
      .operand = bir::Value::named(bir::TypeKind::I128, "rhs"),
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  seeded.module = std::move(module);
  seeded.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  for (const std::string_view value_name : {"lhs", "rhs", "wide.div", "wide.rem", "from.fp"}) {
    set_register_group_override(prepared,
                                "i128_runtime_helper_mapping_dump_contract",
                                value_name,
                                prepare::PreparedRegisterClass::General,
                                2);
  }

  options.run_legalize = false;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(prepared), options);
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_f128_memory_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "f128_memory_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "p.value",
      .size_bytes = 16,
      .align_bytes = 16,
      .is_byval = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepare_f128_register_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "f128_register_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "p.value",
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepare_f128_constant_payload_dump_module() {
  constexpr std::uint64_t low_bits = 0x0123456789abcdefULL;
  constexpr std::uint64_t high_bits = 0x3fff800000000000ULL;

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
  function.name = "f128_constant_payload_dump_contract";
  function.return_type = bir::TypeKind::Void;
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_tf",
      .args = {bir::Value::immediate_f128_bits(low_bits, high_bits)},
      .arg_types = {bir::TypeKind::F128},
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule manual_f128_constant_payload_dump_module() {
  prepare::PreparedBirModule prepared;
  const auto function_name =
      prepared.names.function_names.intern("manual_f128_constant_payload_dump_contract");
  const auto value_name = prepared.names.value_names.intern("const.tf");
  constexpr std::uint64_t low_bits = 0x0123456789abcdefULL;
  constexpr std::uint64_t high_bits = 0x3fff800000000000ULL;

  bir::Function function;
  function.name = "manual_f128_constant_payload_dump_contract";
  function.return_type = bir::TypeKind::F128;
  function.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{
          .value = bir::Value::immediate_f128_bits(low_bits, high_bits),
      },
  });
  prepared.module.functions.push_back(std::move(function));

  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers =
          {
              prepare::PreparedF128Carrier{
                  .function_name = function_name,
                  .value_id = prepare::PreparedValueId{42},
                  .value_name = value_name,
                  .source_type = bir::TypeKind::F128,
                  .kind = prepare::PreparedF128CarrierKind::Missing,
                  .total_size_bytes = 16,
                  .total_align_bytes = 16,
                  .constant_payload = bir::Value::F128Payload{
                      .low_bits = low_bits,
                      .high_bits = high_bits,
                  },
              },
          },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .instruction_index = 3,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .direct_callee_name = std::string{"consume_tf"},
                  .arguments =
                      {
                          prepare::PreparedCallArgumentPlan{
                              .arg_index = 0,
                              .value_bank = prepare::PreparedRegisterBank::Vreg,
                              .source_encoding =
                                  prepare::PreparedStorageEncodingKind::Immediate,
                              .source_value_id = prepare::PreparedValueId{42},
                              .source_literal =
                                  bir::Value::immediate_f128_bits(low_bits, high_bits),
                          },
                      },
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule prepare_f128_soft_float_helper_dump_module(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Add,
    std::string function_name_spelling = "f128_soft_float_helper_dump_contract",
    std::string result_name_spelling = "sum",
    bool rhs_constant = false) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = std::move(function_name_spelling);
  function.return_type = bir::TypeKind::F128;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "lhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "rhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = opcode,
      .result = bir::Value::named(bir::TypeKind::F128, result_name_spelling),
      .operand_type = bir::TypeKind::F128,
      .lhs = bir::Value::named(bir::TypeKind::F128, "lhs"),
      .rhs = rhs_constant
                 ? bir::Value::immediate_f128_bits(0x0123456789abcdefULL,
                                                   0x3fff800000000000ULL)
                 : bir::Value::named(bir::TypeKind::F128, "rhs"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F128, result_name_spelling),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepare_f128_cast_helper_dump_module(
    bir::CastOpcode opcode,
    bir::TypeKind source_type,
    bir::TypeKind result_type,
    std::string function_name_spelling,
    std::string result_name_spelling) {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = std::move(function_name_spelling);
  function.return_type = result_type;
  function.params.push_back(bir::Param{
      .type = source_type,
      .name = "input",
      .size_bytes = source_type == bir::TypeKind::F128 ? std::size_t{16}
                                                       : (source_type == bir::TypeKind::F64
                                                              ? std::size_t{8}
                                                              : std::size_t{4}),
      .align_bytes = source_type == bir::TypeKind::F128 ? std::size_t{16}
                                                        : (source_type == bir::TypeKind::F64
                                                               ? std::size_t{8}
                                                               : std::size_t{4}),
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = opcode,
      .result = bir::Value::named(result_type, result_name_spelling),
      .operand = bir::Value::named(source_type, "input"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(result_type, result_name_spelling),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

}  // namespace

int main() {
  if (const int status = complete_module_body_text_printer_rows_are_byte_stable();
      status != 0) {
    return status;
  }
  if (const int status = atomic_carrier_facts_preserve_fields_and_printer_visibility();
      status != 0) {
    return status;
  }
  if (const int status = incomplete_atomic_carrier_facts_fail_closed(); status != 0) {
    return status;
  }
  if (const int status =
          scalar_fp_unary_intrinsic_carrier_preserves_fields_and_printer_visibility();
      status != 0) {
    return status;
  }
  if (const int status = partial_and_unsupported_intrinsic_carriers_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          inline_asm_carriers_preserve_supported_facts_and_printer_visibility();
      status != 0) {
    return status;
  }
  if (const int status =
          rv64_inline_asm_carriers_preserve_scalar_register_identities();
      status != 0) {
    return status;
  }
  if (const int status =
          rv64_insn_r_structured_scalar_carriers_bind_gpr_operands();
      status != 0) {
    return status;
  }
  if (const int status =
          rv64_inline_asm_carriers_preserve_vector_register_group_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          rv64_inline_asm_impossible_vector_allocation_diagnoses_missing_home();
      status != 0) {
    return status;
  }
  if (const int status = inline_asm_carriers_fail_closed_without_required_facts();
      status != 0) {
    return status;
  }
  if (const int status = crc_and_vector_intrinsic_carriers_preserve_semantic_facts();
      status != 0) {
    return status;
  }
  if (const int status = barrier_dmb_intrinsic_carrier_preserves_semantic_facts();
      status != 0) {
    return status;
  }
  if (const int status = cache_dc_cvau_intrinsic_carrier_preserves_semantic_facts();
      status != 0) {
    return status;
  }
  if (const int status = hint_yield_intrinsic_carrier_preserves_semantic_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          crc_and_vector_intrinsic_carriers_fail_closed_without_required_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          cache_dc_cvau_intrinsic_carriers_fail_closed_without_required_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          hint_yield_intrinsic_carriers_fail_closed_without_required_facts();
      status != 0) {
    return status;
  }
  if (const int status =
          barrier_dmb_intrinsic_carriers_fail_closed_without_required_facts();
      status != 0) {
    return status;
  }

  auto prepared = legalize_short_circuit_or_guard_module();
  const std::string dump = prepare::print(prepared);

  if (!expect_contains(dump, "prepared.module target=riscv64-unknown-linux-gnu",
                       "module header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "--- prepared-bir ---", "prepared bir section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "prepared.func @short_circuit_or_prepare_contract",
                       "prepared function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "branch_condition entry", "branch condition entry")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "join_transfer logic.end.10", "join transfer")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "carrier=select_materialization",
                       "select materialization carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "ownership=authoritative_branch_pair incomings=2 edge_transfers=2",
                       "join transfer ownership summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "source_branch=entry", "source branch ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "source_transfer_indexes=(0, 1)",
                       "join transfer source transfer indexes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "continuation_targets=(block_1, block_2)",
                       "join transfer continuation targets")) {
    return EXIT_FAILURE;
  }

  auto* control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  auto* function = find_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow == nullptr || function == nullptr) {
    std::cerr << "[FAIL] missing prepared short-circuit control-flow fixture\n";
    return EXIT_FAILURE;
  }
  auto* join_block = find_block(*function, "logic.end.10");
  if (join_block == nullptr) {
    std::cerr << "[FAIL] missing prepared short-circuit join block fixture\n";
    return EXIT_FAILURE;
  }
  join_block->insts.clear();
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return prepare::prepared_block_label(prepared.names,
                                                            branch_condition.block_label) !=
                              "entry";
                     }),
      control_flow->branch_conditions.end());

  const std::string published_dump = prepare::print(prepared);
  if (!expect_contains(published_dump, "continuation_targets=(block_1, block_2)",
                       "published join transfer continuation targets")) {
    return EXIT_FAILURE;
  }

  auto* mutable_control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.empty()) {
    std::cerr << "[FAIL] missing mutable authoritative join transfer for printer contract\n";
    return EXIT_FAILURE;
  }
  mutable_control_flow->join_transfers.front().continuation_true_label.reset();
  mutable_control_flow->join_transfers.front().continuation_false_label.reset();

  const std::string unpublished_dump = prepare::print(prepared);
  if (!expect_not_contains(unpublished_dump, "continuation_targets=(",
                           "recomputed continuation targets after removing publication")) {
    return EXIT_FAILURE;
  }

  const auto loop_prepared = legalize_loop_countdown_module();
  const std::string loop_dump = prepare::print(loop_prepared);
  if (!expect_contains(loop_dump, "prepared.func @loop_countdown_prepare_contract",
                       "loop printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "join_transfer loop result=counter kind=loop_carry carrier=edge_store_slot",
                       "loop-carry join header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "ownership=per_edge incomings=2 edge_transfers=2 storage=counter.phi",
                       "slot-backed loop-carry authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump, "incoming [entry] -> 3", "loop entry incoming")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump, "incoming [body] -> next", "loop backedge incoming")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "edge_transfer body -> loop incoming=next destination=counter storage=counter.phi",
                       "loop backedge edge transfer")) {
    return EXIT_FAILURE;
  }

  auto parallel_copy_prepared = legalize_parallel_copy_cycle_module();
  const std::string parallel_copy_dump = prepare::print(parallel_copy_prepared);
  if (!expect_contains(parallel_copy_dump, "prepared.func @parallel_copy_prepare_contract",
                       "parallel-copy printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy entry -> loop execution_site=predecessor_terminator execution_block=entry has_cycle=no resolution=acyclic moves=2 steps=2",
                       "acyclic parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy body -> loop execution_site=predecessor_terminator execution_block=body has_cycle=yes resolution=cycle_break moves=2 steps=3",
                       "cycle-break parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "authority execution_site=predecessor_terminator execution_block=entry",
                       "acyclic parallel-copy authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "authority execution_site=predecessor_terminator execution_block=body",
                       "cycle-break parallel-copy authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[0] save_destination_to_temp move_index=0 save_destination=a blocked_source=b temp_source=cycle_temp(a) carrier=edge_store_slot storage=a.phi uses_cycle_temp_source=no",
                       "cycle temp save step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[1] move move_index=0 source=b destination=a carrier=edge_store_slot storage=a.phi uses_cycle_temp_source=no",
                       "non-temp move step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[2] move move_index=1 source=cycle_temp(a) destination=b carrier=edge_store_slot storage=b.phi uses_cycle_temp_source=yes",
                       "temp-fed move step")) {
    return EXIT_FAILURE;
  }
  auto* mutable_body_bundle = find_parallel_copy_bundle(
      parallel_copy_prepared, "parallel_copy_prepare_contract", "body", "loop");
  if (mutable_body_bundle == nullptr) {
    std::cerr << "[FAIL] missing mutable backedge bundle for printer authority contract\n";
    return EXIT_FAILURE;
  }
  mutable_body_bundle->execution_block_label.reset();
  const std::string unpublished_parallel_copy_dump = prepare::print(parallel_copy_prepared);
  if (!expect_contains(unpublished_parallel_copy_dump,
                       "authority execution_site=predecessor_terminator execution_block=<none>",
                       "parallel-copy authority after removing publication")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(unpublished_parallel_copy_dump,
                           "authority execution_site=predecessor_terminator execution_block=body",
                           "recomputed parallel-copy execution block after removing publication")) {
    return EXIT_FAILURE;
  }

  const auto block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module();
  const std::string block_entry_publication_dump =
      prepare::print(block_entry_publication_prepared);
  const std::string block_entry_publication_row =
      "block_entry_publication successor=join status=available "
      "to_value_id=42 to=published home_kind=register "
      "destination_kind=value destination_storage=register "
      "reg=r9 block_index=3 instruction_index=5";
  if (!expect_contains(block_entry_publication_dump,
                       block_entry_publication_row,
                       "block-entry publication prepared-printer row")) {
    return EXIT_FAILURE;
  }
  const auto prepared_only_agreement =
      find_block_entry_publication_printer_row_agreement(
          block_entry_publication_prepared);
  if (prepared_only_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      prepared_only_agreement.route4_block_entry_publication_attributed) {
    std::cerr << "[FAIL] prepared-only block-entry printer row should fall back without Route 4 attribution\n";
    return EXIT_FAILURE;
  }

  const auto route4_block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module(
          BlockEntryPublicationRouteEvidence::Agreeing);
  const auto route4_block_entry_publication_agreement =
      find_block_entry_publication_printer_row_agreement(
          route4_block_entry_publication_prepared);
  const std::string route4_block_entry_publication_dump =
      prepare::print(route4_block_entry_publication_prepared);
  if (route4_block_entry_publication_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      !route4_block_entry_publication_agreement
           .route4_block_entry_publication_attributed ||
      route4_block_entry_publication_agreement
              .route4_block_entry_publication_instruction_index !=
          std::size_t{5}) {
    std::cerr << "[FAIL] agreeing Route 4 evidence should attribute the block-entry printer row through the prepared lookup boundary\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(route4_block_entry_publication_dump,
                       block_entry_publication_row,
                       "Route 4 attributed block-entry publication row text")) {
    return EXIT_FAILURE;
  }

  const auto missing_phi_block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module(
          BlockEntryPublicationRouteEvidence::MissingPhi);
  const auto missing_phi_block_entry_publication_agreement =
      find_block_entry_publication_printer_row_agreement(
          missing_phi_block_entry_publication_prepared);
  const std::string missing_phi_block_entry_publication_dump =
      prepare::print(missing_phi_block_entry_publication_prepared);
  if (missing_phi_block_entry_publication_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      missing_phi_block_entry_publication_agreement
          .route4_block_entry_publication_attributed) {
    std::cerr << "[FAIL] missing-PHI Route 4 evidence should preserve the prepared block-entry printer row without attribution\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(missing_phi_block_entry_publication_dump,
                       block_entry_publication_row,
                       "missing-PHI fallback block-entry publication row text")) {
    return EXIT_FAILURE;
  }

  const auto wrong_destination_block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module(
          BlockEntryPublicationRouteEvidence::WrongDestination);
  const auto wrong_destination_block_entry_publication_agreement =
      find_block_entry_publication_printer_row_agreement(
          wrong_destination_block_entry_publication_prepared);
  const std::string wrong_destination_block_entry_publication_dump =
      prepare::print(wrong_destination_block_entry_publication_prepared);
  if (wrong_destination_block_entry_publication_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      wrong_destination_block_entry_publication_agreement
          .route4_block_entry_publication_attributed) {
    std::cerr << "[FAIL] wrong-destination Route 4 evidence should preserve the prepared block-entry printer row without attribution\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          wrong_destination_block_entry_publication_dump,
          block_entry_publication_row,
          "wrong-destination fallback block-entry publication row text")) {
    return EXIT_FAILURE;
  }

  const auto wrong_successor_block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module(
          BlockEntryPublicationRouteEvidence::WrongSuccessorBlock);
  const auto wrong_successor_block_entry_publication_agreement =
      find_block_entry_publication_printer_row_agreement(
          wrong_successor_block_entry_publication_prepared);
  const std::string wrong_successor_block_entry_publication_dump =
      prepare::print(wrong_successor_block_entry_publication_prepared);
  if (wrong_successor_block_entry_publication_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      wrong_successor_block_entry_publication_agreement
          .route4_block_entry_publication_attributed) {
    std::cerr << "[FAIL] wrong-successor Route 4 evidence should preserve the prepared block-entry printer row without attribution\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          wrong_successor_block_entry_publication_dump,
          block_entry_publication_row,
          "wrong-successor fallback block-entry publication row text")) {
    return EXIT_FAILURE;
  }

  const auto wrong_index_block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module(
          BlockEntryPublicationRouteEvidence::WrongInstructionIndex);
  const auto wrong_index_block_entry_publication_agreement =
      find_block_entry_publication_printer_row_agreement(
          wrong_index_block_entry_publication_prepared);
  const std::string wrong_index_block_entry_publication_dump =
      prepare::print(wrong_index_block_entry_publication_prepared);
  if (wrong_index_block_entry_publication_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      wrong_index_block_entry_publication_agreement
          .route4_block_entry_publication_attributed) {
    std::cerr << "[FAIL] wrong-instruction Route 4 evidence should preserve the prepared block-entry printer row without attribution\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(wrong_index_block_entry_publication_dump,
                       block_entry_publication_row,
                       "wrong-instruction fallback block-entry publication row text")) {
    return EXIT_FAILURE;
  }

  const auto duplicate_block_entry_publication_prepared =
      prepared_block_entry_publication_printer_row_module(
          BlockEntryPublicationRouteEvidence::Duplicate);
  const auto duplicate_block_entry_publication_agreement =
      find_block_entry_publication_printer_row_agreement(
          duplicate_block_entry_publication_prepared);
  const std::string duplicate_block_entry_publication_dump =
      prepare::print(duplicate_block_entry_publication_prepared);
  if (duplicate_block_entry_publication_agreement.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      duplicate_block_entry_publication_agreement
          .route4_block_entry_publication_attributed ||
      duplicate_block_entry_publication_agreement
              .route4_block_entry_publication_status !=
          bir::RouteIndexValidationStatus::DuplicateReference) {
    std::cerr << "[FAIL] duplicate Route 4 evidence should preserve the prepared block-entry printer row without attribution\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(duplicate_block_entry_publication_dump,
                       block_entry_publication_row,
                       "duplicate fallback block-entry publication row text")) {
    return EXIT_FAILURE;
  }

  const auto critical_edge_prepared = legalize_critical_edge_parallel_copy_module();
  const std::string critical_edge_dump = prepare::print(critical_edge_prepared);
  if (!expect_contains(critical_edge_dump,
                       "prepared.func @critical_edge_parallel_copy_prepare_contract",
                       "critical-edge printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "parallel_copy left -> join execution_site=critical_edge execution_block=<none> has_cycle=no resolution=acyclic moves=1 steps=1",
          "critical-edge parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "parallel_copy right -> join execution_site=predecessor_terminator execution_block=right has_cycle=no resolution=acyclic moves=1 steps=1",
          "linear-edge parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "authority execution_site=critical_edge execution_block=<none>",
          "critical-edge parallel-copy authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "authority execution_site=predecessor_terminator execution_block=right",
          "linear-edge parallel-copy authority")) {
    return EXIT_FAILURE;
  }

  const auto call_wrapper_prepared = prepare_call_wrapper_dump_module();
  const std::string call_wrapper_dump = prepare::print(call_wrapper_prepared);
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=0 wrapper=same_module callee=same_module_i32 variadic_fpr_args=0",
                       "same-module wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=1 wrapper=direct_extern_fixed_arity callee=extern_fixed_i32 variadic_fpr_args=0",
                       "fixed extern wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=2 wrapper=direct_extern_variadic callee=extern_variadic_i32 variadic_fpr_args=1",
                       "variadic extern wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=3 wrapper=indirect variadic_fpr_args=0 args=1 indirect_callee=callee.ptr indirect_home=register indirect_bank=gpr indirect_value_id=",
                       "indirect wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "call block_index=0 inst_index=2 wrapper_kind=direct_extern_variadic variadic_fpr_arg_register_count=1 indirect=no callee=extern_variadic_i32",
                       "variadic wrapper call-plan detail")) {
    return EXIT_FAILURE;
  }
  const auto entry_function_id =
      call_wrapper_prepared.names.function_names.find("variadic_entry_dump_contract");
  const auto extern_function_id =
      call_wrapper_prepared.names.function_names.find("extern_variadic_i32");
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(call_wrapper_prepared, entry_function_id);
  const auto* extern_plan =
      prepare::find_prepared_variadic_entry_plan(call_wrapper_prepared, extern_function_id);
  if (entry_plan == nullptr || entry_plan->named_parameter_count != 2 ||
      entry_plan->named_register_counts.gp != std::optional<std::size_t>{1} ||
      entry_plan->named_register_counts.fp != std::optional<std::size_t>{1} ||
      entry_plan->register_save_area.required ||
      entry_plan->overflow_area.required ||
      entry_plan->va_list_layout.required ||
      entry_plan->missing_required_facts.size() != 2 ||
      extern_plan != nullptr) {
    std::cerr << "[FAIL] variadic entry prepared carrier did not stay distinct from call wrappers or extern declarations\n";
    return EXIT_FAILURE;
  }
  if (std::find(entry_plan->missing_required_facts.begin(),
                entry_plan->missing_required_facts.end(),
                "target_abi.variadic_entry_state") ==
          entry_plan->missing_required_facts.end() ||
      std::find(entry_plan->missing_required_facts.begin(),
                entry_plan->missing_required_facts.end(),
                "target_abi.va_list_layout") ==
          entry_plan->missing_required_facts.end()) {
    std::cerr << "[FAIL] non-AAPCS64 variadic entry carrier did not publish missing target ABI contract facts\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "prepared.summary @variadic_entry_dump_contract stable_base=rsp frame_size=0 frame_alignment=1 has_dynamic_stack=no saved_regs=0 calls=0 dynamic_stack_ops=0 variadic_entry=yes",
                       "variadic entry summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "prepared.func @variadic_entry_dump_contract named_params=2 named_gp=1 named_fp=1 helpers=0",
                       "variadic entry plan header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "register_save_area required=no size=<unknown> align=<unknown> slot=<none>",
                       "variadic entry register-save-area placeholder")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "missing fact=target_abi.variadic_entry_state",
                       "non-AAPCS64 variadic entry missing state contract")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "missing fact=target_abi.va_list_layout",
                       "non-AAPCS64 variadic entry missing va_list contract")) {
    return EXIT_FAILURE;
  }
  const auto rv64_call_wrapper_prepared =
      prepare::prepare_semantic_bir_module_with_options(
          call_wrapper_prepared.module,
          riscv_target_profile(),
          prepare::PrepareOptions{
              .run_legalize = true,
              .run_stack_layout = true,
              .run_liveness = true,
              .run_regalloc = true,
          });
  const std::string rv64_call_wrapper_dump = prepare::print(rv64_call_wrapper_prepared);
  const auto rv64_entry_function_id =
      rv64_call_wrapper_prepared.names.function_names.find("variadic_entry_dump_contract");
  const auto* rv64_entry_plan =
      prepare::find_prepared_variadic_entry_plan(rv64_call_wrapper_prepared,
                                                 rv64_entry_function_id);
  if (rv64_entry_plan == nullptr ||
      rv64_entry_plan->helper_resources.required_helpers.empty() == false ||
      rv64_entry_plan->register_save_area.required ||
      !rv64_entry_plan->overflow_area.required ||
      rv64_entry_plan->overflow_area.align_bytes != std::optional<std::size_t>{8} ||
      !rv64_entry_plan->va_list_layout.required ||
      rv64_entry_plan->va_list_layout.size_bytes != std::optional<std::size_t>{8} ||
      rv64_entry_plan->va_list_layout.align_bytes != std::optional<std::size_t>{8} ||
      rv64_entry_plan->va_list_layout.fields.size() != 1 ||
      !rv64_entry_plan->missing_required_facts.empty()) {
    std::cerr << "[FAIL] RV64 variadic entry carrier did not publish target ABI entry/layout facts for no-helper entry\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_call_wrapper_dump,
                       "va_list_layout required=yes size=8 align=8 fields=1",
                       "RV64 variadic va_list layout header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_call_wrapper_dump,
                       "field kind=overflow_arg_area offset=0 size=8",
                       "RV64 variadic va_list overflow field")) {
    return EXIT_FAILURE;
  }
  const auto aapcs64_variadic_prepared = prepare_aapcs64_variadic_entry_dump_module();
  const std::string aapcs64_variadic_dump = prepare::print(aapcs64_variadic_prepared);
  const auto aapcs64_function_id =
      aapcs64_variadic_prepared.names.function_names.find("aapcs64_variadic_entry_dump_contract");
  const auto* aapcs64_entry_plan =
      prepare::find_prepared_variadic_entry_plan(aapcs64_variadic_prepared, aapcs64_function_id);
  if (aapcs64_entry_plan == nullptr ||
      aapcs64_entry_plan->named_parameter_count != 2 ||
      aapcs64_entry_plan->named_register_counts.gp != std::optional<std::size_t>{1} ||
      aapcs64_entry_plan->named_register_counts.fp != std::optional<std::size_t>{1} ||
      !aapcs64_entry_plan->register_save_area.required ||
      aapcs64_entry_plan->register_save_area.size_bytes != std::optional<std::size_t>{192} ||
      aapcs64_entry_plan->register_save_area.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{0} ||
      aapcs64_entry_plan->register_save_area.stack_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aapcs64_entry_plan->register_save_area.initial_gp_offset_bytes !=
          std::optional<std::ptrdiff_t>{-56} ||
      aapcs64_entry_plan->register_save_area.initial_fp_offset_bytes !=
          std::optional<std::ptrdiff_t>{-112} ||
      !aapcs64_entry_plan->overflow_area.required ||
      aapcs64_entry_plan->overflow_area.base_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{1} ||
      aapcs64_entry_plan->overflow_area.base_stack_offset_bytes !=
          std::optional<std::size_t>{192} ||
      !aapcs64_entry_plan->va_list_layout.required ||
      aapcs64_entry_plan->va_list_layout.fields.size() != 5 ||
      aapcs64_entry_plan->helper_resources.required_helpers.size() != 1 ||
      aapcs64_entry_plan->helper_resources.required_helpers.front() !=
          prepare::PreparedVariadicEntryHelperKind::VaStart ||
      aapcs64_entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{1} ||
      aapcs64_entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      aapcs64_entry_plan->helper_operand_homes.size() != 1 ||
      !aapcs64_entry_plan->helper_operand_homes.front()
           .destination_va_list.has_value() ||
      !aapcs64_entry_plan->missing_required_facts.empty()) {
    std::cerr << "[FAIL] AAPCS64 variadic entry carrier did not publish structured ABI facts "
                 "and prepared storage/scratch authority\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_variadic_dump,
          "register_save_area required=yes size=192 align=16 slot=#0 stack_offset=0 gp_offset=0 fp_offset=64 gp_slot=8 fp_slot=16 saved_gp=7 saved_fp=7 initial_gp_offset=-56 initial_fp_offset=-112",
          "AAPCS64 variadic entry register-save facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "overflow_area required=yes base_slot=#1 base_stack_offset=192 align=8",
                       "AAPCS64 variadic overflow-area storage facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "va_list_layout required=yes size=32 align=8 fields=5",
                       "AAPCS64 variadic va_list layout header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "field kind=gp_register_save_area offset=8 size=8",
                       "AAPCS64 variadic GP save-area field")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "field kind=fp_register_save_area offset=16 size=8",
                       "AAPCS64 variadic FP save-area field")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "helper_resources scratch_registers=1 scratch_stack=0 helpers=[va_start]",
                       "AAPCS64 variadic va_start helper scratch facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "helper kind=va_start",
                       "AAPCS64 variadic va_start helper need")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "helper_operand kind=va_start block=0 inst=0 dst_va_list=ap:",
                       "AAPCS64 variadic va_start helper operand home")) {
    return EXIT_FAILURE;
  }
  const auto aapcs64_helper_family_prepared =
      prepare_aapcs64_variadic_entry_helper_family_dump_module();
  const std::string aapcs64_helper_family_dump = prepare::print(aapcs64_helper_family_prepared);
  const auto aapcs64_helper_family_function_id =
      aapcs64_helper_family_prepared.names.function_names.find(
          "aapcs64_variadic_entry_helper_family_dump_contract");
  const auto* aapcs64_helper_family_entry_plan =
      prepare::find_prepared_variadic_entry_plan(aapcs64_helper_family_prepared,
                                                 aapcs64_helper_family_function_id);
  if (aapcs64_helper_family_entry_plan == nullptr ||
      aapcs64_helper_family_entry_plan->named_parameter_count != 3 ||
      aapcs64_helper_family_entry_plan->named_register_counts.gp !=
          std::optional<std::size_t>{1} ||
      aapcs64_helper_family_entry_plan->named_register_counts.fp !=
          std::optional<std::size_t>{1} ||
      aapcs64_helper_family_entry_plan->helper_resources.required_helpers.size() != 4 ||
      aapcs64_helper_family_entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{2} ||
      aapcs64_helper_family_entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      aapcs64_helper_family_entry_plan->helper_operand_homes.size() != 7) {
    std::cerr << "[FAIL] AAPCS64 variadic helper-family carrier lost named counts, helpers, or scratch facts\n";
    return EXIT_FAILURE;
  }
  const auto& helper_family = aapcs64_helper_family_entry_plan->helper_resources.required_helpers;
  for (const auto helper : {
           prepare::PreparedVariadicEntryHelperKind::VaStart,
           prepare::PreparedVariadicEntryHelperKind::VaArg,
           prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
           prepare::PreparedVariadicEntryHelperKind::VaCopy,
       }) {
    if (std::find(helper_family.begin(), helper_family.end(), helper) == helper_family.end()) {
      std::cerr << "[FAIL] AAPCS64 variadic helper-family carrier missed a helper kind\n";
      return EXIT_FAILURE;
    }
  }
  const auto* scalar_i32_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 1);
  const auto* scalar_f64_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 2);
  const auto* aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 3);
  const auto* hfa_aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 4);
  const auto* missing_aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 6);
  if (scalar_i32_homes == nullptr || scalar_f64_homes == nullptr ||
      !scalar_i32_homes->scalar_access_plan.has_value() ||
      scalar_i32_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea ||
      scalar_i32_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea} ||
      scalar_i32_homes->scalar_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{8} ||
      scalar_i32_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::GpOffset} ||
      scalar_i32_homes->scalar_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{24} ||
      scalar_i32_homes->scalar_access_plan->overflow_source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      scalar_i32_homes->scalar_access_plan->overflow_source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      !scalar_f64_homes->scalar_access_plan.has_value() ||
      scalar_f64_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea ||
      scalar_f64_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea} ||
      scalar_f64_homes->scalar_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{16} ||
      scalar_f64_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpOffset} ||
      scalar_f64_homes->scalar_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{28} ||
      scalar_f64_homes->scalar_access_plan->overflow_source_field_offset_bytes !=
          std::optional<std::size_t>{0}) {
    std::cerr << "[FAIL] AAPCS64 variadic helper-family carrier missed scalar va_arg access-plan facts\n";
    return EXIT_FAILURE;
  }
  prepare::PreparedValueHome aggregate_payload_home{
      .value_name = c4c::kInvalidValueName,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = std::size_t{42},
      .offset_bytes = std::size_t{16},
  };
  prepare::PreparedValueHome aggregate_source_va_list_home{
      .value_name = c4c::kInvalidValueName,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string("x0"),
  };
  prepare::PreparedVariadicAggregateVaArgAccessPlan incomplete_aggregate_plan;
  if (prepare::is_complete_prepared_variadic_aggregate_va_arg_access_plan(
          incomplete_aggregate_plan)) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier treated an empty access plan as complete\n";
    return EXIT_FAILURE;
  }
  prepare::PreparedVariadicAggregateVaArgAccessPlan complete_aggregate_plan{
      .source_class = prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
      .payload_size_bytes = 8,
      .payload_align_bytes = 4,
      .destination_payload_home = aggregate_payload_home,
      .source_field = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
      .source_field_offset_bytes = std::size_t{0},
      .source_payload_offset_bytes = std::size_t{0},
      .source_slot_size_bytes = std::size_t{8},
      .copy_size_bytes = std::size_t{8},
      .copy_align_bytes = std::size_t{4},
      .progression_field = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
      .progression_field_offset_bytes = std::size_t{0},
      .progression_stride_bytes = std::size_t{8},
  };
  prepare::PreparedVariadicEntryHelperOperandHomes complete_aggregate_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
      .source_va_list = aggregate_source_va_list_home,
      .aggregate_destination_payload = aggregate_payload_home,
      .aggregate_access_plan = complete_aggregate_plan,
  };
  if (!prepare::is_complete_prepared_variadic_aggregate_va_arg_access_plan(
          complete_aggregate_plan) ||
      !prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(
          complete_aggregate_homes)) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier cannot represent a complete access plan\n";
    return EXIT_FAILURE;
  }
  if (aggregate_homes == nullptr ||
      !aggregate_homes->aggregate_access_plan.has_value() ||
      aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      aggregate_homes->aggregate_access_plan->payload_size_bytes != 8 ||
      aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
      !aggregate_homes->aggregate_access_plan->destination_payload_home.has_value() ||
      aggregate_homes->aggregate_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      aggregate_homes->aggregate_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aggregate_homes->aggregate_access_plan->source_payload_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aggregate_homes->aggregate_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{8} ||
      aggregate_homes->aggregate_access_plan->copy_size_bytes !=
          std::optional<std::size_t>{8} ||
      aggregate_homes->aggregate_access_plan->copy_align_bytes !=
          std::optional<std::size_t>{4} ||
      aggregate_homes->aggregate_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      aggregate_homes->aggregate_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aggregate_homes->aggregate_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{8}) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier missed prepared access-plan facts\n";
    return EXIT_FAILURE;
  }
  if (hfa_aggregate_homes == nullptr ||
      !hfa_aggregate_homes->aggregate_access_plan.has_value() ||
      hfa_aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::RegisterSaveArea ||
      hfa_aggregate_homes->aggregate_access_plan->payload_size_bytes != 4 ||
      hfa_aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
      hfa_aggregate_homes->aggregate_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea} ||
      hfa_aggregate_homes->aggregate_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{16} ||
      hfa_aggregate_homes->aggregate_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpOffset} ||
      hfa_aggregate_homes->aggregate_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{16} ||
      hfa_aggregate_homes->aggregate_access_plan->register_save_lane_count !=
          std::optional<std::size_t>{1} ||
      hfa_aggregate_homes->aggregate_access_plan->register_save_lane_size_bytes !=
          std::optional<std::size_t>{4}) {
    std::cerr << "[FAIL] AAPCS64 variadic HFA va_arg carrier missed explicit lane-shape facts\n";
    return EXIT_FAILURE;
  }
  if (missing_aggregate_homes == nullptr ||
      missing_aggregate_homes->aggregate_access_plan.has_value() ||
      std::find(aapcs64_helper_family_entry_plan->missing_required_facts.begin(),
                aapcs64_helper_family_entry_plan->missing_required_facts.end(),
                "helper_operand_homes.va_arg_aggregate.aggregate_access_plan") ==
          aapcs64_helper_family_entry_plan->missing_required_facts.end()) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier did not expose the missing aggregate access-plan fact\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "prepared.func @aapcs64_variadic_entry_helper_family_dump_contract named_params=3 named_gp=1 named_fp=1 helpers=4",
          "AAPCS64 variadic helper-family plan header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_resources scratch_registers=2 scratch_stack=0 helpers=[va_start,va_arg,va_arg_aggregate,va_copy]",
                       "AAPCS64 variadic helper-family summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper kind=va_copy",
                       "AAPCS64 variadic va_copy helper need")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg block=0 inst=1",
                       "AAPCS64 variadic scalar va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "scalar_access_plan=source_class=gp_register_save_area:type=i32:size=4:align=4",
          "AAPCS64 variadic scalar i32 va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "source_field=gp_register_save_area@8:source_slot=8:progression_field=gp_offset@24:progression_stride=8:overflow_source_field=overflow_arg_area@0:overflow_stride=4",
          "AAPCS64 variadic scalar i32 va_arg source coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "scalar_access_plan=source_class=fp_register_save_area:type=f64:size=8:align=8",
          "AAPCS64 variadic scalar f64 va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "source_field=fp_register_save_area@16:source_slot=16:progression_field=fp_offset@28:progression_stride=16:overflow_source_field=overflow_arg_area@0:overflow_stride=8",
          "AAPCS64 variadic scalar f64 va_arg source coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=3",
                       "AAPCS64 variadic aggregate va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "aggregate_access_plan=source_class=overflow_arg_area:payload_size=8:payload_align=4",
          "AAPCS64 variadic aggregate va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "source_field=overflow_arg_area@0:source_payload_offset=0:source_slot=8:copy_size=8:copy_align=4:progression_field=overflow_arg_area@0:progression_stride=8",
          "AAPCS64 variadic aggregate va_arg source and progression coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=4",
                       "AAPCS64 variadic HFA aggregate va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "aggregate_access_plan=source_class=register_save_area:payload_size=4:payload_align=4",
          "AAPCS64 variadic HFA aggregate va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "register_save_lanes=1:register_save_lane_size=4",
          "AAPCS64 variadic HFA aggregate va_arg lane-shape facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=6",
                       "AAPCS64 variadic aggregate va_arg missing operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "aggregate_access_plan=<none>",
                       "AAPCS64 variadic aggregate va_arg explicit missing access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "missing fact=helper_operand_homes.va_arg_aggregate.aggregate_access_plan",
          "AAPCS64 variadic aggregate va_arg missing access-plan fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_copy block=0 inst=7",
                       "AAPCS64 variadic va_copy operand homes")) {
    return EXIT_FAILURE;
  }
  auto rv64_helper_family_prepared =
      prepare_aapcs64_variadic_entry_helper_family_dump_module();
  rv64_helper_family_prepared =
      prepare::prepare_semantic_bir_module_with_options(
          rv64_helper_family_prepared.module,
          riscv_target_profile(),
          prepare::PrepareOptions{
              .run_legalize = true,
              .run_stack_layout = true,
              .run_liveness = true,
              .run_regalloc = true,
          });
  const std::string rv64_helper_family_dump =
      prepare::print(rv64_helper_family_prepared);
  const auto rv64_helper_family_function_id =
      rv64_helper_family_prepared.names.function_names.find(
          "aapcs64_variadic_entry_helper_family_dump_contract");
  const auto* rv64_helper_family_entry_plan =
      prepare::find_prepared_variadic_entry_plan(
          rv64_helper_family_prepared,
          rv64_helper_family_function_id);
  const auto has_rv64_missing_fact = [&](std::string_view fact) {
    return rv64_helper_family_entry_plan != nullptr &&
           std::find(rv64_helper_family_entry_plan->missing_required_facts.begin(),
                     rv64_helper_family_entry_plan->missing_required_facts.end(),
                     fact) !=
               rv64_helper_family_entry_plan->missing_required_facts.end();
  };
  if (rv64_helper_family_entry_plan == nullptr ||
      rv64_helper_family_entry_plan->helper_resources.required_helpers.size() != 4 ||
      rv64_helper_family_entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{3} ||
      rv64_helper_family_entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      rv64_helper_family_entry_plan->helper_operand_homes.size() != 5 ||
      rv64_helper_family_entry_plan->register_save_area.required ||
      !rv64_helper_family_entry_plan->overflow_area.required ||
      rv64_helper_family_entry_plan->overflow_area.align_bytes !=
          std::optional<std::size_t>{8} ||
      !rv64_helper_family_entry_plan->va_list_layout.required ||
      rv64_helper_family_entry_plan->va_list_layout.size_bytes !=
          std::optional<std::size_t>{8} ||
      rv64_helper_family_entry_plan->va_list_layout.fields.size() != 1 ||
      has_rv64_missing_fact("target_abi.variadic_entry_state") ||
      has_rv64_missing_fact("target_abi.va_list_layout") ||
      has_rv64_missing_fact("helper_resources.scratch_register_count") ||
      has_rv64_missing_fact("helper_resources.scratch_stack_bytes") ||
      has_rv64_missing_fact("helper_operand_homes.va_start.destination_va_list") ||
      has_rv64_missing_fact("helper_operand_homes.va_start.destination_va_list_address") ||
      has_rv64_missing_fact("helper_operand_homes.va_arg.source_va_list") ||
      has_rv64_missing_fact("helper_operand_homes.va_arg.scalar_result") ||
      has_rv64_missing_fact("helper_operand_homes.va_arg.scalar_access_plan") ||
      !has_rv64_missing_fact("target_abi.va_arg.scalar_payload_abi") ||
      has_rv64_missing_fact("helper_operand_homes.va_arg_aggregate.source_va_list") ||
      has_rv64_missing_fact("helper_operand_homes.va_arg_aggregate.aggregate_destination_payload") ||
      has_rv64_missing_fact("helper_operand_homes.va_arg_aggregate.aggregate_access_plan") ||
      !has_rv64_missing_fact("target_abi.va_arg_aggregate.payload_abi") ||
      !has_rv64_missing_fact("helper_operand_homes.va_copy.destination_va_list") ||
      !has_rv64_missing_fact("helper_operand_homes.va_copy.source_va_list")) {
    std::cerr << "[FAIL] RV64 variadic helper-family carrier did not publish target ABI facts while preserving explicit missing helper facts\n";
    return EXIT_FAILURE;
  }
  const auto* rv64_va_start_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *rv64_helper_family_entry_plan, 0, 0);
  if (rv64_va_start_homes == nullptr ||
      !prepare::has_complete_prepared_variadic_va_start_operand_homes(
          *rv64_va_start_homes)) {
    std::cerr << "[FAIL] RV64 variadic helper-family carrier did not materialize va_start operand homes\n";
    return EXIT_FAILURE;
  }
  const auto* rv64_aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *rv64_helper_family_entry_plan, 0, 3);
  const auto* rv64_scalar_i32_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *rv64_helper_family_entry_plan, 0, 1);
  const auto* rv64_scalar_f64_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *rv64_helper_family_entry_plan, 0, 2);
  const auto* rv64_hfa_shaped_aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *rv64_helper_family_entry_plan, 0, 4);
  if (rv64_scalar_i32_homes == nullptr ||
      !rv64_scalar_i32_homes->scalar_access_plan.has_value() ||
      rv64_scalar_i32_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::OverflowArgArea ||
      rv64_scalar_i32_homes->scalar_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      rv64_scalar_i32_homes->scalar_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{4} ||
      rv64_scalar_i32_homes->scalar_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{4} ||
      rv64_scalar_f64_homes == nullptr ||
      rv64_scalar_f64_homes->scalar_access_plan.has_value()) {
    std::cerr << "[FAIL] RV64 variadic helper-family carrier did not consume or precisely diagnose scalar va_arg plans\n";
    return EXIT_FAILURE;
  }
  if (rv64_aggregate_homes == nullptr ||
      !rv64_aggregate_homes->aggregate_access_plan.has_value() ||
      rv64_aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      rv64_aggregate_homes->aggregate_access_plan->payload_size_bytes != 8 ||
      rv64_aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
      rv64_aggregate_homes->aggregate_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      rv64_aggregate_homes->aggregate_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{8} ||
      rv64_aggregate_homes->aggregate_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{8} ||
      rv64_hfa_shaped_aggregate_homes == nullptr ||
      !rv64_hfa_shaped_aggregate_homes->aggregate_access_plan.has_value() ||
      rv64_hfa_shaped_aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      rv64_hfa_shaped_aggregate_homes->aggregate_access_plan->payload_size_bytes != 4 ||
      rv64_hfa_shaped_aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
      rv64_hfa_shaped_aggregate_homes->aggregate_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{4} ||
      rv64_hfa_shaped_aggregate_homes->aggregate_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{4} ||
      rv64_hfa_shaped_aggregate_homes->aggregate_access_plan->register_save_lane_count.has_value()) {
    std::cerr << "[FAIL] RV64 variadic helper-family carrier did not materialize aggregate overflow va_arg plans\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "va_list_layout required=yes size=8 align=8 fields=1",
                       "RV64 variadic helper-family va_list layout")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "helper_resources scratch_registers=3 scratch_stack=0 helpers=[va_start,va_arg,va_arg_aggregate,va_copy]",
                       "RV64 variadic helper-family helper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "helper_operand kind=va_start block=0 inst=0 dst_va_list=ap:",
                       "RV64 variadic va_start operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "helper_operand kind=va_arg block=0 inst=1",
                       "RV64 variadic scalar i32 va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          rv64_helper_family_dump,
          "scalar_access_plan=source_class=overflow_arg_area:type=i32:size=4:align=4",
          "RV64 variadic scalar i32 va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          rv64_helper_family_dump,
          "source_field=overflow_arg_area@0:source_slot=4:progression_field=overflow_arg_area@0:progression_stride=4:overflow_source_field=overflow_arg_area@0:overflow_stride=4",
          "RV64 variadic scalar i32 va_arg overflow coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "helper_operand kind=va_arg block=0 inst=2",
                       "RV64 variadic unsupported scalar f64 va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "scalar_access_plan=<none>",
                       "RV64 variadic unsupported scalar f64 va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=3",
                       "RV64 variadic aggregate va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          rv64_helper_family_dump,
          "aggregate_access_plan=source_class=overflow_arg_area:payload_size=8:payload_align=4",
          "RV64 variadic aggregate va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          rv64_helper_family_dump,
          "source_field=overflow_arg_area@0:source_payload_offset=0:source_slot=8:copy_size=8:copy_align=4:progression_field=overflow_arg_area@0:progression_stride=8",
          "RV64 variadic aggregate va_arg source and progression coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=4",
                       "RV64 variadic HFA-shaped aggregate va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "missing fact=target_abi.va_arg.scalar_payload_abi",
                       "RV64 scalar va_arg payload ABI diagnostic fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          rv64_helper_family_dump,
          "missing fact=target_abi.va_arg_aggregate.payload_abi",
          "RV64 aggregate va_arg missing payload ABI fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(rv64_helper_family_dump,
                       "missing fact=helper_operand_homes.va_copy.source_va_list",
                       "RV64 va_copy missing operand fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "call block_index=0 inst_index=3 wrapper_kind=indirect variadic_fpr_arg_register_count=0 indirect=yes indirect_callee=callee.ptr indirect_encoding=register indirect_bank=gpr indirect_value_id=",
                       "indirect wrapper call-plan detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "arg index=0 value_bank=gpr source_encoding=immediate source_literal=5",
                       "indirect wrapper immediate argument detail")) {
    return EXIT_FAILURE;
  }

  const auto memory_return_prepared = prepare_memory_return_call_dump_module();
  const std::string memory_return_dump = prepare::print(memory_return_prepared);
  if (!expect_contains(memory_return_dump,
                       "callsite block=0 inst=0 wrapper=direct_extern_fixed_arity callee=extern_make_pair variadic_fpr_args=0 args=2 memory_return=lv.call.sret.storage memory_home=frame_slot sret_arg=0",
                       "memory-return summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(memory_return_dump,
                       "call block_index=0 inst_index=0 wrapper_kind=direct_extern_fixed_arity variadic_fpr_arg_register_count=0 indirect=no callee=extern_make_pair memory_return=lv.call.sret.storage memory_encoding=frame_slot sret_arg_index=0",
                       "memory-return call-plan detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(memory_return_dump,
                       "arg index=1 value_bank=gpr source_encoding=immediate source_literal=13",
                       "memory-return immediate argument detail")) {
    return EXIT_FAILURE;
  }

  const auto i128_helper_prepared = prepare_i128_runtime_helper_mapping_dump_module();
  const std::string i128_helper_dump = prepare::print(i128_helper_prepared);
  if (!expect_contains(i128_helper_dump,
                       "--- prepared-i128-runtime-helpers ---",
                       "i128 runtime helper section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=0 family=div_rem kind=signed_div opcode=sdiv callee=__divti3 source_type=i128 result_type=i128 result=wide.div#",
                       "i128 signed div helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result_ownership=direct_low_high_lanes memory_return=<none>",
                       "i128 helper direct-result ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "resources=[call_boundary,runtime_helper_callee,caller_saved_clobbers,"
                       "source_operation_identity]",
                       "i128 helper resource boundary policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "abi_transition=direct_register_pair_arguments_and_result arg_bank=gpr "
                       "result_bank=gpr arg_count=2 lanes_per_arg=2 result_lanes=2 lane_width=8",
                       "i128 helper ABI register-bank transition policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "clobbers=gpr:",
                       "i128 helper call-clobber policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=1 family=div_rem kind=unsigned_rem opcode=urem callee=__umodti3 source_type=i128 result_type=i128 result=wide.rem#",
                       "i128 unsigned rem helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=2 family=float_integer_conversion kind=float_to_signed_int opcode=fptosi callee=__fixdfti source_type=f64 result_type=i128 result=from.fp#",
                       "i128 float-to-signed conversion helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "operand=fp#",
                       "i128 conversion helper operand identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "source_width=8 result_width=16 source_signed=no result_signed=yes",
                       "i128 conversion helper signedness and width facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar_ownership operand=fp#",
                       "i128 conversion helper scalar FP source ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "[type=f64,width=8,bank=fpr,home=",
                       "i128 conversion helper scalar FP home facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=3 family=float_integer_conversion kind=unsigned_int_to_float opcode=uitofp callee=__floatuntidf source_type=i128 result_type=f64 result=to.fp#",
                       "i128 unsigned-to-float conversion helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "source_width=16 result_width=8 source_signed=no result_signed=no",
                       "i128 conversion helper unsigned source facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result_ownership=scalar_value memory_return=<none>",
                       "i128 conversion helper scalar result ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar_ownership operand=<none> result=to.fp#",
                       "i128 conversion helper scalar result ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=2 family=float_integer_conversion "
                       "kind=float_to_signed_int opcode=fptosi callee=__fixdfti",
                       "i128 conversion helper supported boundary detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "resources=[call_boundary,runtime_helper_callee,caller_saved_clobbers,"
                       "source_operation_identity]",
                       "i128 conversion helper resource boundary policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar.operand=scalar_value_to_abi_argument",
                       "i128 conversion helper scalar argument marshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar.result=abi_result_to_scalar_value",
                       "i128 conversion helper scalar result unmarshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "selected_call_ownership=[owns_terminal_call=yes,callee=yes,"
                       "resources=yes,clobbers=yes,abi_bindings=yes,marshaling=yes,"
                       "live_preservation=yes]",
                       "i128 conversion helper selected-call ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "lhs.low=lhs#",
                       "i128 helper lhs low lane identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result.high=wide.rem#",
                       "i128 helper result high lane identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "abi_bindings lhs.low=lhs#",
                       "i128 helper ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "arg=0,abi_index=0,bank=gpr,class=general,reg=rdi",
                       "i128 helper lhs low ABI argument register binding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "arg=1,abi_index=3,bank=gpr,class=general,reg=rcx",
                       "i128 helper rhs high ABI argument register binding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result,abi_index=1,bank=gpr,class=general,reg=rdx",
                       "i128 helper high result ABI register binding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "marshaling lhs.low=carrier_lane_to_abi_argument",
                       "i128 helper marshaling section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "phase=before_call,op=move,value=lhs#",
                       "i128 helper source lane marshaling phase")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "arg=0,abi_index=0,abi_reg=rdi",
                       "i128 helper source lane ABI argument move target")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "carrier_slot=#",
                       "i128 helper source lane memory-backed marshal source")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result.high=abi_result_to_carrier_lane",
                       "i128 helper result unmarshal section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "phase=after_call,op=move,value=wide.div#",
                       "i128 helper result lane unmarshal phase")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result,abi_index=1,abi_reg=rdx",
                       "i128 helper result lane ABI source register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "live_preservation=[evaluated=yes,caller_saved_clobbers=yes,"
                       "additional=none,preserved=0]",
                       "i128 helper live-preservation policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "selected_call_ownership=[owns_terminal_call=yes,callee=yes,"
                       "resources=yes,clobbers=yes,abi_bindings=yes,marshaling=yes,"
                       "live_preservation=yes]",
                       "i128 helper selected-call ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "carrier=memory_backed,slot=",
                       "i128 helper structured memory-backed lane authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result_requires_register_pair_carrier",
                       "i128 helper non-register lane fail-closed diagnostic")) {
    return EXIT_FAILURE;
  }

  const auto source_shape_prepared = prepare_call_argument_source_shape_dump_module();
  const std::string source_shape_dump = prepare::print(source_shape_prepared);
  const auto* source_shape_storage =
      find_storage_plan_function(source_shape_prepared, "call_argument_source_shape_dump_contract");
  const auto* loaded_ptr =
      source_shape_storage == nullptr
          ? nullptr
          : find_storage_value(source_shape_prepared, *source_shape_storage, "loaded.ptr");
  if (loaded_ptr == nullptr) {
    std::cerr << "missing loaded.ptr storage publication\n";
    return EXIT_FAILURE;
  }
  const std::string computed_summary =
      "arg1 bank=gpr from=computed_address:loaded.ptr#" + std::to_string(loaded_ptr->value_id) + "+4";
  const std::string computed_detail_id =
      "source_base_value_id=" + std::to_string(loaded_ptr->value_id);
  const std::string computed_detail_shape = "source_base=loaded.ptr source_delta=4";
  if (!expect_contains(source_shape_dump,
                       "arg0 bank=gpr from=symbol_address:@extern_data",
                       "symbol-address argument summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       computed_summary,
                       "computed-address argument summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "arg index=0 value_bank=gpr source_encoding=symbol_address",
                       "symbol-address argument detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "source_symbol=@extern_data",
                       "symbol-address argument detail payload")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "source_symbol_id=",
                       "symbol-address argument detail LinkNameId payload")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "arg index=1 value_bank=gpr source_encoding=computed_address",
                       "computed-address argument detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       computed_detail_id,
                       "computed-address argument detail id")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       computed_detail_shape,
                       "computed-address argument detail payload")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "--- prepared-store-source-publications ---",
                       "prepared store-source publication section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "store_source function=call_argument_source_shape_dump_contract "
                       "block=entry inst=4 source=derived.seed status=available "
                       "intent=store_local_publication source_producer=binary "
                       "source_producer_block=entry source_producer_inst=3 "
                       "source_load_local=no source_load_global=no source_cast=no "
                       "source_binary=yes source_select=no direct_global_select_chain=no "
                       "direct_global_root_is_select=no",
                       "store-source row with concrete binary source producer fields")) {
    return EXIT_FAILURE;
  }

  const auto select_chain_prepared = prepare_select_chain_direct_global_dump_module();
  const std::string select_chain_dump = prepare::print(select_chain_prepared);
  if (!expect_contains(select_chain_dump,
                       "direct_global_select_chain=yes direct_global_source=selected.arg "
                       "direct_global_root_is_select=yes direct_global_root_inst=1",
                       "call-argument direct-global select-chain dependency labels")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(select_chain_dump,
                       "--- prepared-select-chain-materializations ---",
                       "prepared select-chain materialization section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(select_chain_dump,
                       "select_chain function=select_chain_direct_global_dump_contract "
                       "block=entry value=selected.arg root_is_select=yes root_inst=1 "
                       "direct_global_select_chain=yes direct_global_root_is_select=yes "
                       "direct_global_root_inst=1 source_producer=select_materialization "
                       "source_producer_block=entry source_producer_inst=1",
                       "scalar select-chain direct-global row with source producer labels")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(select_chain_dump,
                       "select_chain function=select_chain_direct_global_dump_contract "
                       "block=entry value=loaded.global root_is_select=no root_inst=0 "
                       "direct_global_select_chain=yes direct_global_root_is_select=no "
                       "direct_global_root_inst=0 source_producer=load_global "
                       "source_producer_block=entry source_producer_inst=0",
                       "scalar direct-global load row with source producer labels")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(select_chain_dump,
                       "store_source function=select_chain_direct_global_dump_contract "
                       "block=entry inst=2 source=selected.arg status=available "
                       "intent=store_local_publication source_producer=select_materialization "
                       "source_producer_block=entry source_producer_inst=1 "
                       "source_load_local=no source_load_global=no source_cast=no "
                       "source_binary=no source_select=yes direct_global_select_chain=yes "
                       "direct_global_root_is_select=yes direct_global_root_inst=1",
                       "store-source row with concrete direct-global select-chain fields")) {
    return EXIT_FAILURE;
  }

  const auto cross_call_prepared = prepare_cross_call_preservation_dump_module();
  const auto* cross_call_storage =
      find_storage_plan_function(cross_call_prepared, "cross_call_preservation_dump_contract");
  const auto* cross_call_plans =
      find_call_plans_function(cross_call_prepared, "cross_call_preservation_dump_contract");
  const auto* cross_call_carry =
      cross_call_storage == nullptr ? nullptr : find_storage_value(cross_call_prepared, *cross_call_storage, "carry");
  const auto cross_call_function_id =
      cross_call_prepared.names.function_names.find("cross_call_preservation_dump_contract");
  const auto* cross_call_frame_plan =
      cross_call_function_id == c4c::kInvalidFunctionName
          ? nullptr
          : prepare::find_prepared_frame_plan(cross_call_prepared, cross_call_function_id);
  if (cross_call_storage == nullptr || cross_call_plans == nullptr ||
      cross_call_plans->calls.size() < 2 || cross_call_carry == nullptr ||
      cross_call_frame_plan == nullptr) {
    std::cerr << "[FAIL] missing prepared carry storage fixture for cross-call preservation dump\n";
    return EXIT_FAILURE;
  }
  const auto cross_call_saved_it = std::find_if(cross_call_frame_plan->saved_callee_registers.begin(),
                                                cross_call_frame_plan->saved_callee_registers.end(),
                                                [](const auto& saved) {
                                                  return saved.register_name == "s1";
                                                });
  if (cross_call_saved_it == cross_call_frame_plan->saved_callee_registers.end()) {
    std::cerr << "[FAIL] missing published save authority for s1 in cross-call dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!cross_call_saved_it->placement.has_value() ||
      !cross_call_saved_it->slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *cross_call_saved_it->slot_placement)) {
    std::cerr << "[FAIL] missing structured saved-register placement for s1 in cross-call dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::size_t cross_call_save_index = cross_call_saved_it->save_index;
  const std::string cross_call_dump = prepare::print(cross_call_prepared);
  if (!expect_contains(cross_call_dump,
                       "saved gpr:s1 order=" + std::to_string(cross_call_save_index) +
                           " " + register_placement_text(cross_call_saved_it->placement) +
                           " width=1 units=s1 " +
                           saved_register_slot_placement_text(cross_call_saved_it->slot_placement),
                       "cross-call saved-register slot placement summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "saved_register bank=gpr " +
                           register_placement_text(cross_call_saved_it->placement) +
                           " reg=s1 save_index=" + std::to_string(cross_call_save_index) +
                           " width=1 units=s1 " +
                           saved_register_slot_placement_text(cross_call_saved_it->slot_placement),
                       "cross-call frame-plan saved-register slot placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "preserves=carry#" + std::to_string(cross_call_carry->value_id) +
                           ":callee_saved_register:s1[s1]:save" +
                           std::to_string(cross_call_save_index),
                       "cross-call preservation summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "preserve value=carry value_id=" + std::to_string(cross_call_carry->value_id) +
                           " route=callee_saved_register save_index=" +
                           std::to_string(cross_call_save_index) + " " +
                           register_placement_text(cross_call_saved_it->placement) +
                           " reg=s1 bank=gpr units=s1",
                       "cross-call preservation detail")) {
    return EXIT_FAILURE;
  }
  const auto& cross_call_prior_arg = cross_call_plans->calls[1].arguments.front();
  if (!cross_call_prior_arg.source_selection.has_value() ||
      cross_call_prior_arg.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation ||
      cross_call_prior_arg.source_selection->preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      cross_call_prior_arg.source_selection->source_value_id !=
          std::optional<prepare::PreparedValueId>{cross_call_carry->value_id} ||
      cross_call_prior_arg.source_selection->preserved_register_name !=
          std::optional<std::string>{"s1"}) {
    std::cerr << "[FAIL] missing callee-saved prior-preservation source selection fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "arg.source_selection=prior_preservation selection_source_value_id=" +
                           std::to_string(cross_call_carry->value_id) +
                           " selection_source_value=carry",
                       "callee-saved prior-preservation source selection identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "selection_preserved_call_block=0 selection_preserved_call_inst=2 "
                       "selection_preservation_route=callee_saved_register "
                       "selection_preserved_reg=s1 selection_preserved_bank=gpr width=1 units=s1 " +
                           register_placement_text(cross_call_saved_it->placement,
                                                   "selection_preserved_placement"),
                       "callee-saved prior-preservation source selection payload")) {
    return EXIT_FAILURE;
  }

  const auto stack_cross_call_prepared = prepare_stack_cross_call_preservation_dump_module();
  const auto* stack_cross_call_plans =
      find_call_plans_function(stack_cross_call_prepared, "stack_cross_call_preservation_dump_contract");
  if (stack_cross_call_plans == nullptr || stack_cross_call_plans->calls.size() != 1) {
    std::cerr << "[FAIL] missing prepared stack-preservation call fixture for dump\n";
    return EXIT_FAILURE;
  }
  const auto& stack_cross_call = stack_cross_call_plans->calls.front();
  const auto stack_preserved_it = std::find_if(
      stack_cross_call.preserved_values.begin(),
      stack_cross_call.preserved_values.end(),
      [](const auto& preserved) {
        return preserved.route == prepare::PreparedCallPreservationRoute::StackSlot;
      });
  if (stack_preserved_it == stack_cross_call.preserved_values.end() ||
      !stack_preserved_it->slot_id.has_value() ||
      !stack_preserved_it->stack_offset_bytes.has_value() ||
      !stack_preserved_it->stack_size_bytes.has_value() ||
      !stack_preserved_it->stack_align_bytes.has_value() ||
      !stack_preserved_it->spill_slot_placement.has_value() ||
      stack_preserved_it->preservation_destination.storage_kind !=
          prepare::PreparedMoveStorageKind::StackSlot ||
      stack_preserved_it->preservation_reason.empty()) {
    std::cerr << "[FAIL] missing stack-slot preservation authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string stack_cross_call_dump = prepare::print(stack_cross_call_prepared);
  const std::string stack_preserved_summary =
      std::string(prepare::prepared_value_name(stack_cross_call_prepared.names,
                                               stack_preserved_it->value_name)) +
      "#" + std::to_string(stack_preserved_it->value_id) + ":stack_slot:slot#" +
      std::to_string(*stack_preserved_it->slot_id) + ":stack+" +
      std::to_string(*stack_preserved_it->stack_offset_bytes) + ":size=" +
      std::to_string(*stack_preserved_it->stack_size_bytes) + ":align=" +
      std::to_string(*stack_preserved_it->stack_align_bytes);
  if (!expect_contains(stack_cross_call_dump,
                       stack_preserved_summary,
                       "stack-preserved call-slot extent summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_cross_call_dump,
                       "preserve value=" +
                           std::string(prepare::prepared_value_name(stack_cross_call_prepared.names,
                                                                    stack_preserved_it->value_name)) +
                           " value_id=" + std::to_string(stack_preserved_it->value_id) +
                           " route=stack_slot " +
                           spill_slot_placement_text(stack_preserved_it->spill_slot_placement) +
                           " bank=none slot=#" + std::to_string(*stack_preserved_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*stack_preserved_it->stack_offset_bytes) +
                           " stack_size=" +
                           std::to_string(*stack_preserved_it->stack_size_bytes) +
                           " stack_align=" +
                           std::to_string(*stack_preserved_it->stack_align_bytes) +
                           " preservation_source=",
                       "stack-preserved structured spill-slot extent detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_cross_call_dump,
                       "preservation_destination=stack_slot:slot#" +
                           std::to_string(*stack_preserved_it->slot_id) +
                           ":value#" + std::to_string(stack_preserved_it->value_id) +
                           " preservation_reason=" +
                           stack_preserved_it->preservation_reason,
                       "stack-preserved explicit destination and reason detail")) {
    return EXIT_FAILURE;
  }

  const auto manual_stack_prior_prepared =
      manual_stack_prior_preservation_source_selection_dump_module();
  const std::string manual_stack_prior_dump = prepare::print(manual_stack_prior_prepared);
  if (!expect_contains(manual_stack_prior_dump,
                       "call block_index=0 inst_index=8 "
                       "wrapper_kind=direct_extern_fixed_arity "
                       "variadic_fpr_arg_register_count=0 indirect=no "
                       "callee=manual_stack_sink outgoing_stack_argument_area=32",
                       "manual call-level outgoing stack argument area")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(manual_stack_prior_dump,
                       "dest_stack_offset=8 dest_stack_size=8",
                       "manual per-argument stack lane remains separate from call area")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(manual_stack_prior_dump,
                       "arg.source_selection=prior_preservation selection_source_value_id=77 "
                       "selection_source_value=stack.saved.arg",
                       "manual stack-slot prior-preservation source selection identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(manual_stack_prior_dump,
                       "selection_preserved_call_block=0 selection_preserved_call_inst=5 "
                       "selection_preservation_route=stack_slot selection_preserved_bank=none "
                       "selection_preserved_slot=#9 selection_preserved_stack_offset=64 "
                       "selection_preserved_stack_size=4 selection_preserved_stack_align=4",
                       "manual stack-slot prior-preservation source selection payload")) {
    return EXIT_FAILURE;
  }

  const auto f128_memory_prepared = prepare_f128_memory_carrier_dump_module();
  const auto* f128_memory_carrier = find_f128_carrier(
      f128_memory_prepared, "f128_memory_carrier_dump_contract", "p.value");
  if (f128_memory_carrier == nullptr ||
      f128_memory_carrier->kind != prepare::PreparedF128CarrierKind::MemoryBacked ||
      f128_memory_carrier->source_type != bir::TypeKind::F128 ||
      f128_memory_carrier->total_size_bytes != 16 ||
      f128_memory_carrier->total_align_bytes != 16 ||
      !f128_memory_carrier->slot_id.has_value() ||
      !f128_memory_carrier->stack_offset_bytes.has_value() ||
      !f128_memory_carrier->missing_required_facts.empty()) {
    std::cerr << "[FAIL] prepared f128 memory carrier lost frame-slot storage authority\n";
    return EXIT_FAILURE;
  }

  const std::string f128_memory_dump = prepare::print(f128_memory_prepared);
  if (!expect_contains(f128_memory_dump,
                       "--- prepared-f128-carriers ---",
                       "f128 carrier printer section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "f128_carrier p.value value_id=0",
                       "f128 memory carrier value identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "kind=memory_backed size=16 align=16",
                       "f128 memory carrier storage shape")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "slot_id=#",
                       "f128 memory carrier frame-slot id")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "stack_offset=",
                       "f128 memory carrier frame-slot offset")) {
    return EXIT_FAILURE;
  }

  const auto f128_register_prepared = prepare_f128_register_carrier_dump_module();
  const auto* f128_register_carrier = find_f128_carrier(
      f128_register_prepared, "f128_register_carrier_dump_contract", "p.value");
  if (f128_register_carrier == nullptr ||
      f128_register_carrier->kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      f128_register_carrier->source_type != bir::TypeKind::F128 ||
      f128_register_carrier->total_size_bytes != 16 ||
      f128_register_carrier->total_align_bytes != 16 ||
      f128_register_carrier->register_bank != prepare::PreparedRegisterBank::Vreg ||
      f128_register_carrier->register_class != prepare::PreparedRegisterClass::Vector ||
      f128_register_carrier->contiguous_width != 1 ||
      f128_register_carrier->register_name != std::optional<std::string>{"q0"} ||
      f128_register_carrier->occupied_register_names != std::vector<std::string>{"q0"} ||
      !f128_register_carrier->missing_required_facts.empty()) {
    std::cerr << "[FAIL] prepared f128 register carrier lost q-register authority\n";
    return EXIT_FAILURE;
  }

  const std::string f128_register_dump = prepare::print(f128_register_prepared);
  if (!expect_contains(f128_register_dump,
                       "f128_carrier p.value value_id=0",
                       "f128 register carrier value identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_register_dump,
                       "kind=full_width_register size=16 align=16 bank=vreg class=vector",
                       "f128 register carrier storage shape")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_register_dump,
                       "width=1 units=q0 reg=q0",
                       "f128 register carrier q-register occupancy")) {
    return EXIT_FAILURE;
  }

  const auto f128_constant_prepared = prepare_f128_constant_payload_dump_module();
  const char* f128_constant_name =
      "__f128.const.3fff8000000000000123456789abcdef";
  const auto* f128_constant_carrier = find_f128_carrier(
      f128_constant_prepared, "f128_constant_payload_dump_contract", f128_constant_name);
  if (f128_constant_carrier == nullptr ||
      !f128_constant_carrier->constant_payload.has_value() ||
      f128_constant_carrier->constant_payload->low_bits != 0x0123456789abcdefULL ||
      f128_constant_carrier->constant_payload->high_bits != 0x3fff800000000000ULL ||
      f128_constant_carrier->total_size_bytes != 16 ||
      f128_constant_carrier->total_align_bytes != 16) {
    std::cerr << "[FAIL] prepared f128 constant carrier lost exact low/high payload\n";
    return EXIT_FAILURE;
  }

  const std::string f128_constant_dump = prepare::print(f128_constant_prepared);
  if (!expect_contains(
          f128_constant_dump,
          "bir.call void consume_tf(f128 0x3FFF8000000000000123456789ABCDEF)",
          "bir f128 immediate literal preserves both halves")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          f128_constant_dump,
          "home __f128.const.3fff8000000000000123456789abcdef value_id=" +
              std::to_string(f128_constant_carrier->value_id) +
              " kind=rematerializable_immediate imm_f128=0x3FFF8000000000000123456789ABCDEF",
          "f128 prepared value home preserves full-width immediate")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          f128_constant_dump,
          "storage __f128.const.3fff8000000000000123456789abcdef value_id=" +
              std::to_string(f128_constant_carrier->value_id) +
              " encoding=immediate bank=none width=1 "
              "imm_f128=0x3FFF8000000000000123456789ABCDEF",
          "f128 prepared storage preserves full-width immediate")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          f128_constant_dump,
          "constant_payload=0x3FFF8000000000000123456789ABCDEF",
          "f128 carrier full-width constant payload")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          f128_constant_dump,
          "arg index=0 value_bank=vreg source_encoding=immediate source_value_id=" +
              std::to_string(f128_constant_carrier->value_id) + " "
          "source_literal=0x3FFF8000000000000123456789ABCDEF",
          "f128 prepared immediate literal preserves both halves")) {
    return EXIT_FAILURE;
  }

  const auto f128_helper_prepared = prepare_f128_soft_float_helper_dump_module();
  const auto* f128_helper = find_first_f128_runtime_helper(
      f128_helper_prepared, "f128_soft_float_helper_dump_contract");
  if (f128_helper == nullptr ||
      f128_helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Add ||
      f128_helper->callee_name != "__addtf3" ||
      f128_helper->source_type != bir::TypeKind::F128 ||
      f128_helper->result_type != bir::TypeKind::F128 ||
      f128_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_helper->lhs_carrier.has_value() ||
      !f128_helper->rhs_carrier.has_value() ||
      !f128_helper->result_carrier.has_value() ||
      !f128_helper->lhs_abi_argument.has_value() ||
      !f128_helper->rhs_abi_argument.has_value() ||
      !f128_helper->result_abi_result.has_value() ||
      !f128_helper->lhs_argument_move.has_value() ||
      !f128_helper->rhs_argument_move.has_value() ||
      !f128_helper->result_unmarshal_move.has_value() ||
      !f128_helper->resource_policy.caller_saved_clobbers ||
      f128_helper->clobbered_registers.empty() ||
      f128_helper->lhs_carrier->width_bytes != 16 ||
      f128_helper->rhs_carrier->width_bytes != 16 ||
      f128_helper->result_carrier->width_bytes != 16 ||
      f128_helper->lhs_abi_argument->width_bytes != 16 ||
      f128_helper->rhs_abi_argument->width_bytes != 16 ||
      f128_helper->result_abi_result->width_bytes != 16 ||
      f128_helper->lhs_abi_argument->register_name != "q0" ||
      f128_helper->rhs_abi_argument->register_name != "q1" ||
      f128_helper->result_abi_result->register_name != "q0" ||
      f128_helper->lhs_argument_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument ||
      f128_helper->rhs_argument_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument ||
      f128_helper->result_unmarshal_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier ||
      !f128_helper->live_preservation_policy.evaluated ||
      !f128_helper->live_preservation_policy.caller_saved_clobbers_modeled ||
      !f128_helper->live_preservation_policy.no_additional_live_preservation_required ||
      !f128_helper->live_preservation_policy.preserved_values.empty() ||
      !f128_helper->selected_call_ownership.has_clobber_policy ||
      !f128_helper->selected_call_ownership.has_live_preservation ||
      !f128_helper->selected_call_ownership.owns_terminal_call) {
    std::cerr << "[FAIL] prepared f128 soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const auto f128_vreg_clobber_it = std::find_if(
      f128_helper->clobbered_registers.begin(),
      f128_helper->clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.bank == prepare::PreparedRegisterBank::Vreg &&
               clobber.register_name == "v0" &&
               clobber.contiguous_width == 1 &&
               clobber.occupied_register_names.size() == 1 &&
               clobber.occupied_register_names.front() == "v0" &&
               clobber.placement.has_value() &&
               clobber.placement->pool ==
                   prepare::PreparedRegisterSlotPool::ReservedScratch;
      });
  if (f128_vreg_clobber_it == f128_helper->clobbered_registers.end()) {
    std::cerr
        << "[FAIL] prepared f128 soft-float helper lost structured vreg clobber authority\n";
    return EXIT_FAILURE;
  }
  const auto has_f128_missing_fact = [&](std::string_view fact) {
    return std::any_of(f128_helper->missing_required_facts.begin(),
                       f128_helper->missing_required_facts.end(),
                       [&](const std::string& candidate) {
                         return candidate == fact;
                       });
  };
  if (has_f128_missing_fact("f128_helper_boundary_requires_live_preservation_policy") ||
      has_f128_missing_fact("selected_call_ownership_requires_live_preservation_policy") ||
      has_f128_missing_fact("live_preservation_requires_structured_live_across_helper_facts") ||
      has_f128_missing_fact("live_preservation_requires_complete_preserved_value_routes") ||
      has_f128_missing_fact("f128_helper_boundary_requires_caller_saved_clobber_policy") ||
      has_f128_missing_fact("f128_helper_boundary_requires_caller_saved_clobbers") ||
      has_f128_missing_fact("selected_call_ownership_requires_clobber_policy") ||
      has_f128_missing_fact("selected_call_ownership_requires_resource_policy") ||
      has_f128_missing_fact("selected_call_ownership_requires_abi_bindings") ||
      has_f128_missing_fact("selected_call_ownership_requires_marshaling")) {
    std::cerr
        << "[FAIL] prepared f128 soft-float helper did not fail closed on missing authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_helper_dump = prepare::print(f128_helper_prepared);
  if (!expect_contains(f128_helper_dump,
                       "--- prepared-f128-runtime-helpers ---",
                       "f128 runtime helper printer section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=add opcode=add "
                       "callee=__addtf3 source_type=f128 result_type=f128 result=sum#",
                       "f128 add helper identity and callee")) {
    return EXIT_FAILURE;
  }
  const auto f128_constant_rhs_helper_prepared =
      prepare_f128_soft_float_helper_dump_module(
          bir::BinaryOpcode::Add,
          "f128_soft_float_constant_rhs_helper_dump_contract",
          "sum",
          true);
  const auto* f128_constant_rhs_helper = find_first_f128_runtime_helper(
      f128_constant_rhs_helper_prepared,
      "f128_soft_float_constant_rhs_helper_dump_contract");
  if (f128_constant_rhs_helper == nullptr ||
      f128_constant_rhs_helper->rhs_value_name == c4c::kInvalidValueName ||
      f128_constant_rhs_helper_prepared.names.value_names
              .spelling(f128_constant_rhs_helper->rhs_value_name)
              .find("__f128.const.3fff8000000000000123456789abcdef") != 0 ||
      !f128_constant_rhs_helper->rhs_carrier.has_value() ||
      f128_constant_rhs_helper->rhs_carrier->carrier_kind !=
          prepare::PreparedF128CarrierKind::Missing) {
    std::cerr
        << "[FAIL] prepared f128 helper did not resolve full-width constant rhs carrier\n";
    return EXIT_FAILURE;
  }
  const std::string f128_constant_rhs_helper_dump =
      prepare::print(f128_constant_rhs_helper_prepared);
  if (!expect_contains(f128_constant_rhs_helper_dump,
                       "constant_payload=0x3FFF8000000000000123456789ABCDEF",
                       "f128 helper constant rhs preserves full-width payload")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_constant_rhs_helper_dump,
                       "rhs_requires_full_width_f128_carrier",
                       "f128 helper constant rhs remains fail-closed before helper rematerialization")) {
    return EXIT_FAILURE;
  }
  const auto f128_sub_helper_prepared =
      prepare_f128_soft_float_helper_dump_module(
          bir::BinaryOpcode::Sub,
          "f128_soft_float_sub_helper_dump_contract",
          "diff");
  const auto* f128_sub_helper = find_first_f128_runtime_helper(
      f128_sub_helper_prepared, "f128_soft_float_sub_helper_dump_contract");
  if (f128_sub_helper == nullptr ||
      f128_sub_helper->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_sub_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Sub ||
      f128_sub_helper->callee_name != "__subtf3" ||
      f128_sub_helper->source_binary_opcode != bir::BinaryOpcode::Sub ||
      f128_sub_helper->source_type != bir::TypeKind::F128 ||
      f128_sub_helper->result_type != bir::TypeKind::F128 ||
      f128_sub_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_sub_helper->selected_call_ownership.owns_terminal_call ||
      !f128_sub_helper->selected_call_ownership.has_marshaling ||
      !f128_sub_helper->selected_call_ownership.has_live_preservation) {
    std::cerr << "[FAIL] prepared f128 sub soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_sub_helper_dump = prepare::print(f128_sub_helper_prepared);
  if (!expect_contains(f128_sub_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=sub opcode=sub "
                       "callee=__subtf3 source_type=f128 result_type=f128 result=diff#",
                       "f128 sub helper identity and callee")) {
    return EXIT_FAILURE;
  }
  const auto f128_mul_helper_prepared =
      prepare_f128_soft_float_helper_dump_module(
          bir::BinaryOpcode::Mul,
          "f128_soft_float_mul_helper_dump_contract",
          "product");
  const auto* f128_mul_helper = find_first_f128_runtime_helper(
      f128_mul_helper_prepared, "f128_soft_float_mul_helper_dump_contract");
  if (f128_mul_helper == nullptr ||
      f128_mul_helper->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_mul_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Mul ||
      f128_mul_helper->callee_name != "__multf3" ||
      f128_mul_helper->source_binary_opcode != bir::BinaryOpcode::Mul ||
      f128_mul_helper->source_type != bir::TypeKind::F128 ||
      f128_mul_helper->result_type != bir::TypeKind::F128 ||
      f128_mul_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_mul_helper->selected_call_ownership.owns_terminal_call ||
      !f128_mul_helper->selected_call_ownership.has_marshaling ||
      !f128_mul_helper->selected_call_ownership.has_live_preservation) {
    std::cerr << "[FAIL] prepared f128 mul soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_mul_helper_dump = prepare::print(f128_mul_helper_prepared);
  if (!expect_contains(f128_mul_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=mul opcode=mul "
                       "callee=__multf3 source_type=f128 result_type=f128 result=product#",
                       "f128 mul helper identity and callee")) {
    return EXIT_FAILURE;
  }
  const auto f128_div_helper_prepared =
      prepare_f128_soft_float_helper_dump_module(
          bir::BinaryOpcode::SDiv,
          "f128_soft_float_div_helper_dump_contract",
          "quotient");
  const auto* f128_div_helper = find_first_f128_runtime_helper(
      f128_div_helper_prepared, "f128_soft_float_div_helper_dump_contract");
  if (f128_div_helper == nullptr ||
      f128_div_helper->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_div_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Div ||
      f128_div_helper->callee_name != "__divtf3" ||
      f128_div_helper->source_binary_opcode != bir::BinaryOpcode::SDiv ||
      f128_div_helper->source_type != bir::TypeKind::F128 ||
      f128_div_helper->result_type != bir::TypeKind::F128 ||
      f128_div_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_div_helper->selected_call_ownership.owns_terminal_call ||
      !f128_div_helper->selected_call_ownership.has_marshaling ||
      !f128_div_helper->selected_call_ownership.has_live_preservation) {
    std::cerr << "[FAIL] prepared f128 div soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_div_helper_dump = prepare::print(f128_div_helper_prepared);
  if (!expect_contains(f128_div_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=div opcode=sdiv "
                       "callee=__divtf3 source_type=f128 result_type=f128 result=quotient#",
                       "f128 div helper identity and callee")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result_ownership=full_width_carrier "
                       "resources=[call_boundary,runtime_helper_callee,caller_saved_clobbers,"
                       "source_operation_identity]",
                       "f128 helper resource record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "clobbers=gpr:x13/w1[x13],fpr:d13/w1[d13],vreg:v0/w1[v0]",
                       "f128 helper caller-saved clobber summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "carriers lhs=lhs#",
                       "f128 helper lhs carrier record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "rhs=rhs#",
                       "f128 helper rhs carrier record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result=sum#",
                       "f128 helper result carrier record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "abi_bindings lhs=lhs#",
                       "f128 helper lhs ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "arg=0,abi_index=0,bank=vreg,class=vector,abi_reg=q0",
                       "f128 helper lhs ABI q-register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "rhs=rhs#",
                       "f128 helper rhs ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "arg=1,abi_index=1,bank=vreg,class=vector,abi_reg=q1",
                       "f128 helper rhs ABI q-register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result=sum#",
                       "f128 helper result ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result,abi_index=0,bank=vreg,class=vector,abi_reg=q0",
                       "f128 helper result ABI q-register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "marshaling lhs=carrier_to_abi_argument",
                       "f128 helper lhs marshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "rhs=carrier_to_abi_argument",
                       "f128 helper rhs marshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result=abi_result_to_carrier",
                       "f128 helper result unmarshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "live_preservation=[evaluated=yes,caller_saved_clobbers=yes,"
                       "additional=none,preserved=0]",
                       "f128 helper live-preservation authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "selected_call_ownership=[owns_terminal_call=yes,callee=yes,resources=yes,"
                       "clobbers=yes,abi_bindings=yes,marshaling=yes,live_preservation=yes]",
                       "f128 helper complete selected ownership")) {
    return EXIT_FAILURE;
  }

  const auto f32_to_f128_prepared =
      prepare_f128_cast_helper_dump_module(bir::CastOpcode::FPExt,
                                           bir::TypeKind::F32,
                                           bir::TypeKind::F128,
                                           "f128_cast_f32_to_f128_dump_contract",
                                           "wide");
  const auto* f32_to_f128_helper = find_first_f128_runtime_helper(
      f32_to_f128_prepared, "f128_cast_f32_to_f128_dump_contract");
  if (f32_to_f128_helper == nullptr ||
      f32_to_f128_helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Cast ||
      f32_to_f128_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::F32ToF128 ||
      f32_to_f128_helper->source_cast_opcode != bir::CastOpcode::FPExt ||
      f32_to_f128_helper->callee_name != "__extendsftf2" ||
      f32_to_f128_helper->source_type != bir::TypeKind::F32 ||
      f32_to_f128_helper->result_type != bir::TypeKind::F128 ||
      !f32_to_f128_helper->scalar_operand.has_value() ||
      !f32_to_f128_helper->result_carrier.has_value() ||
      !f32_to_f128_helper->scalar_operand_argument_move.has_value() ||
      !f32_to_f128_helper->result_unmarshal_move.has_value() ||
      f32_to_f128_helper->abi_policy.transition !=
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result ||
      f32_to_f128_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f32_to_f128_helper->selected_call_ownership.owns_terminal_call) {
    std::cerr << "[FAIL] prepared f32->f128 cast helper lost structured authority\n";
    return EXIT_FAILURE;
  }
  const std::string f32_to_f128_dump = prepare::print(f32_to_f128_prepared);
  if (!expect_contains(f32_to_f128_dump,
                       "family=cast kind=f32_to_f128 opcode=fpext callee=__extendsftf2 "
                       "source_type=f32 result_type=f128 operand=input#",
                       "f32 to f128 cast helper identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f32_to_f128_dump,
                       "abi_transition=direct_scalar_argument_and_f128_result arg_bank=fpr "
                       "result_bank=vreg arg_count=1 result_count=1 width=16",
                       "f32 to f128 cast ABI policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f32_to_f128_dump,
                       "scalar_operand=scalar_to_abi_argument",
                       "f32 to f128 scalar argument marshaling")) {
    return EXIT_FAILURE;
  }

  const auto f128_to_f64_prepared =
      prepare_f128_cast_helper_dump_module(bir::CastOpcode::FPTrunc,
                                           bir::TypeKind::F128,
                                           bir::TypeKind::F64,
                                           "f128_cast_f128_to_f64_dump_contract",
                                           "narrow");
  const auto* f128_to_f64_helper = find_first_f128_runtime_helper(
      f128_to_f64_prepared, "f128_cast_f128_to_f64_dump_contract");
  if (f128_to_f64_helper == nullptr ||
      f128_to_f64_helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Cast ||
      f128_to_f64_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::F128ToF64 ||
      f128_to_f64_helper->source_cast_opcode != bir::CastOpcode::FPTrunc ||
      f128_to_f64_helper->callee_name != "__trunctfdf2" ||
      !f128_to_f64_helper->lhs_carrier.has_value() ||
      !f128_to_f64_helper->scalar_result.has_value() ||
      !f128_to_f64_helper->lhs_argument_move.has_value() ||
      !f128_to_f64_helper->scalar_result_unmarshal_move.has_value() ||
      f128_to_f64_helper->abi_policy.transition !=
          prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult ||
      f128_to_f64_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue ||
      !f128_to_f64_helper->selected_call_ownership.owns_terminal_call) {
    std::cerr << "[FAIL] prepared f128->f64 cast helper lost structured authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_to_f64_dump = prepare::print(f128_to_f64_prepared);
  if (!expect_contains(f128_to_f64_dump,
                       "family=cast kind=f128_to_f64 opcode=fptrunc callee=__trunctfdf2 "
                       "source_type=f128 result_type=f64 operand=input#",
                       "f128 to f64 cast helper identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_to_f64_dump,
                       "abi_transition=direct_f128_argument_and_scalar_result arg_bank=vreg "
                       "result_bank=fpr arg_count=1 result_count=1 width=8",
                       "f128 to f64 cast ABI policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_to_f64_dump,
                       "scalar_result=abi_result_to_scalar",
                       "f128 to f64 scalar result marshaling")) {
    return EXIT_FAILURE;
  }

  const auto grouped_cross_call_prepared = prepare_grouped_cross_call_preservation_dump_module();
  const auto* grouped_cross_call_plans = find_call_plans_function(
      grouped_cross_call_prepared, "grouped_cross_call_preservation_dump_contract");
  const auto* grouped_cross_call_storage = find_storage_plan_function(
      grouped_cross_call_prepared, "grouped_cross_call_preservation_dump_contract");
  const auto* grouped_cross_call_carry =
      grouped_cross_call_storage == nullptr
          ? nullptr
          : find_storage_value(grouped_cross_call_prepared, *grouped_cross_call_storage, "carry.pre");
  const auto grouped_cross_call_function_id =
      grouped_cross_call_prepared.names.function_names.find(
          "grouped_cross_call_preservation_dump_contract");
  const auto* grouped_cross_call_frame_plan =
      grouped_cross_call_function_id == c4c::kInvalidFunctionName
          ? nullptr
          : prepare::find_prepared_frame_plan(grouped_cross_call_prepared,
                                              grouped_cross_call_function_id);
  if (grouped_cross_call_plans == nullptr || grouped_cross_call_plans->calls.size() != 1 ||
      grouped_cross_call_frame_plan == nullptr || grouped_cross_call_carry == nullptr) {
    std::cerr << "[FAIL] missing grouped cross-call preservation dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto& grouped_cross_call = grouped_cross_call_plans->calls.front();
  const auto grouped_preserved_it = std::find_if(
      grouped_cross_call.preserved_values.begin(),
      grouped_cross_call.preserved_values.end(),
      [](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.contiguous_width == 2 &&
               preserved.register_bank ==
                   std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Vreg};
      });
  if (grouped_preserved_it == grouped_cross_call.preserved_values.end() ||
      !grouped_preserved_it->register_name.has_value() ||
      !grouped_preserved_it->callee_saved_save_index.has_value() ||
      !grouped_preserved_it->register_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped preserved-value authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto grouped_saved_it = std::find_if(
      grouped_cross_call_frame_plan->saved_callee_registers.begin(),
      grouped_cross_call_frame_plan->saved_callee_registers.end(),
      [&](const prepare::PreparedSavedRegister& saved) {
        return saved.occupied_register_names == grouped_preserved_it->occupied_register_names;
      });
  if (grouped_saved_it == grouped_cross_call_frame_plan->saved_callee_registers.end()) {
    std::cerr << "[FAIL] missing grouped saved-register span authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!grouped_saved_it->placement.has_value() ||
      !grouped_saved_it->slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *grouped_saved_it->slot_placement) ||
      !grouped_cross_call_carry->register_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped structured placement identity in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string grouped_cross_call_dump = prepare::print(grouped_cross_call_prepared);
  if (!expect_contains(grouped_cross_call_dump,
                       "saved vreg:" + grouped_saved_it->register_name +
                           " order=" + std::to_string(grouped_saved_it->save_index) +
                           " " + register_placement_text(grouped_saved_it->placement) +
                           " width=2 units=" +
                           grouped_saved_it->occupied_register_names.front() + "," +
                           grouped_saved_it->occupied_register_names.back() + " " +
                           saved_register_slot_placement_text(grouped_saved_it->slot_placement),
                       "grouped saved-register slot placement summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "saved_register bank=vreg " +
                           register_placement_text(grouped_saved_it->placement) +
                           " reg=" + grouped_saved_it->register_name +
                           " save_index=" + std::to_string(grouped_saved_it->save_index) +
                           " width=2 units=" +
                           grouped_saved_it->occupied_register_names.front() + "," +
                           grouped_saved_it->occupied_register_names.back() + " " +
                           saved_register_slot_placement_text(grouped_saved_it->slot_placement),
                       "grouped frame-plan saved-register slot placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "carry.pre#" + std::to_string(grouped_preserved_it->value_id) +
                           ":callee_saved_register:" + *grouped_preserved_it->register_name +
                           "/w2[" + grouped_preserved_it->occupied_register_names.front() + "," +
                           grouped_preserved_it->occupied_register_names.back() + "]" + ":save" +
                           std::to_string(*grouped_preserved_it->callee_saved_save_index),
                       "grouped cross-call preservation summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "preserve value=carry.pre value_id=" +
                           std::to_string(grouped_preserved_it->value_id) +
                           " route=callee_saved_register save_index=" +
                           std::to_string(*grouped_preserved_it->callee_saved_save_index) +
                           " " + register_placement_text(grouped_preserved_it->register_placement) +
                           " reg=" + *grouped_preserved_it->register_name +
                           " width=2 bank=vreg units=" +
                           grouped_saved_it->occupied_register_names.front() + "," +
                           grouped_saved_it->occupied_register_names.back(),
                       "grouped cross-call preservation detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "storage carry.pre value_id=" +
                           std::to_string(grouped_cross_call_carry->value_id) +
                           " encoding=register bank=vreg " +
                           register_placement_text(grouped_cross_call_carry->register_placement) +
                           " reg=" +
                           *grouped_cross_call_carry->register_name +
                           " width=2 units=" +
                           grouped_cross_call_carry->occupied_register_names.front() + "," +
                           grouped_cross_call_carry->occupied_register_names.back(),
                       "grouped storage plan detail")) {
    return EXIT_FAILURE;
  }
  const auto grouped_clobber_it = std::find_if(
      grouped_cross_call.clobbered_registers.begin(),
      grouped_cross_call.clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.bank == prepare::PreparedRegisterBank::Vreg &&
               clobber.contiguous_width == 2 &&
               clobber.occupied_register_names.size() == 2;
      });
  if (grouped_clobber_it == grouped_cross_call.clobbered_registers.end()) {
    std::cerr << "[FAIL] missing grouped clobber span authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!grouped_clobber_it->placement.has_value()) {
    std::cerr << "[FAIL] missing grouped clobber structured placement in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       clobber_summary(*grouped_clobber_it),
                       "grouped call clobber summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       clobber_detail(*grouped_clobber_it),
                       "grouped call clobber detail")) {
    return EXIT_FAILURE;
  }

  const auto grouped_spill_reload_prepared = prepare_grouped_spill_reload_dump_module();
  const auto consumed = c4c::backend::x86::consume_plans(grouped_spill_reload_prepared,
                                                         "grouped_spill_reload_dump_contract");
  if (consumed.regalloc == nullptr || consumed.storage == nullptr) {
    std::cerr << "[FAIL] missing grouped spill/reload dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto grouped_spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [&](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Vreg &&
               op.contiguous_width == 16 &&
               op.occupied_register_names.size() == 16 &&
               op.occupied_register_names.front() == "v0" &&
               op.occupied_register_names.back() == "v15";
      });
  if (grouped_spill_it == consumed.regalloc->spill_reload_ops.end()) {
    std::cerr << "[FAIL] missing grouped spill/reload authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto* grouped_spill_value = find_regalloc_value(*consumed.regalloc, grouped_spill_it->value_id);
  const std::string grouped_spill_value_name =
      grouped_spill_value == nullptr
          ? std::string{}
          : std::string(prepare::prepared_value_name(grouped_spill_reload_prepared.names,
                                                     grouped_spill_value->value_name));
  if (grouped_spill_value == nullptr ||
      grouped_spill_it->source_value_name != grouped_spill_value->value_name ||
      !grouped_spill_it->slot_id.has_value() || !grouped_spill_it->stack_offset_bytes.has_value() ||
      !grouped_spill_it->register_placement.has_value() ||
      !grouped_spill_it->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped spill slot authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string grouped_spill_reload_dump = prepare::print(grouped_spill_reload_prepared);
  if (!expect_contains(grouped_spill_reload_dump,
                       "spill_reload kind=spill value_id=" +
                           std::to_string(grouped_spill_it->value_id) +
                           " value=" + grouped_spill_value_name +
                           " source_value=" + grouped_spill_value_name + " block_index=" +
                           std::to_string(grouped_spill_it->block_index) +
                           " inst_index=" +
                           std::to_string(grouped_spill_it->instruction_index) +
                           " bank=vreg class=vector " +
                           register_placement_text(grouped_spill_it->register_placement) +
                           " " + spill_slot_placement_text(grouped_spill_it->spill_slot_placement) +
                           " reg=v0 width=16 units=v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15 slot_id=#" +
                           std::to_string(*grouped_spill_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*grouped_spill_it->stack_offset_bytes),
                       "grouped spill detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_spill_reload_dump,
                           "spill_reload kind=reload value_id=" +
                           std::to_string(grouped_spill_it->value_id) + " value=" +
                           grouped_spill_value_name + " source_value=" + grouped_spill_value_name,
                       "grouped reload detail")) {
    return EXIT_FAILURE;
  }

  const auto general_grouped_spill_reload_prepared =
      prepare_general_grouped_spill_reload_dump_module();
  const auto general_consumed = c4c::backend::x86::consume_plans(
      general_grouped_spill_reload_prepared, "general_grouped_spill_reload_dump_contract");
  if (general_consumed.regalloc == nullptr || general_consumed.storage == nullptr) {
    std::cerr << "[FAIL] missing grouped general spill/reload dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto general_spill_it = std::find_if(
      general_consumed.regalloc->spill_reload_ops.begin(),
      general_consumed.regalloc->spill_reload_ops.end(),
      [&](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Gpr &&
               op.register_name == std::optional<std::string>{"s1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"s1", "s2"};
      });
  if (general_spill_it == general_consumed.regalloc->spill_reload_ops.end()) {
    std::cerr << "[FAIL] missing grouped general spill/reload authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto* general_spill_value =
      find_regalloc_value(*general_consumed.regalloc, general_spill_it->value_id);
  const auto* general_storage_carry = find_storage_value(
      general_grouped_spill_reload_prepared, *general_consumed.storage, "carry");
  const std::string general_spill_value_name =
      general_spill_value == nullptr
          ? std::string{}
          : std::string(prepare::prepared_value_name(general_grouped_spill_reload_prepared.names,
                                                     general_spill_value->value_name));
  if (general_spill_value == nullptr || general_storage_carry == nullptr ||
      general_spill_it->source_value_name != general_spill_value->value_name ||
      !general_spill_it->slot_id.has_value() ||
      !general_spill_it->stack_offset_bytes.has_value() ||
      !general_spill_it->register_placement.has_value() ||
      !general_spill_it->spill_slot_placement.has_value() ||
      !general_storage_carry->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped general spill slot authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string general_grouped_spill_reload_dump =
      prepare::print(general_grouped_spill_reload_prepared);
  if (!expect_contains(general_grouped_spill_reload_dump,
                       "spill_reload kind=spill value_id=" +
                           std::to_string(general_spill_it->value_id) +
                           " value=" + general_spill_value_name +
                           " source_value=" + general_spill_value_name + " block_index=" +
                           std::to_string(general_spill_it->block_index) +
                           " inst_index=" +
                           std::to_string(general_spill_it->instruction_index) +
                           " bank=gpr class=general " +
                           register_placement_text(general_spill_it->register_placement) +
                           " " + spill_slot_placement_text(general_spill_it->spill_slot_placement) +
                           " reg=s1 width=2 units=s1,s2 slot_id=#" +
                           std::to_string(*general_spill_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*general_spill_it->stack_offset_bytes),
                       "grouped general spill detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(general_grouped_spill_reload_dump,
                           "spill_reload kind=reload value_id=" +
                           std::to_string(general_spill_it->value_id) + " value=" +
                           general_spill_value_name + " source_value=" + general_spill_value_name,
                       "grouped general reload detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(general_grouped_spill_reload_dump,
                       "storage carry value_id=" +
                           std::to_string(general_storage_carry->value_id) +
                           " encoding=frame_slot bank=gpr " +
                           spill_slot_placement_text(general_storage_carry->spill_slot_placement) +
                           " width=2 slot_id=#" +
                           std::to_string(*general_storage_carry->slot_id) +
                           " stack_offset=" +
                           std::to_string(*general_storage_carry->stack_offset_bytes),
                       "grouped general storage detail")) {
    return EXIT_FAILURE;
  }

  const auto float_grouped_spill_reload_prepared = prepare_float_grouped_spill_reload_dump_module();
  const auto float_consumed = c4c::backend::x86::consume_plans(
      float_grouped_spill_reload_prepared, "float_grouped_spill_reload_dump_contract");
  if (float_consumed.regalloc == nullptr || float_consumed.storage == nullptr) {
    std::cerr << "[FAIL] missing grouped float spill/reload dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto float_spill_it = std::find_if(
      float_consumed.regalloc->spill_reload_ops.begin(),
      float_consumed.regalloc->spill_reload_ops.end(),
      [&](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Fpr &&
               op.register_name == std::optional<std::string>{"fs1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"fs1", "fs2"};
      });
  if (float_spill_it == float_consumed.regalloc->spill_reload_ops.end()) {
    std::cerr << "[FAIL] missing grouped float spill/reload authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto* float_spill_value =
      find_regalloc_value(*float_consumed.regalloc, float_spill_it->value_id);
  const auto* float_storage_carry =
      find_storage_value(float_grouped_spill_reload_prepared, *float_consumed.storage, "carry");
  const std::string float_spill_value_name =
      float_spill_value == nullptr
          ? std::string{}
          : std::string(prepare::prepared_value_name(float_grouped_spill_reload_prepared.names,
                                                     float_spill_value->value_name));
  if (float_spill_value == nullptr || float_storage_carry == nullptr ||
      float_spill_it->source_value_name != float_spill_value->value_name ||
      !float_spill_it->slot_id.has_value() || !float_spill_it->stack_offset_bytes.has_value() ||
      !float_spill_it->register_placement.has_value() ||
      !float_spill_it->spill_slot_placement.has_value() ||
      !float_storage_carry->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped float spill slot authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string float_grouped_spill_reload_dump =
      prepare::print(float_grouped_spill_reload_prepared);
  if (!expect_contains(float_grouped_spill_reload_dump,
                       "spill_reload kind=spill value_id=" +
                           std::to_string(float_spill_it->value_id) +
                           " value=" + float_spill_value_name +
                           " source_value=" + float_spill_value_name + " block_index=" +
                           std::to_string(float_spill_it->block_index) +
                           " inst_index=" +
                           std::to_string(float_spill_it->instruction_index) +
                           " bank=fpr class=float " +
                           register_placement_text(float_spill_it->register_placement) +
                           " " + spill_slot_placement_text(float_spill_it->spill_slot_placement) +
                           " reg=fs1 width=2 units=fs1,fs2 slot_id=#" +
                           std::to_string(*float_spill_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*float_spill_it->stack_offset_bytes),
                       "grouped float spill detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(float_grouped_spill_reload_dump,
                           "spill_reload kind=reload value_id=" +
                           std::to_string(float_spill_it->value_id) + " value=" +
                           float_spill_value_name + " source_value=" + float_spill_value_name,
                       "grouped float reload detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(float_grouped_spill_reload_dump,
                       "storage carry value_id=" +
                           std::to_string(float_storage_carry->value_id) +
                           " encoding=frame_slot bank=fpr " +
                           spill_slot_placement_text(float_storage_carry->spill_slot_placement) +
                           " width=2 slot_id=#" +
                           std::to_string(*float_storage_carry->slot_id) +
                           " stack_offset=" +
                           std::to_string(*float_storage_carry->stack_offset_bytes),
                       "grouped float storage detail")) {
    return EXIT_FAILURE;
  }

  const auto call_wrapper_call_plans =
      find_call_plans_function(call_wrapper_prepared, "call_wrapper_dump_contract");
  if (call_wrapper_call_plans == nullptr || call_wrapper_call_plans->calls.size() < 2 ||
      call_wrapper_call_plans->calls[1].clobbered_registers.empty()) {
    std::cerr << "[FAIL] missing prepared clobber fixture for call wrapper dump\n";
    return EXIT_FAILURE;
  }
  const auto& fixed_call = call_wrapper_call_plans->calls[1];
  if (fixed_call.arguments.empty() ||
      !fixed_call.arguments.front().destination_register_name.has_value() ||
      !fixed_call.arguments.front().destination_register_placement.has_value() ||
      !fixed_call.result.has_value() ||
      !fixed_call.result->destination_value_id.has_value() ||
      !fixed_call.result->source_register_name.has_value() ||
      !fixed_call.result->source_register_placement.has_value()) {
    std::cerr << "[FAIL] missing structured call arg/result placement in call wrapper dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       register_placement_text(fixed_call.arguments.front().destination_register_placement,
                                               "dest_placement") +
                           " dest_reg=" + *fixed_call.arguments.front().destination_register_name,
                       "call argument structured destination placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "result value_bank=gpr source_storage=register destination_storage=register "
                           "destination_value_id=" +
                           std::to_string(*fixed_call.result->destination_value_id) + " " +
                           register_placement_text(fixed_call.result->source_register_placement,
                                                   "source_placement") +
                           " source_reg=" + *fixed_call.result->source_register_name,
                       "call result structured source placement")) {
    return EXIT_FAILURE;
  }
  const auto* call_wrapper_locations =
      prepare::find_prepared_value_location_function(call_wrapper_prepared,
                                                     "call_wrapper_dump_contract");
  const auto* fixed_before_call_bundle =
      call_wrapper_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*call_wrapper_locations,
                                               prepare::PreparedMovePhase::BeforeCall,
                                               0,
                                               1);
  const auto* fixed_after_call_bundle =
      call_wrapper_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*call_wrapper_locations,
                                               prepare::PreparedMovePhase::AfterCall,
                                               0,
                                               1);
  if (fixed_before_call_bundle == nullptr || fixed_after_call_bundle == nullptr) {
    std::cerr << "[FAIL] missing fixed-call move bundles in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto fixed_arg_binding_it = std::find_if(
      fixed_before_call_bundle->abi_bindings.begin(),
      fixed_before_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
               binding.destination_register_placement.has_value();
      });
  const auto fixed_result_binding_it = std::find_if(
      fixed_after_call_bundle->abi_bindings.begin(),
      fixed_after_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallResultAbi &&
               binding.destination_register_placement.has_value();
      });
  if (fixed_arg_binding_it == fixed_before_call_bundle->abi_bindings.end() ||
      fixed_result_binding_it == fixed_after_call_bundle->abi_bindings.end()) {
    std::cerr << "[FAIL] missing structured ABI-binding placement in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "abi_binding destination_kind=call_argument_abi destination_storage=register "
                           "abi_index=0 " +
                           register_placement_text(
                               fixed_arg_binding_it->destination_register_placement,
                               "placement") +
                           " reg=" + *fixed_arg_binding_it->destination_register_name,
                       "call argument ABI binding structured placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "abi_binding destination_kind=call_result_abi destination_storage=register " +
                           register_placement_text(
                               fixed_result_binding_it->destination_register_placement,
                               "placement") +
                           " reg=" + *fixed_result_binding_it->destination_register_name,
                       "call result ABI binding structured placement")) {
    return EXIT_FAILURE;
  }
  const auto fixed_result_move_it = std::find_if(
      fixed_after_call_bundle->moves.begin(),
      fixed_after_call_bundle->moves.end(),
      [&](const prepare::PreparedMoveResolution& move) {
        return move.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallResultAbi &&
               fixed_call.result->destination_value_id.has_value() &&
               move.to_value_id == *fixed_call.result->destination_value_id &&
               move.destination_register_placement.has_value();
      });
  if (fixed_result_move_it == fixed_after_call_bundle->moves.end()) {
    std::cerr << "[FAIL] missing structured move destination placement in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "move from_value_id=" +
                           std::to_string(fixed_result_move_it->from_value_id) +
                           " to_value_id=" +
                           std::to_string(fixed_result_move_it->to_value_id) +
                           " destination_kind=call_result_abi destination_storage=register "
                           "op_kind=move uses_cycle_temp_source=no " +
                           register_placement_text(
                               fixed_result_move_it->destination_register_placement,
                               "placement"),
                       "call result move structured destination placement")) {
    return EXIT_FAILURE;
  }
  const std::string fixed_call_clobber_summary =
      "clobbers=" + clobber_summary(call_wrapper_call_plans->calls[1].clobbered_registers.front());
  if (!expect_contains(call_wrapper_dump,
                       fixed_call_clobber_summary,
                       "call summary clobber authority")) {
    return EXIT_FAILURE;
  }
  const std::string fixed_call_clobber_detail =
      clobber_detail(call_wrapper_call_plans->calls[1].clobbered_registers.front());
  if (!expect_contains(call_wrapper_dump,
                       fixed_call_clobber_detail,
                       "call detail clobber authority")) {
    return EXIT_FAILURE;
  }

  const auto stack_arg_prepared = prepare_stack_argument_slot_dump_module();
  const auto* stack_arg_object = find_stack_object(stack_arg_prepared, "p.byval");
  const auto* stack_arg_slot =
      stack_arg_object == nullptr ? nullptr : find_frame_slot(stack_arg_prepared, stack_arg_object->object_id);
  if (stack_arg_object == nullptr || stack_arg_slot == nullptr) {
    std::cerr << "[FAIL] missing prepared byval home slot fixture for stack argument dump\n";
    return EXIT_FAILURE;
  }
  const std::string stack_arg_dump = prepare::print(stack_arg_prepared);
  if (!expect_contains(stack_arg_dump,
                       "arg0 bank=aggregate_address from=frame_slot:stack+0 to=rdi",
                       "stack-backed byval argument summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_arg_dump,
                       "arg index=0 value_bank=aggregate_address source_encoding=frame_slot",
                       "stack-backed byval argument detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_arg_dump,
                       "source_slot=#" + std::to_string(stack_arg_slot->slot_id),
                       "stack-backed byval argument frame-slot identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_arg_dump,
                       "source_stack_offset=" + std::to_string(stack_arg_slot->offset_bytes),
                       "stack-backed byval argument stack offset")) {
    return EXIT_FAILURE;
  }

  const auto stack_result_prepared = prepare_stack_result_slot_dump_module();
  const auto* stack_result_storage =
      find_storage_plan_function(stack_result_prepared, "stack_result_slot_dump_contract");
  const auto* stack_result_value =
      stack_result_storage == nullptr
          ? nullptr
          : find_storage_value(stack_result_prepared, *stack_result_storage, "tmp.call.1");
  if (stack_result_storage == nullptr || stack_result_value == nullptr ||
      !stack_result_value->slot_id.has_value() || !stack_result_value->stack_offset_bytes.has_value()) {
    std::cerr << "[FAIL] missing prepared spilled-result fixture for call result dump\n";
    return EXIT_FAILURE;
  }
  const std::string stack_result_dump = prepare::print(stack_result_prepared);
  if (!expect_contains(stack_result_dump,
                       "result bank=gpr from=register:",
                       "stack-backed scalar result summary source shape")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "result value_bank=gpr source_storage=register destination_storage=stack_slot",
                       "stack-backed scalar result detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "destination_value_id=" + std::to_string(stack_result_value->value_id),
                       "stack-backed scalar result direct owner identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "destination_slot=#" + std::to_string(*stack_result_value->slot_id),
                       "stack-backed scalar result frame-slot identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "dest_stack_offset=" + std::to_string(*stack_result_value->stack_offset_bytes),
                       "stack-backed scalar result stack offset")) {
    return EXIT_FAILURE;
  }

  auto provenance_prepared = prepared_module_printer_body_fixture();
  const auto provenance_function =
      provenance_prepared.names.function_names.intern("provenance_dump_contract");
  const auto provenance_block =
      provenance_prepared.names.block_labels.intern("entry");
  const auto provenance_result =
      provenance_prepared.names.value_names.intern("%loaded");
  provenance_prepared.addressing.functions.push_back(
      prepare::PreparedAddressingFunction{
          .function_name = provenance_function,
          .accesses =
              {
                  prepare::PreparedMemoryAccess{
                      .function_name = provenance_function,
                      .block_label = provenance_block,
                      .inst_index = 0,
                      .result_value_name = provenance_result,
                      .address =
                          prepare::PreparedAddress{
                              .base_kind =
                                  prepare::PreparedAddressBaseKind::PointerValue,
                              .pointer_value_name = provenance_result,
                              .byte_offset = 16,
                              .size_bytes = 4,
                              .align_bytes = 4,
                              .can_use_base_plus_offset = true,
                              .provenance =
                                  bir::MemoryAccessProvenance{
                                      .layout_authority =
                                          bir::MemoryLayoutAuthorityKind::
                                              OpaqueCompatibility,
                                      .dynamic_array =
                                          bir::MemoryDynamicArrayFacts{
                                              .verdict =
                                                  bir::MemoryDynamicArrayRangeVerdict::
                                                      Unbounded,
                                          },
                                      .range_verdict =
                                          bir::MemoryRangeVerdict::ProvenOutOfBounds,
                                  },
                          },
                  },
              },
      });
  const std::string provenance_dump = prepare::print(provenance_prepared);
  if (!expect_contains(
          provenance_dump,
          "base=pointer_value result=%loaded pointer=%loaded offset=16 size=4 "
          "align=4 base_plus_offset=yes layout_authority=opaque_compatibility "
          "range_verdict=proven_out_of_bounds "
          "dynamic_array_verdict=unbounded",
          "prepared addressing provenance verdict dump")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
