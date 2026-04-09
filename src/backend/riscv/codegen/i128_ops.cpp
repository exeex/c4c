// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/i128_ops.rs
// Self-contained translation unit: the shared RISC-V C++ codegen surface does
// not exist yet, so this file keeps the i128 helper logic concrete without
// inventing a repo-wide header.

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::codegen {

enum class IrCmpOp {
  Eq,
  Ne,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
};

enum class IrType {
  I32,
  U32,
  I64,
  U64,
  F32,
  F64,
  F128,
};

struct StackSlot {
  std::int64_t offset = 0;
};

struct Value {
  std::uint32_t id = 0;
  std::optional<std::string> reg;
  std::optional<StackSlot> slot;
  bool is_alloca = false;
  bool is_i128 = false;
};

struct I128Const {
  __int128 value = 0;
};

using Operand = std::variant<Value, I128Const, std::int64_t>;

struct EmitBuffer {
  std::vector<std::string> lines;
  std::uint64_t label_counter = 0;

  void emit(std::string line) { lines.push_back(std::move(line)); }

  void emit_fmt(std::string line) { emit(std::move(line)); }

  std::string fresh_label(std::string_view prefix) {
    return std::string(".L") + std::string(prefix) + "_" + std::to_string(label_counter++);
  }
};

struct RiscvI128Ops {
  EmitBuffer state;

  static std::string reg_name(const Value& value) {
    return value.reg.value_or("t0");
  }

  void emit_store_to_s0(std::string_view reg, std::int64_t offset, std::string_view instr) {
    this->state.emit(std::string("    ") + std::string(instr) + " " + std::string(reg) + ", " +
                     std::to_string(offset) + "(s0)");
  }

  void emit_load_from_s0(std::string_view reg, std::int64_t offset, std::string_view instr) {
    this->state.emit(std::string("    ") + std::string(instr) + " " + std::string(reg) + ", " +
                     std::to_string(offset) + "(s0)");
  }

  void emit_alloca_addr(std::string_view reg, std::int64_t offset) {
    this->state.emit(std::string("    addi ") + std::string(reg) + ", s0, " + std::to_string(offset));
  }

  void operand_to_t0(const Operand& op) { operand_to_t0_t1(op); }

  void operand_to_t0_t1(const Operand& op) {
    std::visit(
        [&](const auto& value) {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, I128Const>) {
            const auto uvalue = static_cast<unsigned __int128>(value.value);
            const auto low = static_cast<std::int64_t>(static_cast<std::uint64_t>(uvalue));
            const auto high = static_cast<std::int64_t>(static_cast<std::uint64_t>(uvalue >> 64));
            this->state.emit_fmt("    li t0, " + std::to_string(low));
            this->state.emit_fmt("    li t1, " + std::to_string(high));
          } else if constexpr (std::is_same_v<T, std::int64_t>) {
            this->state.emit_fmt("    li t0, " + std::to_string(value));
            this->state.emit("    li t1, 0");
          } else if constexpr (std::is_same_v<T, Value>) {
            if (value.slot.has_value()) {
              const auto offset = value.slot->offset;
              if (value.is_alloca) {
                this->emit_alloca_addr("t0", offset);
                this->state.emit("    li t1, 0");
              } else if (value.is_i128) {
                this->emit_load_from_s0("t0", offset, "ld");
                this->emit_load_from_s0("t1", offset + 8, "ld");
              } else if (value.reg.has_value()) {
                this->state.emit("    mv t0, " + *value.reg);
                this->state.emit("    li t1, 0");
              } else {
                this->emit_load_from_s0("t0", offset, "ld");
                this->state.emit("    li t1, 0");
              }
            } else if (value.reg.has_value()) {
              this->state.emit("    mv t0, " + *value.reg);
              this->state.emit("    li t1, 0");
            } else {
              this->state.emit("    li t0, 0");
              this->state.emit("    li t1, 0");
            }
          }
        },
        op);
  }

  void store_t0_to(const Value& dest) {
    if (dest.reg.has_value()) {
      this->state.emit("    mv " + *dest.reg + ", t0");
    } else if (dest.slot.has_value()) {
      this->emit_store_to_s0("t0", dest.slot->offset, "sd");
    }
  }

  void store_t0_t1_to(const Value& dest) {
    if (dest.slot.has_value()) {
      this->emit_store_to_s0("t0", dest.slot->offset, "sd");
      this->emit_store_to_s0("t1", dest.slot->offset + 8, "sd");
    }
  }

  void prep_i128_binop(const Operand& lhs, const Operand& rhs) {
    this->operand_to_t0_t1(lhs);
    this->state.emit("    mv t3, t0");
    this->state.emit("    mv t4, t1");
    this->operand_to_t0_t1(rhs);
    this->state.emit("    mv t5, t0");
    this->state.emit("    mv t6, t1");
  }

  void emit_load_acc_pair_impl(const Operand& op) { this->operand_to_t0_t1(op); }

  void emit_store_acc_pair_impl(const Value& dest) { this->store_t0_t1_to(dest); }

  void emit_store_pair_to_slot_impl(StackSlot slot) {
    this->emit_store_to_s0("t0", slot.offset, "sd");
    this->emit_store_to_s0("t1", slot.offset + 8, "sd");
  }

  void emit_load_pair_from_slot_impl(StackSlot slot) {
    this->emit_load_from_s0("t0", slot.offset, "ld");
    this->emit_load_from_s0("t1", slot.offset + 8, "ld");
  }

  void emit_save_acc_pair_impl() {
    this->state.emit("    mv t3, t0");
    this->state.emit("    mv t4, t1");
  }

  void emit_store_pair_indirect_impl() {
    this->state.emit("    sd t3, 0(t5)");
    this->state.emit("    sd t4, 8(t5)");
  }

  void emit_load_pair_indirect_impl() {
    this->state.emit("    ld t0, 0(t5)");
    this->state.emit("    ld t1, 8(t5)");
  }

  void emit_i128_neg_impl() {
    this->state.emit("    not t0, t0");
    this->state.emit("    not t1, t1");
    this->state.emit("    addi t0, t0, 1");
    this->state.emit("    seqz t2, t0");
    this->state.emit("    add t1, t1, t2");
  }

  void emit_i128_not_impl() {
    this->state.emit("    not t0, t0");
    this->state.emit("    not t1, t1");
  }

  void emit_sign_extend_acc_high_impl() { this->state.emit("    srai t1, t0, 63"); }

  void emit_zero_acc_high_impl() { this->state.emit("    li t1, 0"); }

  void emit_i128_prep_binop_impl(const Operand& lhs, const Operand& rhs) { this->prep_i128_binop(lhs, rhs); }

  void emit_i128_add_impl() {
    this->state.emit("    add t0, t3, t5");
    this->state.emit("    sltu t2, t0, t3");
    this->state.emit("    add t1, t4, t6");
    this->state.emit("    add t1, t1, t2");
  }

  void emit_i128_sub_impl() {
    this->state.emit("    sltu t2, t3, t5");
    this->state.emit("    sub t0, t3, t5");
    this->state.emit("    sub t1, t4, t6");
    this->state.emit("    sub t1, t1, t2");
  }

  void emit_i128_mul_impl() {
    this->state.emit("    mul t0, t3, t5");
    this->state.emit("    mulhu t1, t3, t5");
    this->state.emit("    mul t2, t4, t5");
    this->state.emit("    add t1, t1, t2");
    this->state.emit("    mul t2, t3, t6");
    this->state.emit("    add t1, t1, t2");
  }

  void emit_i128_and_impl() {
    this->state.emit("    and t0, t3, t5");
    this->state.emit("    and t1, t4, t6");
  }

  void emit_i128_or_impl() {
    this->state.emit("    or t0, t3, t5");
    this->state.emit("    or t1, t4, t6");
  }

  void emit_i128_xor_impl() {
    this->state.emit("    xor t0, t3, t5");
    this->state.emit("    xor t1, t4, t6");
  }

  void emit_i128_shl_impl() {
    const auto lbl = this->state.fresh_label("shl128");
    const auto done = this->state.fresh_label("shl128_done");
    const auto noop = this->state.fresh_label("shl128_noop");
    this->state.emit("    andi t5, t5, 127");
    this->state.emit("    beqz t5, " + noop);
    this->state.emit("    li t2, 64");
    this->state.emit("    bge t5, t2, " + lbl);
    this->state.emit("    sll t1, t4, t5");
    this->state.emit("    sub t2, t2, t5");
    this->state.emit("    srl t6, t3, t2");
    this->state.emit("    or t1, t1, t6");
    this->state.emit("    sll t0, t3, t5");
    this->state.emit("    j " + done);
    this->state.emit(lbl + ":");
    this->state.emit("    li t2, 64");
    this->state.emit("    sub t5, t5, t2");
    this->state.emit("    sll t1, t3, t5");
    this->state.emit("    li t0, 0");
    this->state.emit("    j " + done);
    this->state.emit(noop + ":");
    this->state.emit("    mv t0, t3");
    this->state.emit("    mv t1, t4");
    this->state.emit(done + ":");
  }

  void emit_i128_lshr_impl() {
    const auto lbl = this->state.fresh_label("lshr128");
    const auto done = this->state.fresh_label("lshr128_done");
    const auto noop = this->state.fresh_label("lshr128_noop");
    this->state.emit("    andi t5, t5, 127");
    this->state.emit("    beqz t5, " + noop);
    this->state.emit("    li t2, 64");
    this->state.emit("    bge t5, t2, " + lbl);
    this->state.emit("    srl t0, t3, t5");
    this->state.emit("    sub t2, t2, t5");
    this->state.emit("    sll t6, t4, t2");
    this->state.emit("    or t0, t0, t6");
    this->state.emit("    srl t1, t4, t5");
    this->state.emit("    j " + done);
    this->state.emit(lbl + ":");
    this->state.emit("    li t2, 64");
    this->state.emit("    sub t5, t5, t2");
    this->state.emit("    srl t0, t4, t5");
    this->state.emit("    li t1, 0");
    this->state.emit("    j " + done);
    this->state.emit(noop + ":");
    this->state.emit("    mv t0, t3");
    this->state.emit("    mv t1, t4");
    this->state.emit(done + ":");
  }

  void emit_i128_ashr_impl() {
    const auto lbl = this->state.fresh_label("ashr128");
    const auto done = this->state.fresh_label("ashr128_done");
    const auto noop = this->state.fresh_label("ashr128_noop");
    this->state.emit("    andi t5, t5, 127");
    this->state.emit("    beqz t5, " + noop);
    this->state.emit("    li t2, 64");
    this->state.emit("    bge t5, t2, " + lbl);
    this->state.emit("    srl t0, t3, t5");
    this->state.emit("    sub t2, t2, t5");
    this->state.emit("    sll t6, t4, t2");
    this->state.emit("    or t0, t0, t6");
    this->state.emit("    sra t1, t4, t5");
    this->state.emit("    j " + done);
    this->state.emit(lbl + ":");
    this->state.emit("    li t2, 64");
    this->state.emit("    sub t5, t5, t2");
    this->state.emit("    sra t0, t4, t5");
    this->state.emit("    srai t1, t4, 63");
    this->state.emit("    j " + done);
    this->state.emit(noop + ":");
    this->state.emit("    mv t0, t3");
    this->state.emit("    mv t1, t4");
    this->state.emit(done + ":");
  }

  void emit_i128_prep_shift_lhs_impl(const Operand& lhs) {
    this->operand_to_t0_t1(lhs);
    this->state.emit("    mv t3, t0");
    this->state.emit("    mv t4, t1");
  }

  void emit_i128_shl_const_impl(std::uint32_t amount) {
    const std::uint32_t shift = amount & 127;
    if (shift == 0) {
      this->state.emit("    mv t0, t3");
      this->state.emit("    mv t1, t4");
    } else if (shift == 64) {
      this->state.emit("    mv t1, t3");
      this->state.emit("    li t0, 0");
    } else if (shift > 64) {
      this->state.emit("    slli t1, t3, " + std::to_string(shift - 64));
      this->state.emit("    li t0, 0");
    } else {
      this->state.emit("    slli t1, t4, " + std::to_string(shift));
      this->state.emit("    srli t2, t3, " + std::to_string(64 - shift));
      this->state.emit("    or t1, t1, t2");
      this->state.emit("    slli t0, t3, " + std::to_string(shift));
    }
  }

  void emit_i128_lshr_const_impl(std::uint32_t amount) {
    const std::uint32_t shift = amount & 127;
    if (shift == 0) {
      this->state.emit("    mv t0, t3");
      this->state.emit("    mv t1, t4");
    } else if (shift == 64) {
      this->state.emit("    mv t0, t4");
      this->state.emit("    li t1, 0");
    } else if (shift > 64) {
      this->state.emit("    srli t0, t4, " + std::to_string(shift - 64));
      this->state.emit("    li t1, 0");
    } else {
      this->state.emit("    srli t0, t3, " + std::to_string(shift));
      this->state.emit("    slli t2, t4, " + std::to_string(64 - shift));
      this->state.emit("    or t0, t0, t2");
      this->state.emit("    srli t1, t4, " + std::to_string(shift));
    }
  }

  void emit_i128_ashr_const_impl(std::uint32_t amount) {
    const std::uint32_t shift = amount & 127;
    if (shift == 0) {
      this->state.emit("    mv t0, t3");
      this->state.emit("    mv t1, t4");
    } else if (shift == 64) {
      this->state.emit("    mv t0, t4");
      this->state.emit("    srai t1, t4, 63");
    } else if (shift > 64) {
      this->state.emit("    srai t0, t4, " + std::to_string(shift - 64));
      this->state.emit("    srai t1, t4, 63");
    } else {
      this->state.emit("    srli t0, t3, " + std::to_string(shift));
      this->state.emit("    slli t2, t4, " + std::to_string(64 - shift));
      this->state.emit("    or t0, t0, t2");
      this->state.emit("    srai t1, t4, " + std::to_string(shift));
    }
  }

  void emit_i128_divrem_call_impl(const std::string& func_name, const Operand& lhs, const Operand& rhs) {
    this->operand_to_t0_t1(lhs);
    this->state.emit("    mv a0, t0");
    this->state.emit("    mv a1, t1");
    this->operand_to_t0_t1(rhs);
    this->state.emit("    mv a2, t0");
    this->state.emit("    mv a3, t1");
    this->state.emit("    call " + func_name);
    this->state.emit("    mv t0, a0");
    this->state.emit("    mv t1, a1");
  }

  void emit_i128_store_result_impl(const Value& dest) { this->store_t0_t1_to(dest); }

  void emit_i128_to_float_call_impl(const Operand& src, bool from_signed, IrType to_ty) {
    this->operand_to_t0_t1(src);
    this->state.emit("    mv a0, t0");
    this->state.emit("    mv a1, t1");
    const char* func_name = nullptr;
    if (from_signed && to_ty == IrType::F64) {
      func_name = "__floattidf";
    } else if (from_signed && to_ty == IrType::F32) {
      func_name = "__floattisf";
    } else if (!from_signed && to_ty == IrType::F64) {
      func_name = "__floatuntidf";
    } else if (!from_signed && to_ty == IrType::F32) {
      func_name = "__floatuntisf";
    } else {
      throw std::invalid_argument("unsupported i128-to-float conversion");
    }
    this->state.emit(std::string("    call ") + func_name);
    if (to_ty == IrType::F32) {
      this->state.emit("    fmv.x.w t0, fa0");
    } else {
      this->state.emit("    fmv.x.d t0, fa0");
    }
  }

  void emit_float_to_i128_call_impl(const Operand& src, bool to_signed, IrType from_ty) {
    this->operand_to_t0(src);
    if (from_ty == IrType::F32) {
      this->state.emit("    fmv.w.x fa0, t0");
    } else {
      this->state.emit("    fmv.d.x fa0, t0");
    }
    const char* func_name = nullptr;
    if (to_signed && from_ty == IrType::F64) {
      func_name = "__fixdfti";
    } else if (to_signed && from_ty == IrType::F32) {
      func_name = "__fixsfti";
    } else if (!to_signed && from_ty == IrType::F64) {
      func_name = "__fixunsdfti";
    } else if (!to_signed && from_ty == IrType::F32) {
      func_name = "__fixunssfti";
    } else {
      throw std::invalid_argument("unsupported float-to-i128 conversion");
    }
    this->state.emit(std::string("    call ") + func_name);
    this->state.emit("    mv t0, a0");
    this->state.emit("    mv t1, a1");
  }

  void emit_i128_cmp_eq_impl(bool is_ne) {
    this->state.emit("    xor t0, t3, t5");
    this->state.emit("    xor t1, t4, t6");
    this->state.emit("    or t0, t0, t1");
    this->state.emit(std::string("    ") + (is_ne ? "snez" : "seqz") + " t0, t0");
  }

  void emit_i128_cmp_ordered_impl(IrCmpOp op) {
    const auto hi_differ = this->state.fresh_label("cmp128_hi_diff");
    const auto hi_equal = this->state.fresh_label("cmp128_hi_eq");
    const auto done = this->state.fresh_label("cmp128_done");
    this->state.emit("    bne t4, t6, " + hi_differ);
    this->state.emit("    j " + hi_equal);
    this->state.emit(hi_differ + ":");
    switch (op) {
      case IrCmpOp::Slt:
      case IrCmpOp::Sle:
        this->state.emit("    slt t0, t4, t6");
        break;
      case IrCmpOp::Sgt:
      case IrCmpOp::Sge:
        this->state.emit("    slt t0, t6, t4");
        break;
      case IrCmpOp::Ult:
      case IrCmpOp::Ule:
        this->state.emit("    sltu t0, t4, t6");
        break;
      case IrCmpOp::Ugt:
      case IrCmpOp::Uge:
        this->state.emit("    sltu t0, t6, t4");
        break;
      case IrCmpOp::Eq:
      case IrCmpOp::Ne:
        throw std::invalid_argument("i128 ordered cmp got equality op");
    }
    this->state.emit("    j " + done);
    this->state.emit(hi_equal + ":");
    switch (op) {
      case IrCmpOp::Slt:
      case IrCmpOp::Ult:
        this->state.emit("    sltu t0, t3, t5");
        break;
      case IrCmpOp::Sle:
      case IrCmpOp::Ule:
        this->state.emit("    sltu t0, t5, t3");
        this->state.emit("    xori t0, t0, 1");
        break;
      case IrCmpOp::Sgt:
      case IrCmpOp::Ugt:
        this->state.emit("    sltu t0, t5, t3");
        break;
      case IrCmpOp::Sge:
      case IrCmpOp::Uge:
        this->state.emit("    sltu t0, t3, t5");
        this->state.emit("    xori t0, t0, 1");
        break;
      case IrCmpOp::Eq:
      case IrCmpOp::Ne:
        throw std::invalid_argument("i128 ordered cmp got equality op");
    }
    this->state.emit(done + ":");
  }

  void emit_i128_cmp_store_result_impl(const Value& dest) { this->store_t0_to(dest); }
};

}  // namespace c4c::backend::riscv::codegen

