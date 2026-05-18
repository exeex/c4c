#include "machine_printer.hpp"
#include "alu.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "f128.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "memory.hpp"
#include "returns.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace {

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

std::string block_label(c4c::FunctionNameId function_name, c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
}

std::string register_name(const RegisterOperand& operand) {
  return abi::register_name(operand.reg);
}

std::optional<unsigned> integer_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> register_name_with_view(const RegisterOperand& operand,
                                                   abi::RegisterView view) {
  if (!abi::is_gp_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

std::optional<std::string> fp_register_name_with_view(const RegisterOperand& operand,
                                                      abi::RegisterView view) {
  if (!abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

std::optional<abi::RegisterView> integer_register_view(unsigned bit_width) {
  if (bit_width <= 32U) {
    return abi::RegisterView::W;
  }
  if (bit_width == 64U) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

std::optional<abi::RegisterReference> immediate_store_scratch_register(
    const MemoryInstructionRecord& memory) {
  const auto scratch = abi::reserved_mir_scratch_gp_registers().front();
  if (memory.address.size_bytes == 4) {
    return abi::w_register(scratch.index);
  }
  if (memory.address.size_bytes == 8) {
    return abi::x_register(scratch.index);
  }
  return std::nullopt;
}

std::vector<std::string> materialize_integer_constant_lines(
    abi::RegisterReference destination,
    std::uint64_t value,
    unsigned width_bits) {
  std::vector<std::string> lines;
  const std::string destination_name = abi::register_name(destination);
  const unsigned chunks = width_bits == 64U ? 4U : 2U;

  bool emitted_base = false;
  for (unsigned chunk = 0; chunk < chunks; ++chunk) {
    const auto halfword =
        static_cast<unsigned>((value >> (chunk * 16U)) & 0xffffU);
    if (halfword == 0U) {
      continue;
    }

    std::ostringstream line;
    line << (emitted_base ? "movk " : "movz ") << destination_name
         << ", #" << halfword;
    if (chunk != 0U) {
      line << ", lsl #" << (chunk * 16U);
    }
    lines.push_back(line.str());
    emitted_base = true;
  }

  if (!emitted_base) {
    lines.push_back("movz " + destination_name + ", #0");
  }
  return lines;
}

std::vector<std::string> materialize_immediate_store_value_lines(
    abi::RegisterReference scratch,
    const ImmediateOperand& immediate,
    const MemoryInstructionRecord& memory) {
  if (memory.address.size_bytes == 4) {
    const auto value = static_cast<std::uint32_t>(immediate.signed_value);
    return materialize_integer_constant_lines(scratch, value, 32);
  }
  if (memory.address.size_bytes == 8) {
    const auto value = static_cast<std::uint64_t>(immediate.signed_value);
    return materialize_integer_constant_lines(scratch, value, 64);
  }
  return {};
}

std::optional<abi::RegisterView> floating_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::string atomic_loop_label(const AtomicMemoryInstructionRecord& atomic,
                              std::string_view suffix) {
  std::ostringstream out;
  out << ".Latomic_" << atomic.function_name << "_" << atomic.block_label << "_"
      << atomic.instruction_index << "_" << suffix;
  return out.str();
}

std::optional<std::string> atomic_pointer_register_name(
    const AtomicMemoryInstructionRecord& atomic) {
  if (!atomic.pointer_register.has_value()) {
    return std::nullopt;
  }
  return register_name_with_view(*atomic.pointer_register, abi::RegisterView::X);
}

std::optional<std::string> atomic_value_register_name(
    const RegisterOperand& operand,
    std::size_t width_bytes) {
  if (width_bytes == 0 || width_bytes > 8) {
    return std::nullopt;
  }
  return register_name_with_view(operand,
                                 width_bytes == 8 ? abi::RegisterView::X
                                                  : abi::RegisterView::W);
}

std::string_view atomic_plain_load_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 2:
      return "ldrh";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

std::string_view atomic_acquire_load_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldarb";
    case 2:
      return "ldarh";
    case 4:
    case 8:
      return "ldar";
  }
  return {};
}

std::string_view atomic_plain_store_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "strb";
    case 2:
      return "strh";
    case 4:
    case 8:
      return "str";
  }
  return {};
}

std::string_view atomic_release_store_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "stlrb";
    case 2:
      return "stlrh";
    case 4:
    case 8:
      return "stlr";
  }
  return {};
}

std::string_view atomic_exclusive_load_mnemonic(std::size_t width_bytes, bool acquire) {
  switch (width_bytes) {
    case 1:
      return acquire ? "ldaxrb" : "ldxrb";
    case 2:
      return acquire ? "ldaxrh" : "ldxrh";
    case 4:
    case 8:
      return acquire ? "ldaxr" : "ldxr";
  }
  return {};
}

std::string_view atomic_exclusive_store_mnemonic(std::size_t width_bytes, bool release) {
  switch (width_bytes) {
    case 1:
      return release ? "stlxrb" : "stxrb";
    case 2:
      return release ? "stlxrh" : "stxrh";
    case 4:
    case 8:
      return release ? "stlxr" : "stxr";
  }
  return {};
}

std::string_view atomic_rmw_operation_mnemonic(bir::AtomicRmwOpcode opcode) {
  switch (opcode) {
    case bir::AtomicRmwOpcode::Exchange:
      return "mov";
    case bir::AtomicRmwOpcode::Add:
      return "add";
    case bir::AtomicRmwOpcode::Sub:
      return "sub";
    case bir::AtomicRmwOpcode::And:
      return "and";
    case bir::AtomicRmwOpcode::Or:
      return "orr";
    case bir::AtomicRmwOpcode::Xor:
      return "eor";
    case bir::AtomicRmwOpcode::None:
      return {};
  }
  return {};
}

bool atomic_ordering_has_acquire(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

std::string_view required_primary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::string_view required_auxiliary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_auxiliary_printer_mnemonic(instruction);
}

std::optional<std::string> validate_selected_machine_node(const InstructionRecord& instruction) {
  if (instruction.surface != RecordSurfaceKind::MachineInstructionNode) {
    return std::string("printer requires surface machine_instruction_node, got ") +
           std::string(record_surface_kind_name(instruction.surface));
  }
  if (instruction.selection.status != MachineNodeSelectionStatus::Selected) {
    std::string diagnostic = "printer requires selected machine node, got ";
    diagnostic += machine_node_selection_status_name(instruction.selection.status);
    if (!instruction.selection.diagnostic.empty()) {
      diagnostic += ": ";
      diagnostic += instruction.selection.diagnostic;
    }
    return diagnostic;
  }
  return std::nullopt;
}

mir::TargetInstructionPrintResult print_assembler(
    const InstructionRecord& instruction,
    const AssemblerInstructionRecord& assembler) {
  if (instruction.family != InstructionFamily::Assembler) {
    return target_unsupported(bad_header(instruction) +
                              "inline-asm printer requires an assembler machine node");
  }
  const auto substituted = substitute_inline_asm_template(assembler);
  if (!substituted.lines.has_value()) {
    return target_unsupported(bad_header(instruction) + substituted.diagnostic);
  }
  return target_printed(*substituted.lines);
}

mir::TargetInstructionPrintResult print_spill_reload(
    const InstructionRecord& instruction,
    const SpillReloadInstructionRecord& spill_reload) {
  if (!spill_reload.scratch.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing scratch register");
  }
  if (!spill_reload.stack_offset_bytes.has_value() || !spill_reload.stack_offset_is_prepared_snapshot ||
      !spill_reload.slot_id.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing prepared stack-slot offset");
  }
  if (spill_reload.slot.support != MemoryOperandSupportKind::Prepared ||
      spill_reload.slot.base_kind != MemoryBaseKind::FrameSlot ||
      !spill_reload.slot.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is not a prepared frame-slot address");
  }

  const auto address = memory_address(spill_reload.slot);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload address is not printable");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload pseudo kind is unsupported");
  }

  std::ostringstream out;
  out << mnemonic << " " << register_name(*spill_reload.scratch) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_branch(const InstructionRecord& instruction,
                                               const BranchInstructionRecord& branch) {
  if (branch.target.function_name == c4c::kInvalidFunctionName ||
      branch.target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) + "branch target identity is missing");
  }
  if (!branch.conditional) {
    const auto mnemonic = comparison_unconditional_branch_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
    }
    std::ostringstream out;
    out << mnemonic << " " << block_label(branch.target.function_name, branch.target.block_label);
    return target_printed({out.str()});
  }
  if (!branch.target_pair.has_value() || !branch.condition.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch is missing target pair or condition operand");
  }
  const auto* condition = std::get_if<RegisterOperand>(&branch.condition->payload);
  if (branch.condition->kind != OperandKind::Register || condition == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "materialized-bool branch condition is not a register operand");
  }
  if (branch.condition_record.has_value() &&
      branch.condition_record->form != BranchConditionForm::MaterializedBool) {
    return target_unsupported(bad_header(instruction) +
                              "only materialized-bool conditional branches are printable");
  }

  const auto& targets = *branch.target_pair;
  if (targets.true_target.function_name == c4c::kInvalidFunctionName ||
      targets.true_target.block_label == c4c::kInvalidBlockLabel ||
      targets.false_target.function_name == c4c::kInvalidFunctionName ||
      targets.false_target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch target identity is missing");
  }

  const auto spelling = comparison_materialized_bool_branch_spelling(instruction);
  if (!spelling.has_value()) {
    return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
  }

  std::ostringstream condition_line;
  condition_line << spelling->condition_mnemonic << " " << register_name(*condition) << ", "
                 << block_label(targets.true_target.function_name,
                                targets.true_target.block_label);
  std::ostringstream branch_line;
  branch_line << spelling->branch_mnemonic << " "
              << block_label(targets.false_target.function_name,
                             targets.false_target.block_label);
  return target_printed({condition_line.str(), branch_line.str()});
}

mir::TargetInstructionPrintResult print_memory(const InstructionRecord& instruction,
                                               const MemoryInstructionRecord& memory) {
  if (memory.address.support != MemoryOperandSupportKind::Prepared ||
      !memory.address.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "memory address is not a prepared base+offset");
  }
  const auto address = memory_address(memory.address);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) + "memory address is not printable");
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "memory mnemonic is not printable");
  }

  if (memory.memory_kind == MemoryInstructionKind::Load) {
    if (!memory.result_register.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "load node is missing a structured destination register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*memory.result_register) << ", " << address;
    return target_printed({out.str()});
  }
  if (!memory.value.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "store node is missing stored value operand");
  }
  const auto* value = std::get_if<RegisterOperand>(&memory.value->payload);
  if (memory.value->kind == OperandKind::Register && value != nullptr) {
    std::ostringstream out;
    out << mnemonic << " " << register_name(*value) << ", " << address;
    return target_printed({out.str()});
  }
  const auto* immediate = std::get_if<ImmediateOperand>(&memory.value->payload);
  if (memory.value->kind == OperandKind::Immediate && immediate != nullptr) {
    const auto scratch = immediate_store_scratch_register(memory);
    if (!scratch.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "immediate store width is not printable");
    }
    const auto scratch_name = abi::register_name(*scratch);
    auto lines = materialize_immediate_store_value_lines(*scratch, *immediate, memory);
    if (lines.empty()) {
      return target_unsupported(bad_header(instruction) +
                                "immediate store value is not printable");
    }
    std::ostringstream store;
    store << mnemonic << " " << scratch_name << ", " << address;
    lines.push_back(store.str());
    return target_printed(std::move(lines));
  }
  return target_unsupported(bad_header(instruction) +
                            "store value is not a register or immediate operand");
}

mir::TargetInstructionPrintResult print_atomic_memory(
    const InstructionRecord& instruction,
    const AtomicMemoryInstructionRecord& atomic) {
  const auto pointer = atomic_pointer_register_name(atomic);
  if (atomic.atomic_kind != AtomicMemoryInstructionKind::Fence && !pointer.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "atomic memory node is missing printable pointer register");
  }
  const auto address = pointer.has_value() ? "[" + *pointer + "]" : std::string{};

  switch (atomic.atomic_kind) {
    case AtomicMemoryInstructionKind::Load: {
      if (!atomic.result_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic load is missing result register");
      }
      const auto result = atomic_value_register_name(*atomic.result_register, atomic.width_bytes);
      const auto mnemonic = atomic.acquire_semantics
                                ? atomic_acquire_load_mnemonic(atomic.width_bytes)
                                : atomic_plain_load_mnemonic(atomic.width_bytes);
      if (!result.has_value() || mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic load has unsupported width or register view");
      }
      std::ostringstream out;
      out << mnemonic << " " << *result << ", " << address;
      return target_printed({out.str()});
    }
    case AtomicMemoryInstructionKind::Store: {
      if (!atomic.stored_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic store is missing stored register");
      }
      const auto stored = atomic_value_register_name(*atomic.stored_register, atomic.width_bytes);
      const auto mnemonic = atomic.release_semantics
                                ? atomic_release_store_mnemonic(atomic.width_bytes)
                                : atomic_plain_store_mnemonic(atomic.width_bytes);
      if (!stored.has_value() || mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic store has unsupported width or register view");
      }
      std::ostringstream out;
      out << mnemonic << " " << *stored << ", " << address;
      return target_printed({out.str()});
    }
    case AtomicMemoryInstructionKind::Fence:
      if (!atomic.memory_barrier_required) {
        return target_unsupported(bad_header(instruction) +
                                  "relaxed atomic fence is outside the printable subset");
      }
      return target_printed({"dmb ish"});
    case AtomicMemoryInstructionKind::RmwLoop: {
      if (!atomic.result_register.has_value() || !atomic.stored_register.has_value() ||
          !atomic.rmw_new_value_register.has_value() ||
          !atomic.exclusive_status_register.has_value() || !atomic.exclusive_retry_loop) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic rmw loop is missing structured result, value, scratch, or status registers");
      }
      const auto old_value = atomic_value_register_name(*atomic.result_register,
                                                        atomic.width_bytes);
      const auto operand = atomic_value_register_name(*atomic.stored_register,
                                                      atomic.width_bytes);
      const auto new_value = atomic_value_register_name(*atomic.rmw_new_value_register,
                                                        atomic.width_bytes);
      const auto status = atomic_value_register_name(*atomic.exclusive_status_register, 4);
      const auto load = atomic_exclusive_load_mnemonic(atomic.width_bytes,
                                                       atomic.acquire_semantics);
      const auto store = atomic_exclusive_store_mnemonic(atomic.width_bytes,
                                                         atomic.release_semantics);
      const auto op = atomic_rmw_operation_mnemonic(atomic.rmw_opcode);
      if (!old_value.has_value() || !operand.has_value() || !new_value.has_value() ||
          !status.has_value() || load.empty() || store.empty() || op.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic rmw loop has unsupported width, opcode, or register view");
      }
      const auto retry = atomic_loop_label(atomic, "retry");
      std::vector<std::string> lines;
      lines.push_back(retry + ":");
      lines.push_back(std::string(load) + " " + *old_value + ", " + address);
      if (atomic.rmw_opcode == bir::AtomicRmwOpcode::Exchange) {
        lines.push_back(std::string(op) + " " + *new_value + ", " + *operand);
      } else {
        lines.push_back(std::string(op) + " " + *new_value + ", " + *old_value + ", " +
                        *operand);
      }
      lines.push_back(std::string(store) + " " + *status + ", " + *new_value + ", " +
                      address);
      lines.push_back("cbnz " + *status + ", " + retry);
      return target_printed(std::move(lines));
    }
    case AtomicMemoryInstructionKind::CompareExchangeLoop: {
      if (!atomic.expected_register.has_value() || !atomic.desired_register.has_value() ||
          !atomic.result_register.has_value() || !atomic.exclusive_status_register.has_value() ||
          !atomic.exclusive_retry_loop || !atomic.compare_exchange_failure_clears_monitor) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic compare-exchange loop is missing structured operands, status, or monitor-clear facts");
      }
      const RegisterOperand* loaded_operand = nullptr;
      if (atomic.compare_exchange_result_is_old_value) {
        loaded_operand = &*atomic.result_register;
      } else if (atomic.compare_loaded_register.has_value()) {
        loaded_operand = &*atomic.compare_loaded_register;
      }
      if (loaded_operand == nullptr) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic compare-exchange loop is missing loaded-value register");
      }
      const bool failure_acquire = atomic_ordering_has_acquire(atomic.failure_ordering);
      const bool success_needs_post_acquire =
          atomic.acquire_semantics && !failure_acquire;
      const auto loaded = atomic_value_register_name(*loaded_operand, atomic.width_bytes);
      const auto expected = atomic_value_register_name(*atomic.expected_register,
                                                       atomic.width_bytes);
      const auto desired = atomic_value_register_name(*atomic.desired_register,
                                                      atomic.width_bytes);
      const auto result = atomic_value_register_name(*atomic.result_register,
                                                     atomic.compare_exchange_result_is_boolean
                                                         ? 4
                                                         : atomic.width_bytes);
      const auto status = atomic_value_register_name(*atomic.exclusive_status_register, 4);
      const auto load = atomic_exclusive_load_mnemonic(atomic.width_bytes, failure_acquire);
      const auto store = atomic_exclusive_store_mnemonic(atomic.width_bytes,
                                                         atomic.release_semantics);
      if (!loaded.has_value() || !expected.has_value() || !desired.has_value() ||
          !result.has_value() || !status.has_value() || load.empty() || store.empty()) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic compare-exchange loop has unsupported width or register view");
      }
      const auto retry = atomic_loop_label(atomic, "retry");
      const auto failure = atomic_loop_label(atomic, "failure");
      const auto done = atomic_loop_label(atomic, "done");
      std::vector<std::string> lines;
      lines.push_back(retry + ":");
      lines.push_back(std::string(load) + " " + *loaded + ", " + address);
      lines.push_back("cmp " + *loaded + ", " + *expected);
      lines.push_back("b.ne " + failure);
      lines.push_back(std::string(store) + " " + *status + ", " + *desired + ", " +
                      address);
      lines.push_back("cbnz " + *status + ", " + retry);
      if (success_needs_post_acquire) {
        lines.push_back("dmb ishld");
      }
      if (atomic.compare_exchange_result_is_boolean) {
        lines.push_back("mov " + *result + ", #1");
        lines.push_back("b " + done);
      }
      lines.push_back(failure + ":");
      lines.push_back("clrex");
      if (atomic.compare_exchange_result_is_boolean) {
        lines.push_back("mov " + *result + ", #0");
        lines.push_back(done + ":");
      }
      return target_printed(std::move(lines));
    }
  }
  return target_unsupported(bad_header(instruction) +
                            "atomic memory kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_frame(const InstructionRecord& instruction,
                                              const FrameInstructionRecord& frame) {
  if (frame.source_frame == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame provenance");
  }
  if (frame.function_name == c4c::kInvalidFunctionName) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing function identity");
  }
  if (frame.frame_alignment_bytes == 0) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame alignment");
  }
  if (frame.has_dynamic_stack) {
    return target_unsupported(bad_header(instruction) +
                              "dynamic-stack frame node is outside the printable subset");
  }
  if (!frame.saved_callee_registers.empty() || frame.callee_save.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "callee-save frame node is outside the printable subset");
  }

  if (frame.frame_kind != FrameInstructionKind::PrologueSetup &&
      frame.frame_kind != FrameInstructionKind::EpilogueTeardown) {
    return target_unsupported(bad_header(instruction) +
                              "frame node kind is outside the printable subset");
  }
  if (frame.frame_size_bytes == 0) {
    return target_printed({});
  }
  if (frame.frame_size_bytes > 4095) {
    return target_unsupported(bad_header(instruction) +
                              "frame adjustment immediate is outside the plain #imm "
                              "encoding range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "frame mnemonic is not printable");
  }

  std::ostringstream out;
  out << mnemonic << " sp, sp, #" << frame.frame_size_bytes;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_scalar(const InstructionRecord& instruction,
                                               const ScalarInstructionRecord& scalar) {
  if (!scalar.result_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar node is missing a structured destination register operand");
  }
  if (scalar.scalar_cast.has_value()) {
    return print_scalar_cast_instruction(instruction, scalar, bad_header(instruction));
  }
  auto alu = make_scalar_alu_print_lines(instruction, scalar);
  if (alu.lines.has_value()) {
    return target_printed(std::move(*alu.lines));
  }
  return target_unsupported(bad_header(instruction) + alu.diagnostic);
}

mir::TargetInstructionPrintResult print_return(const InstructionRecord& instruction,
                                               const ReturnInstructionRecord& ret) {
  std::vector<std::string> lines;
  const auto print_form = classify_return_value_print_form(ret);
  switch (print_form) {
    case ReturnValuePrintForm::PrimaryReturn: {
      const auto return_mnemonic = required_primary_mnemonic(instruction);
      if (return_mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
      }
      return target_printed({std::string(return_mnemonic)});
    }
    case ReturnValuePrintForm::Unsupported:
      return target_unsupported(bad_header(instruction) +
                                "return value is not a printable immediate operand");
    case ReturnValuePrintForm::ImmediateMaterialization: {
      const auto* immediate = ret.value.has_value()
                                  ? std::get_if<ImmediateOperand>(&ret.value->payload)
                                  : nullptr;
      if (immediate == nullptr) {
        return target_unsupported(bad_header(instruction) +
                                  "return value is not a printable immediate operand");
      }
      if (!is_printable_return_immediate_materialization_value(*immediate)) {
        return target_unsupported(bad_header(instruction) +
                                  "return immediate is outside the selected printable subset");
      }
      const auto result_register = return_immediate_materialization_register(ret.value_type);
      if (!result_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "return type is outside the selected printable subset");
      }
      const auto result_register_name = abi::register_name(*result_register);
      const auto move_mnemonic = required_auxiliary_mnemonic(instruction);
      if (move_mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "return move mnemonic is not printable");
      }
      std::ostringstream move_line;
      move_line << move_mnemonic << " " << result_register_name << ", #"
                << immediate->signed_value;
      lines.push_back(move_line.str());
      break;
    }
    case ReturnValuePrintForm::NoValue:
      break;
  }
  const auto return_mnemonic = required_primary_mnemonic(instruction);
  if (return_mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
  }
  lines.emplace_back(return_mnemonic);
  return target_printed(std::move(lines));
}

}  // namespace

mir::TargetInstructionPrintResult MachineInstructionPrinter::print_instruction(
    const mir::MachinePrintContext&,
    const mir::MachineInstruction<InstructionRecord>& instruction) const {
  return print_machine_instruction_line_payloads(instruction.target);
}

mir::TargetInstructionPrintResult print_machine_instruction_line_payloads(
    const InstructionRecord& instruction) {
  if (const auto invalid = validate_selected_machine_node(instruction); invalid.has_value()) {
    return target_unsupported(*invalid);
  }

  if (const auto* spill_reload =
          std::get_if<SpillReloadInstructionRecord>(&instruction.payload)) {
    return print_spill_reload(instruction, *spill_reload);
  }
  if (const auto* branch = std::get_if<BranchInstructionRecord>(&instruction.payload)) {
    return print_branch(instruction, *branch);
  }
  if (const auto* memory = std::get_if<MemoryInstructionRecord>(&instruction.payload)) {
    return print_memory(instruction, *memory);
  }
  if (const auto* atomic =
          std::get_if<AtomicMemoryInstructionRecord>(&instruction.payload)) {
    return print_atomic_memory(instruction, *atomic);
  }
  if (const auto* frame = std::get_if<FrameInstructionRecord>(&instruction.payload)) {
    return print_frame(instruction, *frame);
  }
  if (const auto* move =
          std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_move(instruction, *move);
  }
  if (const auto* binding =
          std::get_if<CallBoundaryAbiBindingInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_abi_binding(instruction, *binding);
  }
  if (const auto* call = std::get_if<CallInstructionRecord>(&instruction.payload)) {
    return print_call(instruction, *call);
  }
  if (const auto* address = std::get_if<AddressMaterializationRecord>(&instruction.payload)) {
    return print_address_materialization_instruction(instruction, *address);
  }
  if (const auto* transport = std::get_if<I128TransportRecord>(&instruction.payload)) {
    return print_i128_transport(instruction, *transport);
  }
  if (const auto* transport = std::get_if<F128TransportRecord>(&instruction.payload)) {
    return print_f128_transport(instruction, *transport);
  }
  if (const auto* pair = std::get_if<I128PairOperationRecord>(&instruction.payload)) {
    return print_i128_pair_operation(instruction, *pair);
  }
  if (const auto* shift = std::get_if<I128ShiftRecord>(&instruction.payload)) {
    return print_i128_shift(instruction, *shift);
  }
  if (const auto* compare = std::get_if<I128CompareRecord>(&instruction.payload)) {
    return print_i128_compare(instruction, *compare);
  }
  if (const auto* helper =
          std::get_if<I128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_i128_runtime_helper(instruction, *helper);
  }
  if (const auto* helper =
          std::get_if<F128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_f128_runtime_helper(instruction, *helper);
  }
  if (const auto* scalar = std::get_if<ScalarInstructionRecord>(&instruction.payload)) {
    return print_scalar(instruction, *scalar);
  }
  if (instruction.family == InstructionFamily::Intrinsic) {
    return print_intrinsic_instruction(instruction);
  }
  if (const auto* assembler = std::get_if<AssemblerInstructionRecord>(&instruction.payload)) {
    return print_assembler(instruction, *assembler);
  }
  if (const auto* ret = std::get_if<ReturnInstructionRecord>(&instruction.payload)) {
    return print_return(instruction, *ret);
  }
  return target_unsupported(bad_header(instruction) +
                            "instruction family is not in the printable subset");
}

}  // namespace c4c::backend::aarch64::codegen
