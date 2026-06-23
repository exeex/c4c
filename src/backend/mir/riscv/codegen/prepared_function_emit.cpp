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

std::optional<std::string_view> riscv_gpr_argument_register(std::size_t index) {
  switch (index) {
    case 0: return std::string_view{"a0"};
    case 1: return std::string_view{"a1"};
    case 2: return std::string_view{"a2"};
    case 3: return std::string_view{"a3"};
    case 4: return std::string_view{"a4"};
    case 5: return std::string_view{"a5"};
    case 6: return std::string_view{"a6"};
    case 7: return std::string_view{"a7"};
    default: return std::nullopt;
  }
}

bool riscv_gpr_formal_type(c4c::backend::bir::TypeKind type) {
  namespace bir = c4c::backend::bir;
  switch (type) {
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return true;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return false;
  }
  return false;
}

std::optional<std::string> emit_riscv_gpr_formal_stack_store(
    c4c::backend::bir::TypeKind type,
    std::string_view source_register,
    std::size_t offset_bytes) {
  namespace bir = c4c::backend::bir;
  if (!fits_signed_12_bit_load_offset(offset_bytes) ||
      offset_bytes > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  const auto stack_offset = static_cast<std::int64_t>(offset_bytes);
  switch (type) {
    case bir::TypeKind::I8:
      return "    sb " + std::string(source_register) + ", " +
             std::to_string(stack_offset) + "(sp)\n";
    case bir::TypeKind::I16:
      return "    sh " + std::string(source_register) + ", " +
             std::to_string(stack_offset) + "(sp)\n";
    case bir::TypeKind::I32:
      return emit_i32_store_to_stack_offset(source_register, stack_offset);
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return "    sd " + std::string(source_register) + ", " +
             std::to_string(stack_offset) + "(sp)\n";
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

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

bool next_select_uses_value(
    const c4c::backend::bir::Block& block,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value) {
  if (instruction_index + 1U >= block.insts.size() ||
      value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  const auto* select =
      std::get_if<c4c::backend::bir::SelectInst>(&block.insts[instruction_index + 1U]);
  if (select == nullptr) {
    return false;
  }
  const auto names_match = [&](const c4c::backend::bir::Value& candidate) {
    return candidate.kind == c4c::backend::bir::Value::Kind::Named &&
           candidate.name == value.name;
  };
  return names_match(select->true_value) || names_match(select->false_value);
}

bool terminator_uses_value_as_condition(
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::Value& value) {
  return block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch &&
         value.kind == c4c::backend::bir::Value::Kind::Named &&
         block.terminator.condition.kind == c4c::backend::bir::Value::Kind::Named &&
         value.name == block.terminator.condition.name;
}

const c4c::backend::prepare::PreparedBranchCondition* find_branch_condition_for_terminator(
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::bir::Value& condition) {
  if (condition.kind != c4c::backend::bir::Value::Kind::Named ||
      condition.name.empty()) {
    return nullptr;
  }
  const auto* direct =
      c4c::backend::prepare::find_prepared_branch_condition(control_flow, block_label_id);
  if (direct != nullptr &&
      direct->condition_value.kind == c4c::backend::bir::Value::Kind::Named &&
      direct->condition_value.name == condition.name) {
    return direct;
  }

  const c4c::backend::prepare::PreparedBranchCondition* selected = nullptr;
  for (const auto& candidate : control_flow.branch_conditions) {
    if (candidate.condition_value.kind != c4c::backend::bir::Value::Kind::Named ||
        candidate.condition_value.name != condition.name) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &candidate;
  }
  return selected;
}

bool append_riscv_formal_entry_homes(
    std::string& out,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  for (std::size_t param_index = 0; param_index < function.params.size(); ++param_index) {
    const auto& param = function.params[param_index];
    if (!param.is_byval ||
        param.type != bir::TypeKind::Ptr ||
        param.name.empty()) {
      if (param.is_byval || param.name.empty() ||
          !riscv_gpr_formal_type(param.type) ||
          (param.abi.has_value() && !param.abi->passed_in_register)) {
        continue;
      }
      const auto source_register = riscv_gpr_argument_register(param_index);
      if (!source_register.has_value()) {
        return false;
      }
      const bir::Value value{
          .kind = bir::Value::Kind::Named,
          .type = param.type,
          .name = param.name,
      };
      const auto* home = prepared_value_home_for(names, lookups, value);
      if (home == nullptr) {
        return false;
      }
      if (home->kind == prepare::PreparedValueHomeKind::Register) {
        if (!home->register_name.has_value()) {
          return false;
        }
        if (*home->register_name != *source_register) {
          out += "    mv ";
          out += *home->register_name;
          out += ", ";
          out += *source_register;
          out += "\n";
        }
        continue;
      }
      if (home->kind == prepare::PreparedValueHomeKind::StackSlot) {
        if (!home->offset_bytes.has_value()) {
          return false;
        }
        const auto stored = emit_riscv_gpr_formal_stack_store(
            param.type,
            *source_register,
            *home->offset_bytes);
        if (!stored.has_value()) {
          return false;
        }
        out += *stored;
        continue;
      }
      return false;
    }
    const auto source_register = riscv_gpr_argument_register(param_index);
    if (!source_register.has_value()) {
      return false;
    }
    const bir::Value value{
        .kind = bir::Value::Kind::Named,
        .type = bir::TypeKind::Ptr,
        .name = param.name,
    };
    const auto* home = prepared_value_home_for(names, lookups, value);
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !home->offset_bytes.has_value() ||
        !home->size_bytes.has_value() ||
        *home->size_bytes < 8 ||
        !fits_signed_12_bit_load_offset(*home->offset_bytes)) {
      return false;
    }
    const auto stored = emit_riscv_gpr_formal_stack_store(
        bir::TypeKind::Ptr,
        *source_register,
        *home->offset_bytes);
    if (!stored.has_value()) {
      return false;
    }
    out += *stored;
  }
  return true;
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
    if (block_index == 0 &&
        !append_riscv_formal_entry_homes(
            out,
            prepared.names,
            lookups,
            function)) {
      return false;
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
          if (next_select_uses_value(block, instruction_index, binary->result)) {
            continue;
          }
          if (terminator_uses_value_as_condition(block, binary->result)) {
            continue;
          }
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

      if (std::holds_alternative<c4c::backend::bir::PhiInst>(inst)) {
        continue;
      }

      const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst);
      if (select != nullptr) {
        const auto emitted = emit_riscv_simple_select(
            *select,
            prepared,
            function_name,
            context,
            &block);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call != nullptr) {
        const auto emitted = emit_riscv_simple_call(
            prepared,
            *function_name_id,
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
            prepared,
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
              find_branch_condition_for_terminator(
                  *control_flow,
                  *block_label_id,
                  block.terminator.condition);
          if (branch_condition != nullptr) {
            std::string prepared_true_asm_label = true_asm_label;
            std::string prepared_false_asm_label = false_asm_label;
            const auto prepared_targets =
                c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
                    prepared.names,
                    control_flow,
                    *block_label_id,
                    block,
                    *branch_condition);
            if (prepared_targets.has_value()) {
              prepared_true_asm_label = riscv_local_block_label(
                  function_name,
                  c4c::backend::prepare::prepared_block_label(
                      prepared.names,
                      prepared_targets->true_label));
              prepared_false_asm_label = riscv_local_block_label(
                  function_name,
                  c4c::backend::prepare::prepared_block_label(
                      prepared.names,
                      prepared_targets->false_label));
            }
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
        c4c::backend::prepare::PreparedBranchCondition synthetic_branch_condition;
        synthetic_branch_condition.kind =
            c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare;
        synthetic_branch_condition.predicate = compare_it->second.opcode;
        synthetic_branch_condition.lhs = compare_it->second.lhs;
        synthetic_branch_condition.rhs = compare_it->second.rhs;
        const auto register_branch = emit_riscv_prepared_fused_compare_branch(
            synthetic_branch_condition,
            prepared.names,
            lookups,
            true_asm_label,
            false_asm_label);
        if (register_branch.has_value()) {
          out += *register_branch;
          break;
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
