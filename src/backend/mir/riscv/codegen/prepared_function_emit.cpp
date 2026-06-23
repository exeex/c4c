#include "prepared_function_emit.hpp"

#include "prepared_call_emit.hpp"
#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_global_memory_emit.hpp"
#include "prepared_local_memory_emit.hpp"
#include "prepared_scalar_emit.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>

namespace c4c::backend::riscv::codegen {
namespace {

std::optional<c4c::FunctionNameId> prepared_function_id_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function) {
  const auto function_name = names.function_names.find(function.name);
  if (function_name == c4c::kInvalidFunctionName) {
    return std::nullopt;
  }
  return function_name;
}

std::optional<c4c::BlockLabelId> prepared_block_label_id_for(
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const auto prepared_label = names.block_labels.find(structured_label);
      if (prepared_label != c4c::kInvalidBlockLabel) {
        return prepared_label;
      }
    }
  }
  const auto block_label = names.block_labels.find(block.label);
  if (block_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  return block_label;
}

}  // namespace

bool append_simple_prepared_bir_function_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration) {
    return true;
  }

  std::unordered_map<std::string, SimpleCompare> compares;
  const std::string function_name = function.name.empty() ? "anon" : function.name;
  std::size_t prepared_frame_size = 0;
  const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* control_flow = nullptr;
  const auto function_name_id = prepared_function_id_for(prepared.names, function);
  if (function_name_id.has_value()) {
    frame_plan = c4c::backend::prepare::find_prepared_frame_plan(
        prepared,
        *function_name_id);
    control_flow = c4c::backend::prepare::find_prepared_control_flow_function(
        prepared.control_flow,
        *function_name_id);
    if (frame_plan != nullptr) {
      prepared_frame_size = frame_plan->frame_size_bytes;
    }
  }
  bool saves_return_address = false;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<c4c::backend::bir::CallInst>(inst)) {
        saves_return_address = true;
        break;
      }
    }
    if (saves_return_address) {
      break;
    }
  }

  std::optional<std::size_t> return_address_stack_offset;
  std::size_t required_frame_size = prepared_frame_size;
  if (saves_return_address) {
    const auto saved_register_end = prepared_saved_register_stack_end(frame_plan);
    if (!saved_register_end.has_value()) {
      return false;
    }
    required_frame_size =
        std::max({required_frame_size, *saved_register_end, std::size_t{8}});
    return_address_stack_offset = align_riscv_stack_slot(required_frame_size, 8);
    if (*return_address_stack_offset >
        std::numeric_limits<std::size_t>::max() - std::size_t{8}) {
      return false;
    }
    if (!fits_signed_12_bit_load_offset(*return_address_stack_offset)) {
      return false;
    }
    required_frame_size = std::max(required_frame_size, *return_address_stack_offset + 8);
  }
  const std::size_t stack_frame_bytes = align_riscv_stack_frame(required_frame_size);
  if (stack_frame_bytes > 0 &&
      !fits_signed_12_bit_immediate(-static_cast<std::int64_t>(stack_frame_bytes))) {
    return false;
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    const auto block_label_id = prepared_block_label_id_for(
        prepared.module.names,
        prepared.names,
        block);
    if (!block_label_id.has_value()) {
      return false;
    }
    const std::string block_label = bir_block_label_spelling(prepared.module, block);
    out += riscv_local_block_label(function_name, block_label) + ":\n";
    if (block_index == 0 && stack_frame_bytes > 0) {
      out += "    addi sp, sp, -" + std::to_string(stack_frame_bytes) + "\n";
    }
    if (block_index == 0 && return_address_stack_offset.has_value()) {
      out += "    sd ra, " + std::to_string(*return_address_stack_offset) + "(sp)\n";
    }

    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      const auto& inst = block.insts[instruction_index];
      const PreparedCurrentInstructionContext context{
          .names = prepared.names,
          .lookups = lookups,
          .block_label = *block_label_id,
          .instruction_index = instruction_index,
      };
      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
      if (binary != nullptr) {
        if (binary->opcode == c4c::backend::bir::BinaryOpcode::Mul &&
            binary->result.kind == c4c::backend::bir::Value::Kind::Named &&
            instruction_index + 1 < block.insts.size()) {
          const auto* next_binary =
              std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[instruction_index + 1]);
          if (next_binary != nullptr &&
              next_binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
              next_binary->result.type == c4c::backend::bir::TypeKind::Ptr &&
              has_frame_slot_address_materialization_at(PreparedCurrentInstructionContext{
                  .names = prepared.names,
                  .lookups = lookups,
                  .block_label = *block_label_id,
                  .instruction_index = instruction_index + 1,
              }) &&
              ((next_binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                next_binary->lhs.name == binary->result.name) ||
               (next_binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                next_binary->rhs.name == binary->result.name))) {
            continue;
          }
        }
        if (c4c::backend::bir::is_compare_opcode(binary->opcode) &&
            binary->result.kind == c4c::backend::bir::Value::Kind::Named) {
          compares[binary->result.name] = SimpleCompare{
              .opcode = binary->opcode,
              .lhs = binary->lhs,
              .rhs = binary->rhs,
          };
        }
        auto emitted = emit_riscv_simple_prepared_pointer_add(
            *binary,
            context);
        if (!emitted.has_value()) {
          emitted = emit_riscv_simple_binary(*binary, prepared.names, lookups);
        }
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst);
      if (cast != nullptr) {
        const auto emitted = emit_riscv_simple_cast(*cast, prepared.names, lookups);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst);
      if (select != nullptr) {
        const auto emitted = emit_riscv_simple_select(
            *select,
            function_name,
            context);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call != nullptr) {
        const auto emitted = emit_riscv_simple_call(
            *call,
            block_index,
            context);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* store_local = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
      if (store_local != nullptr) {
        const auto emitted = emit_riscv_simple_store_local(
            prepared,
            *function_name_id,
            *store_local,
            context);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* load_local = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
      if (load_local != nullptr) {
        const auto emitted = emit_riscv_simple_load_local(
            prepared,
            *function_name_id,
            *load_local,
            context);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* store_global = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst);
      if (store_global != nullptr) {
        const auto emitted = emit_riscv_simple_store_global(
            prepared,
            *store_global,
            context);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* load_global = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
      if (load_global != nullptr) {
        const auto emitted = emit_riscv_simple_load_global(
            prepared,
            *load_global,
            context);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      return false;
    }

    switch (block.terminator.kind) {
      case c4c::backend::bir::TerminatorKind::Return: {
        const auto returned = emit_riscv_simple_return(
            block.terminator,
            prepared.names,
            lookups,
            return_address_stack_offset,
            stack_frame_bytes);
        if (!returned.has_value()) {
          return false;
        }
        out += *returned;
        break;
      }
      case c4c::backend::bir::TerminatorKind::Branch: {
        const std::string target = bir_target_label_spelling(
            prepared.module,
            block.terminator.target_label_id,
            block.terminator.target_label);
        out += "    j " + riscv_local_block_label(function_name, target) + "\n";
        break;
      }
      case c4c::backend::bir::TerminatorKind::CondBranch: {
        const std::string true_label = bir_target_label_spelling(
            prepared.module,
            block.terminator.true_label_id,
            block.terminator.true_label);
        const std::string false_label = bir_target_label_spelling(
            prepared.module,
            block.terminator.false_label_id,
            block.terminator.false_label);
        const std::string true_asm_label = riscv_local_block_label(function_name, true_label);
        const std::string false_asm_label = riscv_local_block_label(function_name, false_label);

        if (const auto condition_imm = simple_integer_immediate(block.terminator.condition)) {
          out += "    j ";
          out += *condition_imm != 0 ? true_asm_label : false_asm_label;
          out += "\n";
          break;
        }

        if (block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named) {
          return false;
        }
        if (control_flow != nullptr) {
          const auto* branch_condition =
              c4c::backend::prepare::find_prepared_branch_condition(
                  *control_flow,
                  *block_label_id);
          if (branch_condition != nullptr &&
              branch_condition->condition_value.kind ==
                  c4c::backend::bir::Value::Kind::Named &&
              branch_condition->condition_value.name == block.terminator.condition.name) {
            const auto prepared_targets =
                c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
                    prepared.names,
                    control_flow,
                    *block_label_id,
                    block,
                    *branch_condition);
            if (!prepared_targets.has_value()) {
              return false;
            }
            const std::string prepared_true_asm_label = riscv_local_block_label(
                function_name,
                c4c::backend::prepare::prepared_block_label(
                    prepared.names,
                    prepared_targets->true_label));
            const std::string prepared_false_asm_label = riscv_local_block_label(
                function_name,
                c4c::backend::prepare::prepared_block_label(
                    prepared.names,
                    prepared_targets->false_label));
            const auto prepared_branch = emit_riscv_prepared_fused_compare_branch(
                *branch_condition,
                prepared.names,
                lookups,
                prepared_true_asm_label,
                prepared_false_asm_label);
            if (prepared_branch.has_value()) {
              out += *prepared_branch;
              break;
            }
          }
        }
        const auto compare_it = compares.find(block.terminator.condition.name);
        if (compare_it == compares.end()) {
          return false;
        }
        const auto branch = emit_riscv_simple_compare_branch(
            compare_it->second,
            true_asm_label,
            false_asm_label);
        if (!branch.has_value()) {
          return false;
        }
        out += *branch;
        break;
      }
    }
  }
  return true;
}

}  // namespace c4c::backend::riscv::codegen
