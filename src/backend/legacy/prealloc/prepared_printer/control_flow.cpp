#include "private.hpp"

#include "../../bir/bir.hpp"

#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {

namespace {

std::string render_value(const c4c::backend::bir::Value& value) {
  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
    return value.name;
  }
  if (value.type == c4c::backend::bir::TypeKind::F128 && value.f128_payload.has_value()) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(16) << value.f128_payload->high_bits
        << std::setw(16) << value.f128_payload->low_bits;
    return out.str();
  }
  if (value.type == c4c::backend::bir::TypeKind::F32 ||
      value.type == c4c::backend::bir::TypeKind::F64) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setfill('0');
    if (value.type == c4c::backend::bir::TypeKind::F32) {
      out << std::setw(8) << static_cast<std::uint32_t>(value.immediate_bits);
    } else {
      out << std::setw(16) << value.immediate_bits;
    }
    return out.str();
  }
  return std::to_string(value.immediate);
}

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

std::string terminator_kind_name(c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "return";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "branch";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "cond_branch";
  }
  return "unknown";
}

std::string prepared_join_transfer_ownership_name(const PreparedJoinTransfer& transfer) {
  if (transfer.source_branch_block_label.has_value()) {
    return "authoritative_branch_pair";
  }
  if (!transfer.edge_transfers.empty()) {
    return "per_edge";
  }
  return "none";
}

std::string prepared_parallel_copy_resolution_name(const PreparedParallelCopyBundle& bundle) {
  return bundle.has_cycle ? "cycle_break" : "acyclic";
}

void append_parallel_copy_step_detail(std::ostringstream& out,
                                      const PreparedNameTables& names,
                                      const PreparedParallelCopyBundle& bundle,
                                      const PreparedParallelCopyStep& step) {
  const auto* move = find_prepared_parallel_copy_move_for_step(bundle, step);
  if (move == nullptr) {
    out << " invalid_move_index";
    return;
  }

  if (step.kind == PreparedParallelCopyStepKind::SaveDestinationToTemp) {
    out << " save_destination=" << render_value(move->destination_value)
        << " blocked_source=" << render_value(move->source_value)
        << " temp_source=cycle_temp(" << render_value(move->destination_value) << ")"
        << " carrier=" << prepared_join_transfer_carrier_kind_name(move->carrier_kind);
    if (move->storage_name.has_value()) {
      out << " storage=" << prepared_slot_name(names, *move->storage_name);
    }
    return;
  }

  out << " source=";
  if (step.uses_cycle_temp_source) {
    out << "cycle_temp(" << render_value(move->source_value) << ")";
  } else {
    out << render_value(move->source_value);
  }
  out << " destination=" << render_value(move->destination_value)
      << " carrier=" << prepared_join_transfer_carrier_kind_name(move->carrier_kind);
  if (move->storage_name.has_value()) {
    out << " storage=" << prepared_slot_name(names, *move->storage_name);
  }
}

}  // namespace

void append_prepared_control_flow(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-control-flow ---\n";
  for (const auto& function_cf : module.control_flow.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_cf.function_name) << "\n";

    for (const auto& block : function_cf.blocks) {
      out << "  block " << maybe_block_label(module.names, block.block_label)
          << " terminator=" << terminator_kind_name(block.terminator_kind);
      if (block.branch_target_label != kInvalidBlockLabel) {
        out << " target=" << maybe_block_label(module.names, block.branch_target_label);
      }
      if (block.true_label != kInvalidBlockLabel || block.false_label != kInvalidBlockLabel) {
        out << " true=" << maybe_block_label(module.names, block.true_label)
            << " false=" << maybe_block_label(module.names, block.false_label);
      }
      out << "\n";
    }

    for (const auto& condition : function_cf.branch_conditions) {
      out << "  branch_condition " << maybe_block_label(module.names, condition.block_label)
          << " kind=" << prepared_branch_condition_kind_name(condition.kind)
          << " condition=" << render_value(condition.condition_value);
      if (condition.predicate.has_value()) {
        out << " compare=" << c4c::backend::bir::render_binary_opcode(*condition.predicate)
            << " " << c4c::backend::bir::render_type(*condition.compare_type)
            << " " << render_value(*condition.lhs)
            << ", " << render_value(*condition.rhs);
      }
      out << " can_fuse_with_branch=" << (condition.can_fuse_with_branch ? "yes" : "no")
          << " true=" << maybe_block_label(module.names, condition.true_label)
          << " false=" << maybe_block_label(module.names, condition.false_label)
          << "\n";
    }

    for (const auto& transfer : function_cf.join_transfers) {
      out << "  join_transfer " << maybe_block_label(module.names, transfer.join_block_label)
          << " result=" << render_value(transfer.result)
          << " kind=" << prepared_join_transfer_kind_name(transfer.kind)
          << " carrier="
          << prepared_join_transfer_carrier_kind_name(
                 effective_prepared_join_transfer_carrier_kind(transfer))
          << " ownership=" << prepared_join_transfer_ownership_name(transfer)
          << " incomings=" << transfer.incomings.size()
          << " edge_transfers=" << transfer.edge_transfers.size();
      if (transfer.storage_name.has_value()) {
        out << " storage=" << prepared_slot_name(module.names, *transfer.storage_name);
      }
      if (transfer.source_branch_block_label.has_value()) {
        out << " source_branch="
            << maybe_block_label(module.names, *transfer.source_branch_block_label);
      }
      if (transfer.source_true_incoming_label.has_value() ||
          transfer.source_false_incoming_label.has_value()) {
        out << " source_incomings=("
            << maybe_block_label(module.names,
                                 transfer.source_true_incoming_label.value_or(kInvalidBlockLabel))
            << ", "
            << maybe_block_label(module.names,
                                 transfer.source_false_incoming_label.value_or(kInvalidBlockLabel))
            << ")";
      }
      if (transfer.source_true_transfer_index.has_value() ||
          transfer.source_false_transfer_index.has_value()) {
        out << " source_transfer_indexes=("
            << (transfer.source_true_transfer_index.has_value()
                    ? std::to_string(*transfer.source_true_transfer_index)
                    : std::string("<none>"))
            << ", "
            << (transfer.source_false_transfer_index.has_value()
                    ? std::to_string(*transfer.source_false_transfer_index)
                    : std::string("<none>"))
            << ")";
      }
      const auto continuation_targets =
          published_prepared_compare_join_continuation_targets(transfer);
      if (continuation_targets.has_value()) {
        out << " continuation_targets=("
            << maybe_block_label(module.names, continuation_targets->true_label)
            << ", "
            << maybe_block_label(module.names, continuation_targets->false_label)
            << ")";
      }
      out << "\n";
      for (const auto& incoming : transfer.incomings) {
        out << "    incoming [" << incoming.label << "] -> " << render_value(incoming.value)
            << "\n";
      }
      for (const auto& edge : transfer.edge_transfers) {
        out << "    edge_transfer "
            << maybe_block_label(module.names, edge.predecessor_label)
            << " -> " << maybe_block_label(module.names, edge.successor_label)
            << " incoming=" << render_value(edge.incoming_value)
            << " destination=" << render_value(edge.destination_value);
        if (edge.storage_name.has_value()) {
          out << " storage=" << prepared_slot_name(module.names, *edge.storage_name);
        }
        out << "\n";
      }
    }

    for (const auto& bundle : function_cf.parallel_copy_bundles) {
      const auto execution_block =
          published_prepared_parallel_copy_execution_block_label(bundle);
      out << "  parallel_copy "
          << maybe_block_label(module.names, bundle.predecessor_label)
          << " -> " << maybe_block_label(module.names, bundle.successor_label)
          << " execution_site="
          << prepared_parallel_copy_execution_site_name(bundle.execution_site)
          << " execution_block="
          << maybe_block_label(module.names, execution_block.value_or(kInvalidBlockLabel))
          << " has_cycle=" << (bundle.has_cycle ? "yes" : "no")
          << " resolution=" << prepared_parallel_copy_resolution_name(bundle)
          << " moves=" << bundle.moves.size()
          << " steps=" << bundle.steps.size() << "\n";
      out << "    authority execution_site="
          << prepared_parallel_copy_execution_site_name(bundle.execution_site)
          << " execution_block="
          << maybe_block_label(module.names, execution_block.value_or(kInvalidBlockLabel))
          << "\n";
      for (std::size_t move_index = 0; move_index < bundle.moves.size(); ++move_index) {
        const auto& move = bundle.moves[move_index];
        out << "    move[" << move_index << "] "
            << render_value(move.source_value) << " -> " << render_value(move.destination_value)
            << " join_transfer_index=" << move.join_transfer_index
            << " edge_transfer_index=" << move.edge_transfer_index
            << " carrier="
            << prepared_join_transfer_carrier_kind_name(move.carrier_kind);
        if (move.storage_name.has_value()) {
          out << " storage=" << prepared_slot_name(module.names, *move.storage_name);
        }
        out << "\n";
      }
      for (std::size_t step_index = 0; step_index < bundle.steps.size(); ++step_index) {
        const auto& step = bundle.steps[step_index];
        out << "    step[" << step_index << "] "
            << prepared_parallel_copy_step_kind_name(step.kind)
            << " move_index=" << step.move_index;
        append_parallel_copy_step_detail(out, module.names, bundle, step);
        out << " uses_cycle_temp_source="
            << (step.uses_cycle_temp_source ? "yes" : "no") << "\n";
      }
    }
  }
}

}  // namespace c4c::backend::prepare
