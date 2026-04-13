#include "x86_codegen.hpp"

#include "../../bir.hpp"
#include "../../regalloc.hpp"

namespace c4c::backend::x86 {

namespace {

bool is_32bit_int_type(IrType ty) { return ty == IrType::I32 || ty == IrType::U32; }

bool is_unsigned_int_type(IrType ty) {
  switch (ty) {
    case IrType::U16:
    case IrType::U32:
    case IrType::U64:
    case IrType::Ptr:
      return true;
    default:
      return false;
  }
}

std::optional<c4c::backend::PhysReg> assigned_reg(const X86CodegenState& state,
                                                  std::uint32_t value_id) {
  if (const auto reg = state.assigned_reg_index(value_id)) {
    return c4c::backend::PhysReg{*reg};
  }
  return std::nullopt;
}

std::optional<c4c::backend::PhysReg> operand_reg(const X86Codegen& codegen, const Operand& operand) {
  return assigned_reg(codegen.state, operand.raw);
}

std::optional<c4c::backend::PhysReg> dest_reg(const X86Codegen& codegen, const Value& dest) {
  return assigned_reg(codegen.state, dest.raw);
}

std::optional<std::uint8_t> lea_scale_for_mul(std::int64_t imm) {
  switch (imm) {
    case 3: return 2;
    case 5: return 4;
    case 9: return 8;
    default: return std::nullopt;
  }
}

void emit_mov_operand_to_reg(X86Codegen& codegen, const Operand& operand, const char* reg_name) {
  codegen.operand_to_reg(operand, reg_name);
}

void emit_mov_operand_to_phys_reg(X86Codegen& codegen,
                                  const Operand& operand,
                                  c4c::backend::PhysReg target) {
  codegen.operand_to_reg(operand, phys_reg_name(target));
}

void emit_operand_to_callee_reg(X86Codegen& codegen,
                                const Operand& operand,
                                c4c::backend::PhysReg target) {
  const char* target_name_32 = phys_reg_name_32(target);
  if (const auto imm = codegen.const_as_imm32(operand)) {
    if (*imm == 0) {
      codegen.state.emit(std::string("    xorl %") + target_name_32 + ", %" + target_name_32);
    } else {
      codegen.operand_to_reg(operand, phys_reg_name(target));
    }
    return;
  }

  emit_mov_operand_to_phys_reg(codegen, operand, target);
}

void emit_sext32_if_needed(X86Codegen& codegen,
                           c4c::backend::PhysReg reg,
                           bool is_unsigned) {
  if (!is_unsigned) {
    codegen.state.emit(std::string("    movslq %") + phys_reg_name_32(reg) + ", %" +
                       phys_reg_name(reg));
  }
}

void emit_simple_alu_reg_direct(X86Codegen& codegen,
                                c4c::backend::bir::BinaryOpcode opcode,
                                const Operand& lhs,
                                const Operand& rhs,
                                c4c::backend::PhysReg dest,
                                bool use_32bit,
                                bool is_unsigned) {
  const char* dest_name = phys_reg_name(dest);
  const char* dest_name_32 = phys_reg_name_32(dest);
  if (const auto imm = codegen.const_as_imm32(rhs)) {
    emit_operand_to_callee_reg(codegen, lhs, dest);
    if (opcode == c4c::backend::bir::BinaryOpcode::Mul) {
      if (const auto scale = lea_scale_for_mul(*imm)) {
        if (use_32bit) {
          codegen.state.emit(std::string("    leal (%") + dest_name_32 + ", %" + dest_name_32 + ", " +
                            std::to_string(*scale) + "), %" + dest_name_32);
          emit_sext32_if_needed(codegen, dest, is_unsigned);
        } else {
          codegen.state.emit(std::string("    leaq (%") + dest_name + ", %" + dest_name + ", " +
                            std::to_string(*scale) + "), %" + dest_name);
        }
      } else if (use_32bit) {
        codegen.state.emit(std::string("    imull $") + std::to_string(*imm) + ", %" + dest_name_32 +
                          ", %" + dest_name_32);
        emit_sext32_if_needed(codegen, dest, is_unsigned);
      } else {
        codegen.state.emit(std::string("    imulq $") + std::to_string(*imm) + ", %" + dest_name +
                          ", %" + dest_name);
      }
    } else {
      const char* mnemonic = x86_alu_mnemonic(opcode);
      if (use_32bit &&
          (opcode == c4c::backend::bir::BinaryOpcode::Add ||
           opcode == c4c::backend::bir::BinaryOpcode::Sub)) {
        codegen.state.emit(std::string("    ") + mnemonic + "l $" + std::to_string(*imm) + ", %" +
                          dest_name_32);
        emit_sext32_if_needed(codegen, dest, is_unsigned);
      } else {
        codegen.state.emit(std::string("    ") + mnemonic + "q $" + std::to_string(*imm) + ", %" +
                          dest_name);
      }
    }
    codegen.state.reg_cache.invalidate_acc();
    return;
  }

  const auto rhs_reg = operand_reg(codegen, rhs);
  const bool rhs_conflicts = rhs_reg.has_value() && *rhs_reg == dest;

  if (rhs_conflicts) {
    emit_mov_operand_to_reg(codegen, rhs, "rax");
    emit_operand_to_callee_reg(codegen, lhs, dest);
  } else {
    emit_operand_to_callee_reg(codegen, lhs, dest);
  }

  std::string rhs_name = "rax";
  std::string rhs_name_32 = "eax";
  if (!rhs_conflicts && rhs_reg.has_value()) {
    rhs_name = phys_reg_name(*rhs_reg);
    rhs_name_32 = phys_reg_name_32(*rhs_reg);
  } else if (!rhs_conflicts) {
    emit_mov_operand_to_reg(codegen, rhs, "rax");
  }

  if (opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    if (use_32bit) {
      codegen.state.emit(std::string("    imull %") + rhs_name_32 + ", %" + dest_name_32);
      emit_sext32_if_needed(codegen, dest, is_unsigned);
    } else {
      codegen.state.emit(std::string("    imulq %") + rhs_name + ", %" + dest_name);
    }
  } else {
    const char* mnemonic = x86_alu_mnemonic(opcode);
    if (use_32bit &&
        (opcode == c4c::backend::bir::BinaryOpcode::Add ||
         opcode == c4c::backend::bir::BinaryOpcode::Sub)) {
      codegen.state.emit(std::string("    ") + mnemonic + "l %" + rhs_name_32 + ", %" +
                         dest_name_32);
      emit_sext32_if_needed(codegen, dest, is_unsigned);
    } else {
      codegen.state.emit(std::string("    ") + mnemonic + "q %" + rhs_name + ", %" + dest_name);
    }
  }

  codegen.state.reg_cache.invalidate_acc();
}

void emit_shift_reg_direct(X86Codegen& codegen,
                           c4c::backend::bir::BinaryOpcode opcode,
                           const Operand& lhs,
                           const Operand& rhs,
                           c4c::backend::PhysReg dest,
                           bool use_32bit,
                           bool is_unsigned) {
  const char* dest_name = phys_reg_name(dest);
  const char* dest_name_32 = phys_reg_name_32(dest);
  const auto rhs_reg = operand_reg(codegen, rhs);
  const bool rhs_conflicts = rhs_reg.has_value() && *rhs_reg == dest;
  const auto [mnem32, mnem64] = x86_shift_mnemonic(opcode);

  if (const auto imm = codegen.const_as_imm32(rhs)) {
    emit_operand_to_callee_reg(codegen, lhs, dest);
    if (use_32bit) {
      const auto shift_amount = static_cast<std::uint32_t>(*imm) & 31U;
      codegen.state.emit(std::string("    ") + mnem32 + " $" + std::to_string(shift_amount) +
                        ", %" + dest_name_32);
      if (!is_unsigned && opcode != c4c::backend::bir::BinaryOpcode::LShr) {
        emit_sext32_if_needed(codegen, dest, false);
      }
    } else {
      const auto shift_amount = static_cast<std::uint64_t>(*imm) & 63ULL;
      codegen.state.emit(std::string("    ") + mnem64 + " $" + std::to_string(shift_amount) +
                        ", %" + dest_name);
    }
    codegen.state.reg_cache.invalidate_acc();
    return;
  }

  if (rhs_conflicts) {
    emit_mov_operand_to_reg(codegen, rhs, "rcx");
    emit_operand_to_callee_reg(codegen, lhs, dest);
  } else {
    emit_operand_to_callee_reg(codegen, lhs, dest);
    emit_mov_operand_to_reg(codegen, rhs, "rcx");
  }

  if (use_32bit) {
    codegen.state.emit(std::string("    ") + mnem32 + " %cl, %" + dest_name_32);
    if (!is_unsigned && opcode != c4c::backend::bir::BinaryOpcode::LShr) {
      emit_sext32_if_needed(codegen, dest, false);
    }
  } else {
    codegen.state.emit(std::string("    ") + mnem64 + " %cl, %" + dest_name);
  }

  codegen.state.reg_cache.invalidate_acc();
}

bool emit_acc_immediate(X86Codegen& codegen,
                        const Value& dest,
                        c4c::backend::bir::BinaryOpcode opcode,
                        const Operand& lhs,
                        const Operand& rhs,
                        bool use_32bit,
                        bool is_unsigned) {
  if (opcode == c4c::backend::bir::BinaryOpcode::Add ||
      opcode == c4c::backend::bir::BinaryOpcode::Sub ||
      opcode == c4c::backend::bir::BinaryOpcode::And ||
      opcode == c4c::backend::bir::BinaryOpcode::Or ||
      opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    if (const auto imm = codegen.const_as_imm32(rhs)) {
      codegen.operand_to_rax(lhs);
      const char* mnemonic = x86_alu_mnemonic(opcode);
      if (use_32bit &&
          (opcode == c4c::backend::bir::BinaryOpcode::Add ||
           opcode == c4c::backend::bir::BinaryOpcode::Sub)) {
        codegen.state.emit(std::string("    ") + mnemonic + "l $" + std::to_string(*imm) + ", %eax");
        if (!is_unsigned) {
          codegen.state.emit("    cltq");
        }
      } else {
        codegen.state.emit(std::string("    ") + mnemonic + "q $" + std::to_string(*imm) + ", %rax");
      }
      codegen.state.reg_cache.invalidate_acc();
      codegen.store_rax_to(dest);
      return true;
    }
  }

  if (opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    if (const auto imm = codegen.const_as_imm32(rhs)) {
      codegen.operand_to_rax(lhs);
      if (const auto scale = lea_scale_for_mul(*imm)) {
        if (use_32bit) {
          codegen.state.emit(std::string("    leal (%eax, %eax, ") + std::to_string(*scale) +
                            "), %eax");
          if (!is_unsigned) {
            codegen.state.emit("    cltq");
          }
        } else {
          codegen.state.emit(std::string("    leaq (%rax, %rax, ") + std::to_string(*scale) +
                            "), %rax");
        }
      } else if (use_32bit) {
        codegen.state.emit(std::string("    imull $") + std::to_string(*imm) + ", %eax, %eax");
        if (!is_unsigned) {
          codegen.state.emit("    cltq");
        }
      } else {
        codegen.state.emit(std::string("    imulq $") + std::to_string(*imm) + ", %rax, %rax");
      }
      codegen.state.reg_cache.invalidate_acc();
      codegen.store_rax_to(dest);
      return true;
    }
  }

  if (opcode == c4c::backend::bir::BinaryOpcode::Shl ||
      opcode == c4c::backend::bir::BinaryOpcode::AShr ||
      opcode == c4c::backend::bir::BinaryOpcode::LShr) {
    if (const auto imm = codegen.const_as_imm32(rhs)) {
      codegen.operand_to_rax(lhs);
      const auto [mnem32, mnem64] = x86_shift_mnemonic(opcode);
      if (use_32bit) {
        const auto shift_amount = static_cast<std::uint32_t>(*imm) & 31U;
        codegen.state.emit(std::string("    ") + mnem32 + " $" + std::to_string(shift_amount) +
                          ", %eax");
        if (!is_unsigned && opcode != c4c::backend::bir::BinaryOpcode::LShr) {
          codegen.state.emit("    cltq");
        }
      } else {
        const auto shift_amount = static_cast<std::uint64_t>(*imm) & 63ULL;
        codegen.state.emit(std::string("    ") + mnem64 + " $" + std::to_string(shift_amount) +
                          ", %rax");
      }
      codegen.state.reg_cache.invalidate_acc();
      codegen.store_rax_to(dest);
      return true;
    }
  }

  return false;
}

}  // namespace

void X86Codegen::emit_float_neg_impl(IrType ty) {
  if (ty == IrType::F32) {
    this->state.emit("    movd %eax, %xmm0");
    this->state.emit("    movl $0x80000000, %ecx");
    this->state.emit("    movd %ecx, %xmm1");
    this->state.emit("    xorps %xmm1, %xmm0");
    this->state.emit("    movd %xmm0, %eax");
  } else {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movabsq $-9223372036854775808, %rcx");
    this->state.emit("    movq %rcx, %xmm1");
    this->state.emit("    xorpd %xmm1, %xmm0");
    this->state.emit("    movq %xmm0, %rax");
  }
}

void X86Codegen::emit_int_neg_impl(IrType /*ty*/) { this->state.emit("    negq %rax"); }
void X86Codegen::emit_int_not_impl(IrType /*ty*/) { this->state.emit("    notq %rax"); }

void X86Codegen::emit_int_clz_impl(IrType ty) {
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    lzcntl %eax, %eax"
                                                           : "    lzcntq %rax, %rax");
}

void X86Codegen::emit_int_ctz_impl(IrType ty) {
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    tzcntl %eax, %eax"
                                                           : "    tzcntq %rax, %rax");
}

void X86Codegen::emit_int_bswap_impl(IrType ty) {
  if (ty == IrType::I16 || ty == IrType::U16) {
    this->state.emit("    rolw $8, %ax");
  } else if (ty == IrType::I32 || ty == IrType::U32) {
    this->state.emit("    bswapl %eax");
  } else {
    this->state.emit("    bswapq %rax");
  }
}

void X86Codegen::emit_int_popcount_impl(IrType ty) {
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    popcntl %eax, %eax"
                                                           : "    popcntq %rax, %rax");
}

void X86Codegen::emit_int_binop_impl(const Value& dest,
                                     unsigned op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty) {
  const auto opcode = static_cast<c4c::backend::bir::BinaryOpcode>(op);
  const bool use_32bit = is_32bit_int_type(ty);
  const bool is_unsigned = is_unsigned_int_type(ty);

  if (const auto direct_dest = dest_reg(*this, dest)) {
    const bool is_simple_alu = opcode == c4c::backend::bir::BinaryOpcode::Add ||
                               opcode == c4c::backend::bir::BinaryOpcode::Sub ||
                               opcode == c4c::backend::bir::BinaryOpcode::And ||
                               opcode == c4c::backend::bir::BinaryOpcode::Or ||
                               opcode == c4c::backend::bir::BinaryOpcode::Xor ||
                               opcode == c4c::backend::bir::BinaryOpcode::Mul;
    if (is_simple_alu) {
      emit_simple_alu_reg_direct(*this, opcode, lhs, rhs, *direct_dest, use_32bit, is_unsigned);
      return;
    }
    if (opcode == c4c::backend::bir::BinaryOpcode::Shl ||
        opcode == c4c::backend::bir::BinaryOpcode::AShr ||
        opcode == c4c::backend::bir::BinaryOpcode::LShr) {
      emit_shift_reg_direct(*this, opcode, lhs, rhs, *direct_dest, use_32bit, is_unsigned);
      return;
    }
  }

  if (emit_acc_immediate(*this, dest, opcode, lhs, rhs, use_32bit, is_unsigned)) {
    return;
  }

  this->operand_to_rax(lhs);
  this->operand_to_rcx(rhs);

  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Sub:
    case c4c::backend::bir::BinaryOpcode::Mul:
      if (use_32bit) {
        const char* mnem = opcode == c4c::backend::bir::BinaryOpcode::Mul ? "imul"
                                                                          : x86_alu_mnemonic(opcode);
        this->state.emit(std::string("    ") + mnem + "l %ecx, %eax");
        if (!is_unsigned) {
          this->state.emit("    cltq");
        }
      } else {
        const char* mnem = opcode == c4c::backend::bir::BinaryOpcode::Mul ? "imul"
                                                                          : x86_alu_mnemonic(opcode);
        this->state.emit(std::string("    ") + mnem + "q %rcx, %rax");
      }
      break;
    case c4c::backend::bir::BinaryOpcode::SDiv:
      if (use_32bit) {
        this->state.emit("    cltd");
        this->state.emit("    idivl %ecx");
        this->state.emit("    cltq");
      } else {
        this->state.emit("    cqto");
        this->state.emit("    idivq %rcx");
      }
      break;
    case c4c::backend::bir::BinaryOpcode::UDiv:
      this->state.emit("    xorl %edx, %edx");
      this->state.emit(use_32bit ? "    divl %ecx" : "    divq %rcx");
      break;
    case c4c::backend::bir::BinaryOpcode::SRem:
      if (use_32bit) {
        this->state.emit("    cltd");
        this->state.emit("    idivl %ecx");
        this->state.emit("    movl %edx, %eax");
        this->state.emit("    cltq");
      } else {
        this->state.emit("    cqto");
        this->state.emit("    idivq %rcx");
        this->state.emit("    movq %rdx, %rax");
      }
      break;
    case c4c::backend::bir::BinaryOpcode::URem:
      this->state.emit("    xorl %edx, %edx");
      if (use_32bit) {
        this->state.emit("    divl %ecx");
        this->state.emit("    movl %edx, %eax");
      } else {
        this->state.emit("    divq %rcx");
        this->state.emit("    movq %rdx, %rax");
      }
      break;
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
      this->state.emit(std::string("    ") + x86_alu_mnemonic(opcode) + "q %rcx, %rax");
      break;
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::LShr: {
      const auto [mnem32, mnem64] = x86_shift_mnemonic(opcode);
      if (use_32bit) {
        this->state.emit(std::string("    ") + mnem32 + " %cl, %eax");
        if (!is_unsigned && opcode != c4c::backend::bir::BinaryOpcode::LShr) {
          this->state.emit("    cltq");
        }
      } else {
        this->state.emit(std::string("    ") + mnem64 + " %cl, %rax");
      }
      break;
    }
  }

  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

void X86Codegen::emit_copy_i128_impl(const Value& dest, const Operand& src) {
  this->operand_to_rax_rdx(src);
  this->store_rax_rdx_to(dest);
}

}  // namespace c4c::backend::x86
