#include "atomics_intrinsics_lowering.hpp"

#include <limits>
#include <stdexcept>
#include <string>

namespace c4c::backend::x86 {

namespace {

Operand value_as_operand(const Value& value) {
  Operand operand;
  operand.raw = value.raw;
  return operand;
}

void load_dest_ptr_to_reg(X86Codegen& codegen, const Value& dest_ptr, const char* reg) {
  codegen.operand_to_reg(value_as_operand(dest_ptr), reg);
}

std::int64_t operand_to_imm_i64(const Operand& operand) {
  return operand.immediate.value_or(0);
}

const char* atomic_load_dest_reg(IrType ty) {
  switch (ty) {
    case IrType::U32:
    case IrType::F32:
      return "%eax";
    default:
      return "%rax";
  }
}

const char* sse_binary_mnemonic(IntrinsicOp intrinsic) {
  switch (intrinsic) {
    case IntrinsicOp::Pcmpeqb128: return "pcmpeqb";
    case IntrinsicOp::Pcmpeqd128: return "pcmpeqd";
    case IntrinsicOp::Psubusb128: return "psubusb";
    case IntrinsicOp::Psubsb128: return "psubsb";
    case IntrinsicOp::Por128: return "por";
    case IntrinsicOp::Pand128: return "pand";
    case IntrinsicOp::Pxor128: return "pxor";
    case IntrinsicOp::Aesenc128: return "aesenc";
    case IntrinsicOp::Aesenclast128: return "aesenclast";
    case IntrinsicOp::Aesdec128: return "aesdec";
    case IntrinsicOp::Aesdeclast128: return "aesdeclast";
    case IntrinsicOp::Paddw128: return "paddw";
    case IntrinsicOp::Psubw128: return "psubw";
    case IntrinsicOp::Pmulhw128: return "pmulhw";
    case IntrinsicOp::Pmaddwd128: return "pmaddwd";
    case IntrinsicOp::Pcmpgtw128: return "pcmpgtw";
    case IntrinsicOp::Pcmpgtb128: return "pcmpgtb";
    case IntrinsicOp::Paddd128: return "paddd";
    case IntrinsicOp::Psubd128: return "psubd";
    case IntrinsicOp::Packssdw128: return "packssdw";
    case IntrinsicOp::Packsswb128: return "packsswb";
    case IntrinsicOp::Packuswb128: return "packuswb";
    case IntrinsicOp::Punpcklbw128: return "punpcklbw";
    case IntrinsicOp::Punpckhbw128: return "punpckhbw";
    case IntrinsicOp::Punpcklwd128: return "punpcklwd";
    case IntrinsicOp::Punpckhwd128: return "punpckhwd";
    default: throw std::logic_error("unexpected x86 intrinsic binary mnemonic request");
  }
}

const char* sse_unary_imm_mnemonic(IntrinsicOp intrinsic) {
  switch (intrinsic) {
    case IntrinsicOp::Pslldqi128: return "pslldq";
    case IntrinsicOp::Psrldqi128: return "psrldq";
    case IntrinsicOp::Psllqi128: return "psllq";
    case IntrinsicOp::Psrlqi128: return "psrlq";
    case IntrinsicOp::Psllwi128: return "psllw";
    case IntrinsicOp::Psrlwi128: return "psrlw";
    case IntrinsicOp::Psrawi128: return "psraw";
    case IntrinsicOp::Psradi128: return "psrad";
    case IntrinsicOp::Pslldi128: return "pslld";
    case IntrinsicOp::Psrldi128: return "psrld";
    default: throw std::logic_error("unexpected x86 intrinsic unary-imm mnemonic request");
  }
}

const char* sse_shuffle_mnemonic(IntrinsicOp intrinsic) {
  switch (intrinsic) {
    case IntrinsicOp::Pshufd128: return "pshufd";
    case IntrinsicOp::Pshuflw128: return "pshuflw";
    case IntrinsicOp::Pshufhw128: return "pshufhw";
    default: throw std::logic_error("unexpected x86 intrinsic shuffle mnemonic request");
  }
}

}  // namespace

void X86Codegen::emit_atomic_rmw_impl(const Value& dest,
                                      AtomicRmwOp op,
                                      const Operand& ptr,
                                      const Operand& val,
                                      IrType ty,
                                      AtomicOrdering /*ordering*/) {
  this->operand_to_rax(ptr);
  this->state.emit("    movq %rax, %rcx");
  this->operand_to_rax(val);
  this->state.reg_cache.invalidate_all();

  const auto size_suffix = this->type_suffix(ty);
  const auto val_reg = this->reg_for_type("rax", ty);
  switch (op) {
    case AtomicRmwOp::Add:
      this->state.emit(std::string("    lock xadd") + size_suffix + " %" + val_reg + ", (%rcx)");
      break;
    case AtomicRmwOp::Xchg:
      this->state.emit(std::string("    xchg") + size_suffix + " %" + val_reg + ", (%rcx)");
      break;
    case AtomicRmwOp::TestAndSet:
      this->state.emit("    movb $1, %al");
      this->state.emit("    xchgb %al, (%rcx)");
      this->state.emit("    movzbl %al, %eax");
      break;
    case AtomicRmwOp::Sub:
      this->emit_x86_atomic_op_loop(ty, "sub");
      break;
    case AtomicRmwOp::And:
      this->emit_x86_atomic_op_loop(ty, "and");
      break;
    case AtomicRmwOp::Or:
      this->emit_x86_atomic_op_loop(ty, "or");
      break;
    case AtomicRmwOp::Xor:
      this->emit_x86_atomic_op_loop(ty, "xor");
      break;
    case AtomicRmwOp::Nand:
      this->emit_x86_atomic_op_loop(ty, "nand");
      break;
  }

  this->store_rax_to(dest);
}

void X86Codegen::emit_atomic_cmpxchg_impl(const Value& dest,
                                          const Operand& ptr,
                                          const Operand& expected,
                                          const Operand& desired,
                                          IrType ty,
                                          AtomicOrdering /*success_ordering*/,
                                          AtomicOrdering /*failure_ordering*/,
                                          bool returns_bool) {
  this->operand_to_rax(ptr);
  this->state.emit("    movq %rax, %rcx");
  this->operand_to_rax(desired);
  this->state.emit("    movq %rax, %rdx");
  this->operand_to_rax(expected);
  this->state.reg_cache.invalidate_all();
  const auto size_suffix = this->type_suffix(ty);
  const auto desired_reg = this->reg_for_type("rdx", ty);
  this->state.emit(std::string("    lock cmpxchg") + size_suffix + " %" + desired_reg +
                   ", (%rcx)");
  if (returns_bool) {
    this->state.emit("    sete %al");
    this->state.emit("    movzbl %al, %eax");
  }
  this->store_rax_to(dest);
}

void X86Codegen::emit_atomic_load_impl(const Value& dest,
                                       const Operand& ptr,
                                       IrType ty,
                                       AtomicOrdering /*ordering*/) {
  this->operand_to_rax(ptr);
  this->state.reg_cache.invalidate_all();
  const auto load_instr = this->mov_load_for_type(ty);
  this->state.emit(std::string("    ") + load_instr + " (%rax), " + atomic_load_dest_reg(ty));
  this->store_rax_to(dest);
}

void X86Codegen::emit_atomic_store_impl(const Operand& ptr,
                                        const Operand& val,
                                        IrType ty,
                                        AtomicOrdering ordering) {
  this->operand_to_rax(val);
  this->state.emit("    movq %rax, %rdx");
  this->operand_to_rax(ptr);
  this->state.reg_cache.invalidate_all();
  const auto store_reg = this->reg_for_type("rdx", ty);
  const auto store_instr = this->mov_store_for_type(ty);
  this->state.emit(std::string("    ") + store_instr + " %" + store_reg + ", (%rax)");
  if (ordering == AtomicOrdering::SeqCst) {
    this->state.emit("    mfence");
  }
}

void X86Codegen::emit_fence_impl(AtomicOrdering ordering) {
  if (ordering != AtomicOrdering::Relaxed) {
    this->state.emit("    mfence");
  }
}

void float_operand_to_xmm0(X86Codegen& codegen, const Operand& op, bool is_f32) {
  if (const auto imm = op.immediate) {
    if (*imm == 0) {
      codegen.state.emit(is_f32 ? "    xorps %xmm0, %xmm0" : "    xorpd %xmm0, %xmm0");
      return;
    }

    if (is_f32) {
      codegen.state.out.emit_instr_imm_reg("    movl", *imm, "eax");
      codegen.state.emit("    movd %eax, %xmm0");
      return;
    }

    if (*imm >= static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) &&
        *imm <= static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
      codegen.state.out.emit_instr_imm_reg("    movq", *imm, "rax");
    } else {
      codegen.state.out.emit_instr_imm_reg("    movabsq", *imm, "rax");
    }
    codegen.state.emit("    movq %rax, %xmm0");
    return;
  }

  codegen.operand_to_reg(op, "rax");
  codegen.state.emit(is_f32 ? "    movd %eax, %xmm0" : "    movq %rax, %xmm0");
}

void emit_nontemporal_store(X86Codegen& codegen,
                            const IntrinsicOp& op,
                            const Operand& ptr,
                            const Operand& val,
                            std::optional<Value> dest) {
  (void)dest;
  codegen.operand_to_reg(ptr, "rax");
  switch (op) {
    case IntrinsicOp::Movnti:
      codegen.operand_to_reg(val, "rcx");
      codegen.state.emit("    movnti %ecx, (%rax)");
      return;
    case IntrinsicOp::Movnti64:
      codegen.operand_to_reg(val, "rcx");
      codegen.state.emit("    movnti %rcx, (%rax)");
      return;
    case IntrinsicOp::Movntdq:
      codegen.operand_to_reg(val, "rcx");
      codegen.state.emit("    movdqu (%rcx), %xmm0");
      codegen.state.emit("    movntdq %xmm0, (%rax)");
      return;
    case IntrinsicOp::Movntpd:
      codegen.operand_to_reg(val, "rcx");
      codegen.state.emit("    movupd (%rcx), %xmm0");
      codegen.state.emit("    movntpd %xmm0, (%rax)");
      return;
    default:
      return;
  }
}

void emit_sse_binary_128(X86Codegen& codegen,
                         const Value& dest_ptr,
                         const Operand& lhs,
                         const Operand& rhs,
                         const char* mnemonic) {
  codegen.operand_to_reg(lhs, "rax");
  codegen.state.emit("    movdqu (%rax), %xmm0");
  codegen.operand_to_reg(rhs, "rcx");
  codegen.state.emit("    movdqu (%rcx), %xmm1");
  codegen.state.emit(std::string("    ") + mnemonic + " %xmm1, %xmm0");
  load_dest_ptr_to_reg(codegen, dest_ptr, "rax");
  codegen.state.emit("    movdqu %xmm0, (%rax)");
}

void emit_sse_unary_imm_128(X86Codegen& codegen,
                            const Value& dest_ptr,
                            const Operand& src,
                            std::uint8_t imm,
                            const char* mnemonic) {
  codegen.operand_to_reg(src, "rax");
  codegen.state.emit("    movdqu (%rax), %xmm0");
  codegen.state.emit(std::string("    ") + mnemonic + " $" + std::to_string(imm) + ", %xmm0");
  load_dest_ptr_to_reg(codegen, dest_ptr, "rax");
  codegen.state.emit("    movdqu %xmm0, (%rax)");
}

void emit_sse_shuffle_imm_128(X86Codegen& codegen,
                              const Value& dest_ptr,
                              const Operand& lhs,
                              const Operand& rhs,
                              std::uint8_t imm,
                              const char* mnemonic) {
  (void)rhs;
  codegen.operand_to_reg(lhs, "rax");
  codegen.state.emit("    movdqu (%rax), %xmm0");
  codegen.state.emit(std::string("    ") + mnemonic + " $" + std::to_string(imm) +
                     ", %xmm0, %xmm0");
  load_dest_ptr_to_reg(codegen, dest_ptr, "rax");
  codegen.state.emit("    movdqu %xmm0, (%rax)");
}

void X86Codegen::emit_intrinsic_impl(const std::optional<Value>& dest,
                                     const IntrinsicOp& intrinsic,
                                     const std::vector<Operand>& args) {
  switch (intrinsic) {
    case IntrinsicOp::Lfence:
      this->state.emit("    lfence");
      return;
    case IntrinsicOp::Mfence:
      this->state.emit("    mfence");
      return;
    case IntrinsicOp::Sfence:
      this->state.emit("    sfence");
      return;
    case IntrinsicOp::Pause:
      this->state.emit("    pause");
      return;
    case IntrinsicOp::Clflush:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    clflush (%rax)");
      return;
    case IntrinsicOp::Movnti:
    case IntrinsicOp::Movnti64:
    case IntrinsicOp::Movntdq:
    case IntrinsicOp::Movntpd:
      if (dest.has_value()) {
        emit_nontemporal_store(*this, intrinsic, value_as_operand(*dest), args[0], dest);
      }
      return;
    case IntrinsicOp::Loaddqu:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Storedqu:
      if (dest.has_value()) {
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->operand_to_reg(args[0], "rcx");
        this->state.emit("    movdqu (%rcx), %xmm0");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pcmpeqb128:
    case IntrinsicOp::Pcmpeqd128:
    case IntrinsicOp::Psubusb128:
    case IntrinsicOp::Psubsb128:
    case IntrinsicOp::Por128:
    case IntrinsicOp::Pand128:
    case IntrinsicOp::Pxor128:
    case IntrinsicOp::Aesenc128:
    case IntrinsicOp::Aesenclast128:
    case IntrinsicOp::Aesdec128:
    case IntrinsicOp::Aesdeclast128:
    case IntrinsicOp::Paddw128:
    case IntrinsicOp::Psubw128:
    case IntrinsicOp::Pmulhw128:
    case IntrinsicOp::Pmaddwd128:
    case IntrinsicOp::Pcmpgtw128:
    case IntrinsicOp::Pcmpgtb128:
    case IntrinsicOp::Paddd128:
    case IntrinsicOp::Psubd128:
    case IntrinsicOp::Packssdw128:
    case IntrinsicOp::Packsswb128:
    case IntrinsicOp::Packuswb128:
    case IntrinsicOp::Punpcklbw128:
    case IntrinsicOp::Punpckhbw128:
    case IntrinsicOp::Punpcklwd128:
    case IntrinsicOp::Punpckhwd128:
      if (dest.has_value()) {
        emit_sse_binary_128(*this, *dest, args[0], args[1], sse_binary_mnemonic(intrinsic));
      }
      return;
    case IntrinsicOp::Pmovmskb128:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    pmovmskb %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::SetEpi8:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movd %eax, %xmm0");
        this->state.emit("    punpcklbw %xmm0, %xmm0");
        this->state.emit("    punpcklwd %xmm0, %xmm0");
        this->state.emit("    pshufd $0, %xmm0, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::SetEpi32:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movd %eax, %xmm0");
        this->state.emit("    pshufd $0, %xmm0, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Crc32_8:
    case IntrinsicOp::Crc32_16:
    case IntrinsicOp::Crc32_32:
    case IntrinsicOp::Crc32_64:
      this->operand_to_reg(args[0], "rax");
      this->operand_to_reg(args[1], "rcx");
      switch (intrinsic) {
        case IntrinsicOp::Crc32_8:
          this->state.emit("    crc32b %cl, %eax");
          break;
        case IntrinsicOp::Crc32_16:
          this->state.emit("    crc32w %cx, %eax");
          break;
        case IntrinsicOp::Crc32_32:
          this->state.emit("    crc32l %ecx, %eax");
          break;
        case IntrinsicOp::Crc32_64:
          this->state.emit("    crc32q %rcx, %rax");
          break;
        default:
          break;
      }
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::FrameAddress:
      this->state.emit("    movq %rbp, %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::ReturnAddress:
      this->state.emit("    movq 8(%rbp), %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::ThreadPointer:
      this->state.emit("    movq %fs:0, %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::SqrtF64:
      float_operand_to_xmm0(*this, args[0], false);
      this->state.emit("    sqrtsd %xmm0, %xmm0");
      this->state.emit("    movq %xmm0, %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::SqrtF32:
      float_operand_to_xmm0(*this, args[0], true);
      this->state.emit("    sqrtss %xmm0, %xmm0");
      this->state.emit("    movd %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::FabsF64:
      float_operand_to_xmm0(*this, args[0], false);
      this->state.emit("    movabsq $0x7FFFFFFFFFFFFFFF, %rcx");
      this->state.emit("    movq %rcx, %xmm1");
      this->state.emit("    andpd %xmm1, %xmm0");
      this->state.emit("    movq %xmm0, %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::FabsF32:
      float_operand_to_xmm0(*this, args[0], true);
      this->state.emit("    movl $0x7FFFFFFF, %ecx");
      this->state.emit("    movd %ecx, %xmm1");
      this->state.emit("    andps %xmm1, %xmm0");
      this->state.emit("    movd %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::Aesimc128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->state.emit("    aesimc %xmm0, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Aeskeygenassist128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->state.emit("    aeskeygenassist $" + std::to_string(operand_to_imm_i64(args[1])) +
                         ", %xmm0, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pclmulqdq128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->operand_to_reg(args[1], "rcx");
        this->state.emit("    movdqu (%rcx), %xmm1");
        this->state.emit("    pclmulqdq $" + std::to_string(operand_to_imm_i64(args[2])) +
                         ", %xmm1, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pslldqi128:
    case IntrinsicOp::Psrldqi128:
    case IntrinsicOp::Psllqi128:
    case IntrinsicOp::Psrlqi128:
    case IntrinsicOp::Psllwi128:
    case IntrinsicOp::Psrlwi128:
    case IntrinsicOp::Psrawi128:
    case IntrinsicOp::Psradi128:
    case IntrinsicOp::Pslldi128:
    case IntrinsicOp::Psrldi128:
      if (dest.has_value()) {
        emit_sse_unary_imm_128(*this, *dest, args[0],
                               static_cast<std::uint8_t>(operand_to_imm_i64(args[1])),
                               sse_unary_imm_mnemonic(intrinsic));
      }
      return;
    case IntrinsicOp::Pshufd128:
    case IntrinsicOp::Pshuflw128:
    case IntrinsicOp::Pshufhw128:
      if (dest.has_value()) {
        emit_sse_shuffle_imm_128(*this, *dest, args[0], Operand::zero(),
                                 static_cast<std::uint8_t>(operand_to_imm_i64(args[1])),
                                 sse_shuffle_mnemonic(intrinsic));
      }
      return;
    case IntrinsicOp::Loadldi128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movq (%rax), %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::SetEpi16:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movd %eax, %xmm0");
        this->state.emit("    punpcklwd %xmm0, %xmm0");
        this->state.emit("    pshufd $0, %xmm0, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pinsrw128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->operand_to_reg(args[1], "rcx");
        this->state.emit("    pinsrw $" + std::to_string(operand_to_imm_i64(args[2])) +
                         ", %ecx, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pextrw128:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    pextrw $" + std::to_string(operand_to_imm_i64(args[1])) +
                       ", %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::Pinsrd128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->operand_to_reg(args[1], "rcx");
        this->state.emit("    pinsrd $" + std::to_string(operand_to_imm_i64(args[2])) +
                         ", %ecx, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pextrd128:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    pextrd $" + std::to_string(operand_to_imm_i64(args[1])) +
                       ", %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::Pinsrb128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->operand_to_reg(args[1], "rcx");
        this->state.emit("    pinsrb $" + std::to_string(operand_to_imm_i64(args[2])) +
                         ", %ecx, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pextrb128:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    pextrb $" + std::to_string(operand_to_imm_i64(args[1])) +
                       ", %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::Pinsrq128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movdqu (%rax), %xmm0");
        this->operand_to_reg(args[1], "rcx");
        this->state.emit("    pinsrq $" + std::to_string(operand_to_imm_i64(args[2])) +
                         ", %rcx, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Pextrq128:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    pextrq $" + std::to_string(operand_to_imm_i64(args[1])) +
                       ", %xmm0, %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::Storeldi128:
      if (dest.has_value()) {
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->operand_to_reg(args[0], "rcx");
        this->state.emit("    movdqu (%rcx), %xmm0");
        this->state.emit("    movq %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Cvtsi128Si32:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    movd %xmm0, %eax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
    case IntrinsicOp::Cvtsi32Si128:
      if (dest.has_value()) {
        this->operand_to_reg(args[0], "rax");
        this->state.emit("    movd %eax, %xmm0");
        load_dest_ptr_to_reg(*this, *dest, "rax");
        this->state.emit("    movdqu %xmm0, (%rax)");
      }
      return;
    case IntrinsicOp::Cvtsi128Si64:
      this->operand_to_reg(args[0], "rax");
      this->state.emit("    movdqu (%rax), %xmm0");
      this->state.emit("    movq %xmm0, %rax");
      if (dest.has_value()) {
        this->store_rax_to(*dest);
      }
      return;
  }
}

}  // namespace c4c::backend::x86
