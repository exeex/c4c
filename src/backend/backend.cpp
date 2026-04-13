#include "backend.hpp"

#include "bir_printer.hpp"

#include "../codegen/lir/lir_printer.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace c4c::backend {

namespace {

using c4c::backend::bir::BinaryInst;
using c4c::backend::bir::BinaryOpcode;
using c4c::backend::bir::CallInst;
using c4c::backend::bir::Function;
using c4c::backend::bir::LoadLocalInst;
using c4c::backend::bir::Module;
using c4c::backend::bir::StoreLocalInst;
using c4c::backend::bir::TerminatorKind;
using c4c::backend::bir::TypeKind;
using c4c::backend::bir::Value;

constexpr std::array<std::string_view, 7> kRiscvTempRegs = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6"};
constexpr std::array<std::string_view, 8> kRiscvIncomingArgRegs = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
constexpr std::array<std::string_view, 6> kRiscvArgRegs = {"a0", "a1", "a2", "a3", "a4", "a5"};

Target resolve_public_lir_target(const c4c::codegen::lir::LirModule& module,
                                 Target public_target) {
  if (public_target != Target::X86_64) {
    return public_target;
  }

  auto target = target_from_triple(module.target_triple);
  if (target != Target::I686) {
    target = Target::X86_64;
  }
  return target;
}

std::string emit_bootstrap_lir_module(const c4c::codegen::lir::LirModule& module,
                                      Target target) {
  (void)target;
  return c4c::codegen::lir::print_llvm(module);
}

std::int64_t align_up(std::int64_t value, std::int64_t align) {
  return ((value + align - 1) / align) * align;
}

bool is_supported_riscv_i32_value(const Value& value) {
  return value.type == TypeKind::I32;
}

bool is_supported_riscv_ptr_value(const Value& value) {
  return value.type == TypeKind::Ptr;
}

std::int64_t riscv_stack_slot_size(TypeKind type) {
  switch (type) {
    case TypeKind::I32:
      return 4;
    case TypeKind::Ptr:
      return 8;
    default:
      return 0;
  }
}

std::int64_t riscv_stack_slot_align(TypeKind type) {
  switch (type) {
    case TypeKind::I32:
      return 4;
    case TypeKind::Ptr:
      return 8;
    default:
      return 0;
  }
}

bool function_uses_calls(const Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<CallInst>(inst)) {
        return true;
      }
    }
  }
  return false;
}

struct RiscvFrameLayout {
  std::int64_t frame_size = 0;
  std::optional<std::int64_t> ra_offset;
  std::unordered_map<std::string, std::int64_t> slot_offsets;
};

std::optional<RiscvFrameLayout> compute_riscv_frame_layout(const Function& function) {
  RiscvFrameLayout layout;
  std::int64_t next_offset = 0;

  auto reserve_slot = [&](std::string_view name, TypeKind type) {
    if (layout.slot_offsets.find(std::string(name)) != layout.slot_offsets.end()) {
      return true;
    }
    const auto slot_size = riscv_stack_slot_size(type);
    const auto slot_align = riscv_stack_slot_align(type);
    if (slot_size == 0 || slot_align == 0) {
      return false;
    }
    next_offset = align_up(next_offset, slot_align);
    layout.slot_offsets.emplace(std::string(name), next_offset);
    next_offset += slot_size;
    return true;
  };

  for (const auto& slot : function.local_slots) {
    if (slot.is_address_taken || slot.is_byval_copy ||
        (slot.type != TypeKind::I32 && slot.type != TypeKind::Ptr)) {
      return std::nullopt;
    }
    if (!reserve_slot(slot.name, slot.type)) {
      return std::nullopt;
    }
  }

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<LoadLocalInst>(&inst)) {
        if (load->byte_offset != 0 || load->align_bytes != 0 || load->address.has_value() ||
            (load->result.type != TypeKind::I32 && load->result.type != TypeKind::Ptr)) {
          return std::nullopt;
        }
        if (!reserve_slot(load->slot_name, load->result.type)) {
          return std::nullopt;
        }
      } else if (const auto* store = std::get_if<StoreLocalInst>(&inst)) {
        if (store->byte_offset != 0 || store->align_bytes != 0 || store->address.has_value() ||
            (!is_supported_riscv_i32_value(store->value) &&
             !is_supported_riscv_ptr_value(store->value))) {
          return std::nullopt;
        }
        if (!reserve_slot(store->slot_name, store->value.type)) {
          return std::nullopt;
        }
      }
    }
  }

  if (function_uses_calls(function)) {
    next_offset = align_up(next_offset, 8);
    layout.ra_offset = next_offset;
    next_offset += 8;
  }

  layout.frame_size = align_up(next_offset, 16);
  return layout;
}

struct RiscvEmitState {
  std::unordered_map<std::string, std::string> value_regs;
  std::size_t next_temp_index = 0;
};

std::optional<std::string> allocate_riscv_temp(RiscvEmitState& state) {
  if (state.next_temp_index >= kRiscvTempRegs.size()) {
    return std::nullopt;
  }
  return std::string(kRiscvTempRegs[state.next_temp_index++]);
}

std::optional<std::string> materialize_riscv_i32_value(const Value& value,
                                                       RiscvEmitState& state,
                                                       std::ostringstream& out) {
  if (!is_supported_riscv_i32_value(value)) {
    return std::nullopt;
  }
  if (value.kind == Value::Kind::Immediate) {
    auto reg = allocate_riscv_temp(state);
    if (!reg.has_value()) {
      return std::nullopt;
    }
    out << "    li " << *reg << ", " << value.immediate << "\n";
    return reg;
  }

  const auto it = state.value_regs.find(value.name);
  if (it == state.value_regs.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::string> materialize_riscv_ptr_value(const Value& value,
                                                       RiscvEmitState& state,
                                                       std::ostringstream& out) {
  (void)out;
  if (!is_supported_riscv_ptr_value(value) || value.kind != Value::Kind::Named) {
    return std::nullopt;
  }

  const auto it = state.value_regs.find(value.name);
  if (it == state.value_regs.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool move_riscv_i32_value_into_reg(const Value& value,
                                   std::string_view dest_reg,
                                   RiscvEmitState& state,
                                   std::ostringstream& out) {
  if (!is_supported_riscv_i32_value(value)) {
    return false;
  }
  if (value.kind == Value::Kind::Immediate) {
    out << "    li " << dest_reg << ", " << value.immediate << "\n";
    return true;
  }

  const auto it = state.value_regs.find(value.name);
  if (it == state.value_regs.end()) {
    return false;
  }
  if (it->second != dest_reg) {
    out << "    mv " << dest_reg << ", " << it->second << "\n";
  }
  return true;
}

bool move_riscv_i32_call_args_into_regs(const std::vector<Value>& args,
                                        RiscvEmitState& state,
                                        std::ostringstream& out) {
  if (args.size() > kRiscvArgRegs.size()) {
    return false;
  }

  struct PendingCallArgMove {
    std::string_view dest_reg;
    bool is_immediate = false;
    std::int64_t immediate = 0;
    std::string src_reg;
  };

  std::vector<PendingCallArgMove> moves;
  moves.reserve(args.size());
  for (std::size_t index = 0; index < args.size(); ++index) {
    PendingCallArgMove move{
        .dest_reg = kRiscvArgRegs[index],
    };
    if (!is_supported_riscv_i32_value(args[index])) {
      return false;
    }
    if (args[index].kind == Value::Kind::Immediate) {
      move.is_immediate = true;
      move.immediate = args[index].immediate;
      moves.push_back(std::move(move));
      continue;
    }

    const auto it = state.value_regs.find(args[index].name);
    if (it == state.value_regs.end()) {
      return false;
    }
    move.src_reg = it->second;
    if (move.src_reg == move.dest_reg) {
      continue;
    }
    moves.push_back(std::move(move));
  }

  while (!moves.empty()) {
    bool emitted = false;
    for (std::size_t index = 0; index < moves.size(); ++index) {
      if (!moves[index].is_immediate) {
        const bool src_is_pending_dest =
            std::any_of(moves.begin(), moves.end(), [&](const PendingCallArgMove& other) {
              return other.dest_reg == moves[index].src_reg;
            });
        if (src_is_pending_dest) {
          continue;
        }
      }

      if (moves[index].is_immediate) {
        out << "    li " << moves[index].dest_reg << ", " << moves[index].immediate << "\n";
      } else {
        out << "    mv " << moves[index].dest_reg << ", " << moves[index].src_reg << "\n";
      }
      moves.erase(moves.begin() + static_cast<std::ptrdiff_t>(index));
      emitted = true;
      break;
    }
    if (emitted) {
      continue;
    }

    auto temp = allocate_riscv_temp(state);
    if (!temp.has_value()) {
      return false;
    }
    const auto cycle_src = moves.front().src_reg;
    out << "    mv " << *temp << ", " << cycle_src << "\n";
    for (auto& move : moves) {
      if (!move.is_immediate && move.src_reg == cycle_src) {
        move.src_reg = *temp;
      }
    }
  }
  return true;
}

bool lower_riscv_binary_inst(const BinaryInst& inst,
                             RiscvEmitState& state,
                             std::ostringstream& out) {
  if (inst.opcode != BinaryOpcode::Add || inst.result.type != TypeKind::I32 ||
      (inst.operand_type != TypeKind::Void && inst.operand_type != TypeKind::I32) ||
      !is_supported_riscv_i32_value(inst.lhs) || !is_supported_riscv_i32_value(inst.rhs)) {
    return false;
  }

  std::string result_reg;
  if (inst.lhs.kind == Value::Kind::Named) {
    const auto lhs_it = state.value_regs.find(inst.lhs.name);
    if (lhs_it == state.value_regs.end()) {
      return false;
    }
    result_reg = lhs_it->second;
  } else {
    auto reg = allocate_riscv_temp(state);
    if (!reg.has_value()) {
      return false;
    }
    result_reg = *reg;
    out << "    li " << result_reg << ", " << inst.lhs.immediate << "\n";
  }

  if (inst.rhs.kind == Value::Kind::Immediate) {
    out << "    addi " << result_reg << ", " << result_reg << ", " << inst.rhs.immediate
        << "\n";
  } else {
    const auto rhs_it = state.value_regs.find(inst.rhs.name);
    if (rhs_it == state.value_regs.end()) {
      return false;
    }
    out << "    add " << result_reg << ", " << result_reg << ", " << rhs_it->second << "\n";
  }
  state.value_regs[inst.result.name] = result_reg;
  return true;
}

bool lower_riscv_call_inst(const CallInst& inst,
                           RiscvEmitState& state,
                           std::ostringstream& out) {
  if (inst.is_variadic || inst.calling_convention != c4c::backend::bir::CallingConv::C ||
      inst.args.size() > kRiscvArgRegs.size() || inst.arg_types.size() != inst.args.size()) {
    return false;
  }
  if (inst.return_type != TypeKind::Void && inst.return_type != TypeKind::I32) {
    return false;
  }
  for (std::size_t index = 0; index < inst.args.size(); ++index) {
    if (inst.arg_types[index] != TypeKind::I32) {
      return false;
    }
  }
  std::optional<std::string> indirect_callee_reg;
  if (inst.is_indirect) {
    if (!inst.callee_value.has_value()) {
      return false;
    }
    const auto callee_reg = materialize_riscv_ptr_value(*inst.callee_value, state, out);
    if (!callee_reg.has_value()) {
      return false;
    }
    const bool callee_in_arg_reg = std::find(kRiscvArgRegs.begin(),
                                             kRiscvArgRegs.begin() + inst.args.size(),
                                             std::string_view(*callee_reg)) !=
                                   kRiscvArgRegs.begin() + inst.args.size();
    if (callee_in_arg_reg) {
      indirect_callee_reg = allocate_riscv_temp(state);
      if (!indirect_callee_reg.has_value()) {
        return false;
      }
      out << "    mv " << *indirect_callee_reg << ", " << *callee_reg << "\n";
    } else {
      indirect_callee_reg = *callee_reg;
    }
  }
  if (!move_riscv_i32_call_args_into_regs(inst.args, state, out)) {
    return false;
  }

  if (inst.is_indirect) {
    out << "    jalr ra, " << *indirect_callee_reg << ", 0\n";
  } else {
    out << "    call " << inst.callee << "\n";
  }
  state.value_regs.clear();
  state.next_temp_index = 0;
  if (inst.result.has_value()) {
    if (inst.result->type != TypeKind::I32) {
      return false;
    }
    state.value_regs[inst.result->name] = "a0";
  }
  return true;
}

bool lower_riscv_function_body(const Function& function,
                               const RiscvFrameLayout& layout,
                               std::ostringstream& out) {
  if (function.is_declaration || function.blocks.size() != 1 ||
      function.blocks.front().label != "entry") {
    return false;
  }
  const auto& block = function.blocks.front();
  if (block.terminator.kind != TerminatorKind::Return) {
    return false;
  }

  RiscvEmitState state;
  if (function.params.size() > kRiscvIncomingArgRegs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    if (function.params[index].type != TypeKind::I32 &&
        function.params[index].type != TypeKind::Ptr) {
      return false;
    }
    state.value_regs[function.params[index].name] = std::string(kRiscvIncomingArgRegs[index]);
  }

  if (layout.frame_size > 0) {
    out << "    addi sp, sp, -" << layout.frame_size << "\n";
    if (layout.ra_offset.has_value()) {
      out << "    sd ra, " << *layout.ra_offset << "(sp)\n";
    }
  }

  for (const auto& inst : block.insts) {
    if (const auto* binary = std::get_if<BinaryInst>(&inst)) {
      if (!lower_riscv_binary_inst(*binary, state, out)) {
        return false;
      }
    } else if (const auto* store = std::get_if<StoreLocalInst>(&inst)) {
      const auto slot_it = layout.slot_offsets.find(store->slot_name);
      if (slot_it == layout.slot_offsets.end()) {
        return false;
      }
      std::optional<std::string> src_reg;
      if (store->value.type == TypeKind::I32) {
        src_reg = materialize_riscv_i32_value(store->value, state, out);
      } else if (store->value.type == TypeKind::Ptr) {
        src_reg = materialize_riscv_ptr_value(store->value, state, out);
      } else {
        return false;
      }
      if (!src_reg.has_value()) {
        return false;
      }
      if (store->value.type == TypeKind::I32) {
        out << "    sw " << *src_reg << ", " << slot_it->second << "(sp)\n";
      } else {
        out << "    sd " << *src_reg << ", " << slot_it->second << "(sp)\n";
      }
    } else if (const auto* load = std::get_if<LoadLocalInst>(&inst)) {
      const auto slot_it = layout.slot_offsets.find(load->slot_name);
      if (slot_it == layout.slot_offsets.end()) {
        return false;
      }
      auto result_reg = allocate_riscv_temp(state);
      if (!result_reg.has_value()) {
        return false;
      }
      if (load->result.type == TypeKind::I32) {
        out << "    lw " << *result_reg << ", " << slot_it->second << "(sp)\n";
      } else if (load->result.type == TypeKind::Ptr) {
        out << "    ld " << *result_reg << ", " << slot_it->second << "(sp)\n";
      } else {
        return false;
      }
      state.value_regs[load->result.name] = *result_reg;
    } else if (const auto* call = std::get_if<CallInst>(&inst)) {
      if (!lower_riscv_call_inst(*call, state, out)) {
        return false;
      }
    } else {
      return false;
    }
  }

  if (block.terminator.value.has_value() &&
      !move_riscv_i32_value_into_reg(*block.terminator.value, "a0", state, out)) {
    return false;
  }
  if (layout.ra_offset.has_value()) {
    out << "    ld ra, " << *layout.ra_offset << "(sp)\n";
  }
  if (layout.frame_size > 0) {
    out << "    addi sp, sp, " << layout.frame_size << "\n";
  }
  out << "    ret\n";
  return true;
}

std::optional<std::string> try_emit_supported_riscv_prepared_bir(const Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty()) {
    return std::nullopt;
  }

  std::ostringstream out;
  out << "    .text\n";
  for (const auto& function : module.functions) {
    if (function.return_type != TypeKind::Void && function.return_type != TypeKind::I32) {
      return std::nullopt;
    }
    if (function.is_variadic) {
      return std::nullopt;
    }
    if (function.is_declaration) {
      continue;
    }

    const auto layout = compute_riscv_frame_layout(function);
    if (!layout.has_value()) {
      return std::nullopt;
    }

    out << "    .globl " << function.name << "\n";
    out << function.name << ":\n";
    if (!lower_riscv_function_body(function, *layout, out)) {
      return std::nullopt;
    }
  }
  return out.str();
}

std::string emit_prepared_bir_or_fallback(const c4c::backend::bir::Module& module,
                                          Target target) {
  if (target == Target::Riscv64) {
    if (const auto emitted = try_emit_supported_riscv_prepared_bir(module);
        emitted.has_value()) {
      return *emitted;
    }
  }
  return c4c::backend::bir::print(module);
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : module_(bir_module) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : module_(std::cref(lir_module)) {}

c4c::codegen::lir::LirModule prepare_lir_module_for_target(
    const c4c::codegen::lir::LirModule& module,
    Target target) {
  return c4c::backend::prepare::prepare_lir_module_with_options(module, target).module;
}

c4c::backend::bir::Module prepare_bir_module_for_target(
    const c4c::backend::bir::Module& module,
    Target target) {
  return c4c::backend::prepare::prepare_bir_module_with_options(module, target).module;
}

std::string emit_target_bir_module(const bir::Module& module, Target public_target) {
  (void)public_target;
  const auto prepared =
      c4c::backend::prepare::prepare_bir_module_with_options(module, public_target);
  return emit_prepared_bir_or_fallback(prepared.module, public_target);
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   Target public_target) {
  const auto target = resolve_public_lir_target(module, public_target);
  return emit_module(BackendModuleInput{module}, BackendOptions{.target = target});
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    Target public_target,
    const std::string& output_path) {
  const auto emitted = emit_target_lir_module(module, public_target);
  return BackendAssembleResult{
      .staged_text = emitted,
      .output_path = output_path,
      .object_emitted = false,
      .error = "backend bootstrap mode does not assemble objects yet",
  };
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    return emit_target_bir_module(input.bir_module(), options.target);
  }

  const auto& lir_module = input.lir_module();
  const auto target = resolve_public_lir_target(lir_module, options.target);

  auto lowering = c4c::backend::try_lower_to_bir_with_options(
      lir_module, c4c::backend::BirLoweringOptions{});
  auto prepared = c4c::backend::prepare::prepare_lir_module_with_options(
      lir_module, target, c4c::backend::prepare::PrepareOptions{});

  if (lowering.module.has_value()) {
    const auto prepared_bir = c4c::backend::prepare::prepare_bir_module_with_options(
        *lowering.module, target, c4c::backend::prepare::PrepareOptions{});
    return emit_prepared_bir_or_fallback(prepared_bir.module, target);
  }
  return emit_bootstrap_lir_module(prepared.module, target);
}

}  // namespace c4c::backend
