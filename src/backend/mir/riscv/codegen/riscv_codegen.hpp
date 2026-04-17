#pragma once

#include "../../f128_softfloat.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::codegen {

struct PhysReg {
  std::uint32_t value = 0;

  constexpr PhysReg() = default;
  constexpr explicit PhysReg(std::uint32_t v) : value(v) {}
};

struct StackSlot {
  std::int64_t raw = 0;
};

struct SlotAddr {
  enum class Kind : unsigned char {
    Direct,
    Indirect,
    OverAligned,
  };

  Kind kind = Kind::Direct;
  StackSlot slot{};
  std::uint32_t value_id = 0;

  static SlotAddr Direct(StackSlot direct_slot) {
    return SlotAddr{
        .kind = Kind::Direct,
        .slot = direct_slot,
    };
  }

  static SlotAddr Indirect(StackSlot indirect_slot) {
    return SlotAddr{
        .kind = Kind::Indirect,
        .slot = indirect_slot,
    };
  }

  static SlotAddr OverAligned(StackSlot over_aligned_slot, std::uint32_t over_aligned_value_id) {
    return SlotAddr{
        .kind = Kind::OverAligned,
        .slot = over_aligned_slot,
        .value_id = over_aligned_value_id,
    };
  }
};

struct Value {
  int raw = 0;
};

enum class IrType : unsigned {
  Void,
  I8,
  I16,
  I32,
  I64,
  I128,
  U16,
  U32,
  U64,
  Ptr,
  F32,
  F64,
  F128,
};

struct IrFunction {
  bool is_variadic = false;
  IrType return_type = IrType::Void;
  std::size_t va_named_gp_count = 0;
  std::size_t va_named_stack_bytes = 0;
};

enum class FloatOp : unsigned {
  Add,
  Sub,
  Mul,
  Div,
};

enum class IrBinOp : unsigned {
  Add,
  Sub,
  Mul,
  SDiv,
  UDiv,
  SRem,
  URem,
  And,
  Or,
  Xor,
  Shl,
  AShr,
  LShr,
};

enum class IrCmpOp : unsigned {
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

enum class AtomicOrdering : unsigned {
  Relaxed,
  Acquire,
  Release,
  AcqRel,
  SeqCst,
};

enum class AtomicRmwOp : unsigned {
  Add,
  Sub,
  And,
  Or,
  Xor,
  Nand,
  Xchg,
  TestAndSet,
};

enum class IntrinsicOp : std::uint16_t {
  Lfence,
  Mfence,
  Sfence,
  Pause,
  Clflush,
  Movnti,
  Movnti64,
  Movntdq,
  Movntpd,
  Loaddqu,
  Storedqu,
  Pcmpeqb128,
  Pcmpeqd128,
  Psubusb128,
  Psubsb128,
  Por128,
  Pand128,
  Pxor128,
  Pmovmskb128,
  SetEpi8,
  SetEpi32,
  Crc32_8,
  Crc32_16,
  Crc32_32,
  Crc32_64,
  FrameAddress,
  ReturnAddress,
  ThreadPointer,
  SqrtF64,
  SqrtF32,
  FabsF64,
  FabsF32,
  Aesenc128,
  Aesenclast128,
  Aesdec128,
  Aesdeclast128,
  Aesimc128,
  Aeskeygenassist128,
  Pclmulqdq128,
  Pslldqi128,
  Psrldqi128,
  Psllqi128,
  Psrlqi128,
  Pshufd128,
  Loadldi128,
  Paddw128,
  Psubw128,
  Pmulhw128,
  Pmaddwd128,
  Pcmpgtw128,
  Pcmpgtb128,
  Paddd128,
  Psubd128,
  Packssdw128,
  Packsswb128,
  Packuswb128,
  Punpcklbw128,
  Punpckhbw128,
  Punpcklwd128,
  Punpckhwd128,
  Psllwi128,
  Psrlwi128,
  Psrawi128,
  Psradi128,
  Pslldi128,
  Psrldi128,
  SetEpi16,
  Pinsrw128,
  Pextrw128,
  Pinsrd128,
  Pextrd128,
  Pinsrb128,
  Pextrb128,
  Pinsrq128,
  Pextrq128,
  Storeldi128,
  Cvtsi128Si32,
  Cvtsi32Si128,
  Cvtsi128Si64,
  Pshuflw128,
  Pshufhw128,
};

struct RiscvFloatClass {
  struct OneFloat {
    bool is_double = false;
  };
  struct TwoFloats {
    bool lo_is_double = false;
    bool hi_is_double = false;
  };
  struct FloatAndInt {
    bool float_is_double = false;
    std::size_t int_offset = 0;
    std::size_t int_size = 0;
  };
  struct IntAndFloat {
    bool float_is_double = false;
    std::size_t int_size = 0;
    std::size_t float_offset = 0;
  };

  using Variant = std::variant<std::monostate, OneFloat, TwoFloats, FloatAndInt, IntAndFloat>;

  Variant value;

  static RiscvFloatClass one_float(bool is_double) { return RiscvFloatClass{OneFloat{is_double}}; }

  static RiscvFloatClass two_floats(bool lo_is_double, bool hi_is_double) {
    return RiscvFloatClass{TwoFloats{lo_is_double, hi_is_double}};
  }

  static RiscvFloatClass float_and_int(bool float_is_double, std::size_t int_offset,
                                       std::size_t int_size) {
    return RiscvFloatClass{FloatAndInt{float_is_double, int_offset, int_size}};
  }

  static RiscvFloatClass int_and_float(bool float_is_double, std::size_t int_size,
                                       std::size_t float_offset) {
    return RiscvFloatClass{IntAndFloat{float_is_double, int_size, float_offset}};
  }
};

struct CallAbiConfig {
  std::size_t max_int_regs = 0;
  std::size_t max_float_regs = 0;
  bool align_i128_pairs = false;
  bool f128_in_fp_regs = false;
  bool f128_in_gp_pairs = false;
  bool variadic_floats_in_gp = false;
  bool large_struct_by_ref = false;
  bool use_sysv_struct_classification = false;
  bool use_riscv_float_struct_classification = false;
  bool allow_struct_split_reg_stack = false;
  bool align_struct_pairs = false;
  bool sret_uses_dedicated_reg = false;
};

struct CallArgClass {
  struct IntReg {
    std::size_t reg_idx = 0;
  };
  struct FloatReg {
    std::size_t reg_idx = 0;
  };
  struct I128RegPair {
    std::size_t base_reg_idx = 0;
  };
  struct F128Reg {
    std::size_t reg_idx = 0;
  };
  struct StructByValReg {
    std::size_t base_reg_idx = 0;
    std::size_t size = 0;
  };
  struct StructSseReg {
    std::size_t lo_fp_idx = 0;
    std::optional<std::size_t> hi_fp_idx;
    std::size_t size = 0;
  };
  struct StructMixedIntSseReg {
    std::size_t int_reg_idx = 0;
    std::size_t fp_reg_idx = 0;
    std::size_t size = 0;
  };
  struct StructMixedSseIntReg {
    std::size_t fp_reg_idx = 0;
    std::size_t int_reg_idx = 0;
    std::size_t size = 0;
  };
  struct StructByValStack {
    std::size_t size = 0;
  };
  struct StructSplitRegStack {
    std::size_t reg_idx = 0;
    std::size_t size = 0;
  };
  struct LargeStructStack {
    std::size_t size = 0;
  };
  struct Stack {};
  struct F128Stack {};
  struct I128Stack {};
  struct ZeroSizeSkip {};

  using Variant = std::variant<IntReg,
                               FloatReg,
                               I128RegPair,
                               F128Reg,
                               StructByValReg,
                               StructSseReg,
                               StructMixedIntSseReg,
                               StructMixedSseIntReg,
                               StructByValStack,
                               StructSplitRegStack,
                               LargeStructStack,
                               Stack,
                               F128Stack,
                               I128Stack,
                               ZeroSizeSkip>;

  Variant value;

  bool is_stack() const {
    return std::visit(
        [](const auto& v) -> bool {
          using T = std::decay_t<decltype(v)>;
          return std::is_same_v<T, Stack> || std::is_same_v<T, F128Stack> ||
                 std::is_same_v<T, I128Stack> || std::is_same_v<T, StructByValStack> ||
                 std::is_same_v<T, LargeStructStack> ||
                 std::is_same_v<T, StructSplitRegStack>;
        },
        value);
  }

  std::size_t stack_bytes() const {
    constexpr std::size_t slot_size = 8;
    constexpr std::size_t align_mask = slot_size - 1;
    return std::visit(
        [align_mask](const auto& v) -> std::size_t {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, F128Stack> || std::is_same_v<T, I128Stack>) {
            return 16;
          } else if constexpr (std::is_same_v<T, StructByValStack> ||
                               std::is_same_v<T, LargeStructStack>) {
            return (v.size + align_mask) & ~align_mask;
          } else if constexpr (std::is_same_v<T, StructSplitRegStack>) {
            const auto stack_part = v.size > slot_size ? v.size - slot_size : 0;
            return (stack_part + align_mask) & ~align_mask;
          } else if constexpr (std::is_same_v<T, Stack>) {
            return slot_size;
          } else {
            return 0;
          }
        },
        value);
  }
};

struct Operand {
  enum class Kind {
    Value,
    ImmI64,
    F32Const,
    F64Const,
  };

  Kind kind = Kind::Value;
  int raw = 0;
  std::int64_t imm_i64 = 0;
  float f32 = 0.0f;
  double f64 = 0.0;

  constexpr Operand() = default;
  constexpr explicit Operand(int value_id) : kind(Kind::Value), raw(value_id) {}

  static Operand immediate_i64(std::int64_t value) {
    Operand operand;
    operand.kind = Kind::ImmI64;
    operand.imm_i64 = value;
    return operand;
  }

  static Operand f32_const(float value) {
    Operand operand;
    operand.kind = Kind::F32Const;
    operand.f32 = value;
    return operand;
  }

  static Operand f64_const(double value) {
    Operand operand;
    operand.kind = Kind::F64Const;
    operand.f64 = value;
    return operand;
  }
};

class RiscvCodegenState {
 public:
  void emit(std::string line);
  std::optional<StackSlot> get_slot(int value_id) const;
  void mark_needs_got_for_addr(std::string name);
  bool needs_got_for_addr(std::string_view name) const;
  std::optional<PhysReg> assigned_reg(int value_id) const {
    const auto it = assigned_regs.find(value_id);
    if (it == assigned_regs.end()) {
      return std::nullopt;
    }
    return it->second;
  }
  bool is_alloca(int value_id) const { return allocas.find(value_id) != allocas.end(); }
  std::optional<SlotAddr> resolve_slot_addr(int value_id) const {
    const auto slot = get_slot(value_id);
    if (!slot.has_value()) {
      return std::nullopt;
    }
    if (over_aligned_allocas.find(value_id) != over_aligned_allocas.end()) {
      return SlotAddr::OverAligned(*slot, static_cast<std::uint32_t>(value_id));
    }
    if (is_alloca(value_id)) {
      return SlotAddr::Indirect(*slot);
    }
    return SlotAddr::Direct(*slot);
  }
  std::optional<std::size_t> alloca_over_align(int value_id) const {
    const auto it = over_aligned_allocas.find(value_id);
    if (it == over_aligned_allocas.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  std::unordered_map<int, StackSlot> slots;
  std::unordered_map<int, PhysReg> assigned_regs;
  std::unordered_set<std::string> got_addr_symbols;
  std::unordered_set<int> allocas;
  std::unordered_map<int, std::size_t> over_aligned_allocas;
  std::vector<std::string> asm_lines;
};

class RiscvCodegen : public c4c::backend::F128SoftFloatHooks {
 public:
  std::int64_t aligned_frame_size_impl(std::int64_t raw_space) const;
  void emit_prologue_impl(const IrFunction& func, std::int64_t frame_size);
  void emit_epilogue_impl(std::int64_t frame_size);
  void emit_epilogue_and_ret_impl(std::int64_t frame_size);
  IrType current_return_type_impl() const;
  void emit_float_neg_impl(IrType ty);
  void emit_int_neg_impl(IrType ty);
  void emit_int_not_impl(IrType ty);
  void emit_rv_binary_128(std::string_view op);
  void emit_rv_zero_byte_mask(std::string_view src_reg, std::string_view dst_reg);
  void emit_rv_cmpeq_bytes();
  void emit_rv_cmpeq_dwords();
  void emit_rv_binary_128_bytewise();
  void emit_rv_psubusb_8bytes(std::string_view a_reg,
                              std::string_view b_reg,
                              std::string_view dst_reg);
  void emit_rv_psubsb_128();
  void emit_rv_psubsb_8bytes(std::string_view a_reg,
                             std::string_view b_reg,
                             std::string_view dst_reg);
  void emit_rv_pmovmskb();
  void emit_intrinsic_rv(const std::optional<Value>& dest,
                         const IntrinsicOp& op,
                         const std::optional<Value>& dest_ptr,
                         const std::vector<Operand>& args);
  void emit_clz(IrType ty);
  void emit_ctz(IrType ty);
  void emit_bswap(IrType ty);
  void emit_popcount(IrType ty);
  void emit_atomic_load_impl(const Value& dest,
                             const Operand& ptr,
                             IrType ty,
                             AtomicOrdering ordering);
  void emit_atomic_rmw_impl(const Value& dest,
                            AtomicRmwOp op,
                            const Operand& ptr,
                            const Operand& val,
                            IrType ty,
                            AtomicOrdering ordering);
  void emit_atomic_cmpxchg_impl(const Value& dest,
                                const Operand& ptr,
                                const Operand& expected,
                                const Operand& desired,
                                IrType ty,
                                AtomicOrdering ordering,
                                AtomicOrdering failure_ordering,
                                bool returns_bool);
  void emit_atomic_store_impl(const Operand& ptr,
                              const Operand& val,
                              IrType ty,
                              AtomicOrdering ordering);
  void emit_fence_impl(AtomicOrdering ordering);
  void emit_int_binop_impl(const Value& dest,
                           IrBinOp op,
                           const Operand& lhs,
                           const Operand& rhs,
                           IrType ty);
  void emit_copy_i128_impl(const Value& dest, const Operand& src);
  void emit_return_impl(const std::optional<Operand>& value, std::int64_t frame_size);
  void emit_load_operand_for_return_default(const Operand& src) { operand_to_t0(src); }
  void emit_store_result_for_default_cast(const Value& dest) { store_t0_to(dest); }
  void emit_load_acc_pair_for_return_default(const Operand& src) {
    switch (src.kind) {
      case Operand::Kind::Value:
        if (const auto slot = state.get_slot(src.raw)) {
          emit_load_from_s0("t0", slot->raw, "ld");
          emit_load_from_s0("t1", slot->raw + 8, "ld");
          return;
        }
        break;
      case Operand::Kind::ImmI64:
      case Operand::Kind::F32Const:
      case Operand::Kind::F64Const:
        operand_to_t0(src);
        state.emit("    li t1, 0");
        return;
    }

    state.emit("    li t0, 0");
    state.emit("    li t1, 0");
  }
  bool f128_try_get_frame_offset(int value_id, std::int64_t& offset) const override;
  void f128_load_from_frame_offset_to_arg1(std::int64_t offset) override;
  void f128_load_operand_and_extend(const Operand& operand) override;
  void f128_flip_sign_bit() override;
  void f128_store_arg1_to_frame_offset(std::int64_t offset) override;
  void f128_track_self(int dest_id) override;
  void f128_truncate_result_to_acc() override;
  void f128_set_acc_cache(int dest_id) override;
  void f128_alloc_temp_16() override;
  void f128_save_arg1_to_sp() override;
  void f128_move_arg1_to_arg2() override;
  void f128_reload_arg1_from_sp() override;
  void f128_free_temp_16() override;
  void f128_call(const char* name) override;
  void f128_cmp_result_to_bool(c4c::backend::F128CmpKind kind) override;
  void f128_store_bool_result(const Value& dest) override;
  void f128_load_operand_to_acc(const Operand& operand) override;
  void f128_sign_extend_acc(std::size_t from_size) override;
  void f128_zero_extend_acc(std::size_t from_size) override;
  void f128_move_acc_to_arg0() override;
  void f128_store_result_and_truncate(const Value& dest) override;
  void f128_move_arg0_to_acc() override;
  void f128_narrow_acc(IrType to_ty) override;
  void f128_store_acc_to_dest(const Value& dest) override;
  void f128_extend_float_to_f128(IrType from_ty) override;
  void f128_truncate_to_float_acc(IrType to_ty) override;
  void f128_load_from_addr_reg_to_arg1();
  void f128_store_arg1_to_addr();
  void emit_cast_instrs_impl(IrType from_ty, IrType to_ty);
  void emit_cast_impl(const Value& dest, const Operand& src, IrType from_ty, IrType to_ty);
  void emit_load_acc_pair_impl(const Operand& src);
  void emit_store_acc_pair_impl(const Value& dest);
  void emit_store_pair_to_slot_impl(StackSlot slot);
  void emit_load_pair_from_slot_impl(StackSlot slot);
  void emit_save_acc_pair_impl();
  void emit_store_pair_indirect_impl();
  void emit_load_pair_indirect_impl();
  void emit_i128_neg_impl();
  void emit_i128_not_impl();
  void emit_sign_extend_acc_high_impl();
  void emit_zero_acc_high_impl();
  void emit_i128_prep_binop_impl(const Operand& lhs, const Operand& rhs);
  void emit_i128_add_impl();
  void emit_i128_sub_impl();
  void emit_i128_mul_impl();
  void emit_i128_and_impl();
  void emit_i128_or_impl();
  void emit_i128_xor_impl();
  void emit_i128_shl_impl();
  void emit_i128_lshr_impl();
  void emit_i128_ashr_impl();
  void emit_i128_prep_shift_lhs_impl(const Operand& lhs);
  void emit_i128_shl_const_impl(std::uint32_t amount);
  void emit_i128_lshr_const_impl(std::uint32_t amount);
  void emit_i128_ashr_const_impl(std::uint32_t amount);
  void emit_i128_divrem_call_impl(const std::string& func_name,
                                  const Operand& lhs,
                                  const Operand& rhs);
  void emit_i128_store_result_impl(const Value& dest);
  void emit_i128_to_float_call_impl(const Operand& src, bool from_signed, IrType to_ty);
  void emit_float_to_i128_call_impl(const Operand& src, bool to_signed, IrType from_ty);
  void emit_i128_cmp_eq_impl(bool is_ne);
  void emit_i128_cmp_ordered_impl(IrCmpOp op);
  void emit_i128_cmp_store_result_impl(const Value& dest);
  void emit_f128_operand_to_a0_a1(const Operand& src);
  void emit_f128_neg_full(const Value& dest, const Operand& src);
  void emit_f128_neg_impl(const Value& dest, const Operand& src);
  void emit_f128_cmp_impl(const Value& dest,
                          IrCmpOp op,
                          const Operand& lhs,
                          const Operand& rhs);
  void emit_return_i128_to_regs_impl();
  void emit_return_f128_to_reg_impl();
  void emit_return_f32_to_reg_impl();
  void emit_return_f64_to_reg_impl();
  void emit_return_int_to_reg_impl();
  void emit_get_return_f64_second_impl(const Value& dest);
  void emit_set_return_f64_second_impl(const Operand& src);
  void emit_get_return_f32_second_impl(const Value& dest);
  void emit_set_return_f32_second_impl(const Operand& src);
  void emit_va_arg_impl(const Value& dest, const Value& va_list_ptr, IrType result_ty);
  void emit_va_start_impl(const Value& va_list_ptr);
  void emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr);
  void emit_store_impl(const Operand& val, const Value& ptr, IrType ty);
  void emit_load_impl(const Value& dest, const Value& ptr, IrType ty);
  void emit_store_with_const_offset_impl(const Operand& val,
                                         const Value& base,
                                         std::int64_t offset,
                                         IrType ty);
  void emit_load_with_const_offset_impl(const Value& dest,
                                        const Value& base,
                                        std::int64_t offset,
                                        IrType ty);
  static const char* store_for_type(IrType ty);
  static const char* load_for_type(IrType ty);
  void emit_typed_store_to_slot_impl(const char* instr, IrType ty, StackSlot slot);
  void emit_typed_load_from_slot_impl(const char* instr, StackSlot slot);
  void emit_add_offset_to_addr_reg_impl(std::int64_t offset);
  void emit_add_imm_to_acc_impl(std::int64_t imm);
  void emit_round_up_acc_to_16_impl();
  void emit_sub_sp_by_acc_impl();
  void emit_mov_sp_to_acc_impl();
  void emit_mov_acc_to_sp_impl();
  void emit_align_acc_impl(std::size_t align);
  void emit_alloca_aligned_addr_impl(StackSlot slot, std::uint32_t val_id);
  void emit_alloca_aligned_addr_to_acc_impl(StackSlot slot, std::uint32_t val_id);
  void emit_load_ptr_from_slot_impl(StackSlot slot, std::uint32_t val_id);
  void emit_slot_addr_to_secondary_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_gep_direct_const_impl(StackSlot slot, std::int64_t offset);
  void emit_gep_indirect_const_impl(StackSlot slot, std::int64_t offset, std::uint32_t val_id);
  void emit_memcpy_load_dest_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_memcpy_load_src_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_memcpy_impl_impl(std::size_t size);
  CallAbiConfig call_abi_config_impl() const;
  std::size_t emit_call_f128_pre_convert_impl(const std::vector<Operand>& args,
                                              const std::vector<CallArgClass>& arg_classes,
                                              const std::vector<IrType>& arg_types,
                                              std::size_t stack_arg_space);
  std::size_t emit_call_compute_stack_space_impl(const std::vector<CallArgClass>& arg_classes,
                                                 const std::vector<IrType>& arg_types) const;
  std::int64_t emit_call_stack_args_impl(const std::vector<Operand>& args,
                                         const std::vector<CallArgClass>& arg_classes,
                                         const std::vector<IrType>& arg_types,
                                         std::size_t stack_arg_space,
                                         std::size_t fptr_spill,
                                         std::size_t f128_temp_space);
  void emit_call_reg_args_impl(
      const std::vector<Operand>& args,
      const std::vector<CallArgClass>& arg_classes,
      const std::vector<IrType>& arg_types,
      std::int64_t total_sp_adjust,
      std::size_t f128_temp_space,
      std::size_t stack_arg_space,
      const std::vector<std::optional<RiscvFloatClass>>& classes);
  void emit_call_instruction_impl(const std::optional<std::string>& direct_name,
                                  const std::optional<Operand>& func_ptr,
                                  bool indirect,
                                  std::size_t stack_arg_space);
  void emit_call_cleanup_impl(std::size_t stack_arg_space,
                              std::size_t f128_temp_space,
                              bool indirect);
  void emit_call_store_result_impl(const Value& dest, IrType return_type);
  void emit_global_addr_impl(const Value& dest, const std::string& name);
  void emit_tls_global_addr_impl(const Value& dest, const std::string& name);
  void emit_label_addr_impl(const Value& dest, const std::string& label);
  void emit_float_binop_impl(const Value& dest,
                             FloatOp op,
                             const Operand& lhs,
                             const Operand& rhs,
                             IrType ty);
  void emit_int_cmp_impl(const Value& dest,
                         IrCmpOp op,
                         const Operand& lhs,
                         const Operand& rhs,
                         IrType ty);
  void emit_fused_cmp_branch_impl(IrCmpOp op,
                                  const Operand& lhs,
                                  const Operand& rhs,
                                  IrType ty,
                                  const std::string& true_label,
                                  const std::string& false_label);
  void emit_select_impl(const Value& dest,
                        const Operand& cond,
                        const Operand& true_val,
                        const Operand& false_val,
                        IrType ty);
  void emit_float_cmp_impl(const Value& dest,
                           IrCmpOp op,
                           const Operand& lhs,
                           const Operand& rhs,
                           IrType ty);

  RiscvCodegenState state;
  std::vector<PhysReg> used_callee_saved;
  bool is_variadic = false;
  IrType current_return_type = IrType::Void;
  std::int64_t current_frame_size = 0;
  std::int64_t variadic_save_area_bytes = 0;
  std::size_t va_named_gp_count = 0;
  std::size_t va_named_stack_bytes = 0;

 private:
  void emit_store_to_s0(const char* reg, std::int64_t offset, const char* instr);
  void emit_load_from_s0(const char* reg, std::int64_t offset, const char* instr);
  void emit_cmp_operand_load(const Operand& lhs, const Operand& rhs, IrType ty);
  void emit_subword_atomic_rmw(AtomicRmwOp op, IrType ty, const char* aq_rl);
  void emit_subword_atomic_cmpxchg(IrType ty, const char* aq_rl, bool returns_bool);
  void operand_to_t0(const Operand& src);
  void store_t0_to(const Value& dest);
};

// Transitional RV64 codegen surface.
// The intent is that the first owner cluster compiles against this header
// instead of depending on emit.cpp-local declarations.
inline constexpr std::array<PhysReg, 6> RISCV_CALLEE_SAVED = {
    PhysReg(1), PhysReg(7), PhysReg(8), PhysReg(9), PhysReg(10), PhysReg(11),
};

inline constexpr std::array<PhysReg, 5> CALL_TEMP_CALLEE_SAVED = {
    PhysReg(2), PhysReg(3), PhysReg(4), PhysReg(5), PhysReg(6),
};

inline constexpr std::array<const char*, 8> RISCV_ARG_REGS = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
};

inline constexpr std::size_t RISCV_VARIADIC_SLOT_BYTES = 8;
inline constexpr std::size_t RISCV_VARIADIC_LONG_DOUBLE_BYTES = 16;

constexpr std::size_t riscv_variadic_named_gp_count(std::size_t raw_named_gp_count) {
  return raw_named_gp_count < RISCV_ARG_REGS.size() ? raw_named_gp_count : RISCV_ARG_REGS.size();
}

constexpr std::int64_t riscv_variadic_reg_save_area_size() {
  return static_cast<std::int64_t>(RISCV_ARG_REGS.size() * RISCV_VARIADIC_SLOT_BYTES);
}

constexpr std::int64_t riscv_variadic_gp_save_offset(std::size_t reg_index) {
  return static_cast<std::int64_t>(reg_index * RISCV_VARIADIC_SLOT_BYTES);
}

constexpr std::int64_t riscv_variadic_next_arg_stack_offset(std::size_t named_gp_count,
                                                            std::size_t named_stack_bytes) {
  const auto bounded_named_gp_count = riscv_variadic_named_gp_count(named_gp_count);
  if (bounded_named_gp_count >= RISCV_ARG_REGS.size()) {
    return riscv_variadic_reg_save_area_size() + static_cast<std::int64_t>(named_stack_bytes);
  }
  return riscv_variadic_gp_save_offset(bounded_named_gp_count);
}

constexpr std::size_t riscv_variadic_arg_step_bytes(bool is_long_double) {
  return is_long_double ? RISCV_VARIADIC_LONG_DOUBLE_BYTES : RISCV_VARIADIC_SLOT_BYTES;
}

const char* riscv_amo_ordering_suffix(AtomicOrdering ordering);
const char* riscv_amo_width_suffix(IrType ty);
void riscv_sign_extend_atomic_result(RiscvCodegenState& state, IrType ty);
bool riscv_is_atomic_subword_type(IrType ty);
std::uint32_t riscv_atomic_subword_bits(IrType ty);

const char* callee_saved_name(PhysReg reg);
std::optional<PhysReg> riscv_reg_to_callee_saved(std::string_view name);
std::optional<PhysReg> constraint_to_callee_saved_riscv(std::string_view constraint);

enum class RvConstraintKind {
  GpReg,
  FpReg,
  Memory,
  Address,
  Immediate,
  ZeroOrReg,
  Specific,
  Tied,
};

struct RvConstraint {
  RvConstraintKind kind = RvConstraintKind::GpReg;
  std::string specific;
  std::size_t tied = 0;
};

RvConstraint classify_rv_constraint(std::string_view constraint);

std::string substitute_riscv_asm_operands(
    const std::string& line,
    const std::vector<std::string>& op_regs,
    const std::vector<std::optional<std::string>>& op_names,
    const std::vector<RvConstraintKind>& op_kinds,
    const std::vector<long long>& op_mem_offsets,
    const std::vector<std::string>& op_mem_addrs,
    const std::vector<std::optional<long long>>& op_imm_values,
    const std::vector<std::optional<std::string>>& op_imm_symbols,
    const std::vector<std::size_t>& gcc_to_internal);

}  // namespace c4c::backend::riscv::codegen

namespace c4c::backend {

void emit_riscv_return_default(
    riscv::codegen::RiscvCodegen& codegen,
    const riscv::codegen::Operand* value,
    std::int64_t frame_size);
void emit_cast_default(riscv::codegen::RiscvCodegen& codegen,
                       const riscv::codegen::Value& dest,
                       const riscv::codegen::Operand& src,
                       riscv::codegen::IrType from_ty,
                       riscv::codegen::IrType to_ty);

}  // namespace c4c::backend
