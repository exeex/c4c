#pragma once

#include "../assembler/mod.hpp"
#include "../../../bir/lir_to_bir.hpp"
#include "../../../prealloc/target_register_profile.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {
namespace bir {
struct Module;
enum class BinaryOpcode : unsigned char;
}
namespace prepare {
struct PreparedBirModule;
}

struct ParsedBackendExternCallArg;
struct LivenessInput;
struct LivenessResult;
struct PhysReg;
struct RegAllocIntegrationResult;
}  // namespace c4c::backend

namespace c4c::backend::x86 {

// Transitional x86 codegen surface.
// The intent is that sibling translation units in this directory depend on this
// header instead of on emit.cpp-local declarations.

struct Value {
  std::uint32_t raw = 0;
};

struct Operand {
  std::uint32_t raw = 0;
  std::optional<std::int64_t> immediate = std::nullopt;

  static Operand immediate_i32(std::int32_t value) {
    return Operand{
        .raw = 0,
        .immediate = static_cast<std::int64_t>(value),
    };
  }

  static Operand immediate_i64(std::int64_t value) {
    return Operand{
        .raw = 0,
        .immediate = value,
    };
  }

  static Operand zero() { return Operand::immediate_i32(0); }
};

enum class AddressSpace : unsigned {
  Default,
  SegFs,
  SegGs,
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
enum class IrUnaryOp : unsigned {
  Neg,
  Not,
  Clz,
  Ctz,
  Bswap,
  Popcount,
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
enum class FloatOp : unsigned {
  Add,
  Sub,
  Mul,
  Div,
};
enum class AsmOperandKind : unsigned;
enum class EightbyteClass : unsigned {
  None,
  Integer,
  Sse,
  X87,
  Memory,
};
enum class RiscvFloatClass : unsigned {
  None,
  Float,
  Double,
};

// Active intrinsic inventory carried by the translated x86 intrinsics owner.
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
struct AsmOperand;
struct BlockId {
  std::uint32_t raw = 0;
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
  enum class Kind : unsigned char {
    Register,
    Stack,
  };

  Kind kind = Kind::Register;

  bool is_stack() const { return kind == Kind::Stack; }
  bool is_register() const { return kind == Kind::Register; }
};

struct IrInstruction {
  enum class Kind : unsigned char {
    Other,
    CallIndirect,
    BinOp,
    UnaryOp,
    Cast,
    Cmp,
    Store,
    AtomicRmw,
  };

  Kind kind = Kind::Other;
  IrType ty = IrType::Void;
  IrType from_ty = IrType::Void;
  IrType to_ty = IrType::Void;

  bool is_call_indirect() const { return kind == Kind::CallIndirect; }
};

struct IrBlock {
  std::vector<IrInstruction> instructions;
};

struct IrParam {
  std::string name;
  IrType type = IrType::Void;
};

struct IrFunction {
  bool is_variadic = false;
  IrType return_type = IrType::Void;
  std::vector<EightbyteClass> ret_eightbyte_classes;
  std::vector<IrParam> params;
  std::vector<IrBlock> blocks;
};

namespace ParamClass {

struct IntReg {
  std::size_t reg_idx = 0;
};

struct FloatReg {
  std::size_t reg_idx = 0;
};

struct StructByValReg {
  std::size_t base_reg_idx = 0;
  std::size_t size = 0;
};

struct StructSseReg {
  std::size_t lo_fp_idx = 0;
  std::optional<std::size_t> hi_fp_idx = std::nullopt;
};

struct StructMixedIntSseReg {
  std::size_t int_reg_idx = 0;
  std::size_t fp_reg_idx = 0;
};

struct StructMixedSseIntReg {
  std::size_t fp_reg_idx = 0;
  std::size_t int_reg_idx = 0;
};

struct StructStack {
  std::int64_t offset = 0;
  std::size_t size = 0;
};

struct LargeStructStack {
  std::int64_t offset = 0;
  std::size_t size = 0;
};

struct StackScalar {
  std::int64_t offset = 0;
};

}  // namespace ParamClass

struct ParamClassInfo {
  using Data = std::variant<ParamClass::IntReg,
                            ParamClass::FloatReg,
                            ParamClass::StructByValReg,
                            ParamClass::StructSseReg,
                            ParamClass::StructMixedIntSseReg,
                            ParamClass::StructMixedSseIntReg,
                            ParamClass::StructStack,
                            ParamClass::LargeStructStack,
                            ParamClass::StackScalar>;

  Data data = ParamClass::StackScalar{};

  std::size_t gp_reg_count() const {
    return std::visit(
        [](const auto& value) -> std::size_t {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, ParamClass::IntReg>) {
            return 1;
          } else if constexpr (std::is_same_v<T, ParamClass::StructByValReg>) {
            return value.size > 8 ? 2 : 1;
          } else if constexpr (std::is_same_v<T, ParamClass::StructMixedIntSseReg> ||
                               std::is_same_v<T, ParamClass::StructMixedSseIntReg>) {
            return 1;
          } else {
            return 0;
          }
        },
        data);
  }

  bool is_float_reg() const {
    return std::holds_alternative<ParamClass::FloatReg>(data);
  }
};

struct ParamClassification {
  std::vector<ParamClassInfo> classes;
};

struct X86CodegenState;

struct X86CodegenOutput {
  X86CodegenState* owner = nullptr;

  X86CodegenOutput() = default;
  explicit X86CodegenOutput(X86CodegenState* owner_state) : owner(owner_state) {}

  void bind(X86CodegenState* owner_state) { owner = owner_state; }
  void emit_instr_imm_reg(const char* mnemonic, std::int64_t imm, const char* reg);
  void emit_instr_reg_rbp(const char* mnemonic, const char* reg, std::int64_t offset);
  void emit_instr_rbp_reg(const char* mnemonic, std::int64_t offset, const char* reg);
  void emit_instr_rbp(const char* mnemonic, std::int64_t offset);
  void emit_instr_mem_reg(const char* mnemonic,
                          std::int64_t offset,
                          const char* base_reg,
                          const char* dest_reg);
};

struct X86CodegenRegCache {
  std::optional<std::uint32_t> acc_value_id;
  bool acc_known_zero_extended = false;
  bool acc_valid = false;

  void invalidate_all();
  void invalidate_acc();
  void set_acc(std::uint32_t value_id, bool known_zero_extended);
};

struct X86CodegenState {
  X86CodegenOutput out;
  X86CodegenRegCache reg_cache;
  std::unordered_set<std::uint32_t> f128_direct_slots;
  std::vector<std::string> asm_lines;
  std::unordered_set<std::string> got_addr_symbols;
  std::unordered_map<std::uint32_t, StackSlot> slots;
  std::unordered_map<std::uint32_t, std::uint8_t> reg_assignment_indices;
  std::unordered_set<std::uint32_t> allocas;
  std::unordered_map<std::uint32_t, std::size_t> over_aligned_allocas;
  std::unordered_map<std::uint32_t, std::uint32_t> f128_load_sources;
  std::unordered_map<std::uint32_t, std::array<std::uint64_t, 2>> f128_constant_words;
  std::vector<ParamClassInfo> param_classes;
  std::unordered_set<std::size_t> param_pre_stored;
  bool pic_mode = false;
  bool cf_protection_branch = false;
  bool function_return_thunk = false;

  X86CodegenState();
  X86CodegenState(const X86CodegenState& other);
  X86CodegenState& operator=(const X86CodegenState& other);
  X86CodegenState(X86CodegenState&& other) noexcept;
  X86CodegenState& operator=(X86CodegenState&& other) noexcept;
  void rebind_output();

  void emit(const std::string& asm_line);
  std::optional<StackSlot> get_slot(std::uint32_t value_id) const;
  std::optional<std::uint8_t> assigned_reg_index(std::uint32_t value_id) const;
  bool is_alloca(std::uint32_t value_id) const;
  std::optional<SlotAddr> resolve_slot_addr(std::uint32_t value_id) const;
  void track_f128_load(std::uint32_t dest_id, std::uint32_t ptr_id, std::int64_t offset);
  std::optional<std::uint32_t> get_f128_source(std::uint32_t value_id) const;
  std::optional<std::size_t> alloca_over_align(std::uint32_t value_id) const;
  void set_f128_constant_words(std::uint32_t operand_id, std::uint64_t lo, std::uint64_t hi);
  std::optional<std::array<std::uint64_t, 2>> get_f128_constant_words(std::uint32_t operand_id) const;
  void mark_needs_got_for_addr(std::string name);
  bool needs_got_for_addr(std::string_view name) const;
};

struct X86Codegen {
  X86CodegenState state;
  IrType current_return_type = IrType::Void;
  std::vector<EightbyteClass> func_ret_classes;
  std::vector<EightbyteClass> call_ret_classes;
  std::unordered_map<std::string, std::uint8_t> reg_assignments;
  std::vector<std::uint8_t> used_callee_saved;
  bool is_variadic = false;
  bool no_sse = false;
  std::size_t num_named_int_params = 0;
  std::size_t num_named_fp_params = 0;
  std::int64_t num_named_stack_bytes = 0;
  std::int64_t reg_save_area_offset = 0;

  void operand_to_reg(const Operand& op, const char* reg);
  void operand_to_rax(const Operand& op);
  void operand_to_rcx(const Operand& op);
  void operand_to_rax_rdx(const Operand& op);
  void prep_i128_binop(const Operand& lhs, const Operand& rhs);
  std::optional<std::int64_t> const_as_imm32(const Operand& op) const;
  void store_rax_to(const Value& dest);
  void store_rax_rdx_to(const Value& dest);
  const char* reg_for_type(const char* reg, IrType ty) const;
  const char* mov_load_for_type(IrType ty) const;
  const char* mov_store_for_type(IrType ty) const;
  const char* type_suffix(IrType ty) const;
  void emit_x86_atomic_op_loop(IrType ty, const char* op);

  std::int64_t calculate_stack_space_impl(const IrFunction& func);
  std::int64_t aligned_frame_size_impl(std::int64_t raw_space) const;
  void emit_prologue_impl(const IrFunction& func, std::int64_t frame_size);
  void emit_epilogue_impl(std::int64_t frame_size);
  void emit_store_params_impl(const IrFunction& func);
  void emit_param_ref_impl(const Value& dest, std::size_t param_idx, IrType ty);
  void emit_epilogue_and_ret_impl(std::int64_t frame_size);
  const char* store_instr_for_type_impl(IrType ty) const;
  const char* load_instr_for_type_impl(IrType ty) const;

  CallAbiConfig call_abi_config_impl() const;
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
  void set_call_ret_eightbyte_classes_impl(const std::vector<EightbyteClass>& classes);
  void emit_call_store_result_impl(const Value& dest, IrType return_type);
  void emit_call_store_i128_result_impl(const Value& dest);
  void emit_call_move_f32_to_acc_impl();
  void emit_call_move_f64_to_acc_impl();

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
  void emit_typed_store_to_slot_impl(const char* instr, IrType ty, StackSlot slot);
  void emit_typed_load_from_slot_impl(const char* instr, StackSlot slot);
  void emit_save_acc_impl();
  void emit_load_ptr_from_slot_impl(StackSlot slot, std::uint32_t val_id);
  void emit_typed_store_indirect_impl(const char* instr, IrType ty);
  void emit_typed_load_indirect_impl(const char* instr);
  void emit_add_offset_to_addr_reg_impl(std::int64_t offset);
  void emit_alloca_addr_to(const char* reg, std::uint32_t val_id, std::int64_t offset);
  void emit_slot_addr_to_secondary_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_add_secondary_to_acc_impl();
  void emit_gep_direct_const_impl(StackSlot slot, std::int64_t offset);
  void emit_gep_indirect_const_impl(StackSlot slot, std::int64_t offset, std::uint32_t val_id);
  void emit_gep_add_const_to_acc_impl(std::int64_t offset);
  void emit_add_imm_to_acc_impl(std::int64_t imm);
  void emit_round_up_acc_to_16_impl();
  void emit_sub_sp_by_acc_impl();
  void emit_mov_sp_to_acc_impl();
  void emit_mov_acc_to_sp_impl();
  void emit_align_acc_impl(std::size_t align);
  void emit_memcpy_load_dest_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_memcpy_load_src_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_alloca_aligned_addr_impl(StackSlot slot, std::uint32_t val_id);
  void emit_alloca_aligned_addr_to_acc_impl(StackSlot slot, std::uint32_t val_id);
  void emit_acc_to_secondary_impl();
  void emit_memcpy_store_dest_from_acc_impl();
  void emit_memcpy_store_src_from_acc_impl();
  void emit_memcpy_impl_impl(std::size_t size);
  void emit_seg_load_impl(const Value& dest, const Value& ptr, IrType ty, AddressSpace seg);
  void emit_seg_load_symbol_impl(const Value& dest,
                                 const std::string& sym,
                                 IrType ty,
                                 AddressSpace seg);
  void emit_seg_store_impl(const Operand& val, const Value& ptr, IrType ty, AddressSpace seg);
  void emit_seg_store_symbol_impl(const Operand& val,
                                  const std::string& sym,
                                  IrType ty,
                                  AddressSpace seg);

  void emit_global_addr_impl(const Value& dest, const std::string& name);
  void emit_tls_global_addr_impl(const Value& dest, const std::string& name);
  void emit_global_addr_absolute_impl(const Value& dest, const std::string& name);
  void emit_global_load_rip_rel_impl(const Value& dest, const std::string& sym, IrType ty);
  void emit_global_store_rip_rel_impl(const Operand& val, const std::string& sym, IrType ty);
  void emit_label_addr_impl(const Value& dest, const std::string& label);
  void emit_store_result_impl(const Value& dest);
  void emit_load_operand_impl(const Operand& op);

  void emit_float_cmp_impl(const Value& dest,
                           IrCmpOp op,
                           const Operand& lhs,
                           const Operand& rhs,
                           IrType ty);
  void emit_f128_cmp_impl(const Value& dest,
                          IrCmpOp op,
                          const Operand& lhs,
                          const Operand& rhs);
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
  void emit_fused_cmp_branch_blocks_impl(IrCmpOp op,
                                         const Operand& lhs,
                                         const Operand& rhs,
                                         IrType ty,
                                         BlockId true_block,
                                         BlockId false_block);
  void emit_cond_branch_blocks_impl(const Operand& cond, BlockId true_block, BlockId false_block);
  void emit_select_impl(const Value& dest,
                        const Operand& cond,
                        const Operand& true_val,
                        const Operand& false_val,
                        IrType ty);

  void emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size);
  IrType current_return_type_impl() const;
  void emit_return_f32_to_reg_impl();
  void emit_return_f64_to_reg_impl();
  void emit_return_i128_to_regs_impl();
  void emit_return_f128_to_reg_impl();
  void emit_return_int_to_reg_impl();
  void emit_get_return_f64_second_impl(const Value& dest);
  void emit_set_return_f64_second_impl(const Operand& src);
  void emit_get_return_f32_second_impl(const Value& dest);
  void emit_set_return_f32_second_impl(const Operand& src);
  void emit_get_return_f128_second_impl(const Value& dest);
  void emit_set_return_f128_second_impl(const Operand& src);

  void emit_cast_instrs_impl(IrType from_ty, IrType to_ty);
  void emit_cast_impl(const Value& dest,
                      const Operand& src,
                      IrType from_ty,
                      IrType to_ty);
  void emit_float_neg_impl(IrType ty);
  void emit_int_neg_impl(IrType ty);
  void emit_int_not_impl(IrType ty);
  void emit_int_clz_impl(IrType ty);
  void emit_int_ctz_impl(IrType ty);
  void emit_int_bswap_impl(IrType ty);
  void emit_int_popcount_impl(IrType ty);
  void emit_int_binop_impl(const Value& dest,
                           unsigned op,
                           const Operand& lhs,
                           const Operand& rhs,
                           IrType ty);
  void emit_copy_i128_impl(const Value& dest, const Operand& src);
  void emit_float_binop_impl(const Value& dest,
                             FloatOp op,
                             const Operand& lhs,
                             const Operand& rhs,
                             IrType ty);
  void emit_float_binop_impl_impl(const std::string& mnemonic, IrType ty);
  const char* emit_float_binop_mnemonic_impl(FloatOp op) const;
  void emit_unaryop_impl(const Value& dest, IrUnaryOp op, const Operand& src, IrType ty);

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
  void emit_load_acc_pair_impl(const Operand& op);
  void emit_store_acc_pair_impl(const Value& dest);
  void emit_sign_extend_acc_high_impl();
  void emit_zero_acc_high_impl();
  void emit_store_pair_to_slot_impl(StackSlot slot);
  void emit_load_pair_from_slot_impl(StackSlot slot);
  void emit_save_acc_pair_impl();
  void emit_store_pair_indirect_impl();
  void emit_load_pair_indirect_impl();
  void emit_i128_neg_impl();
  void emit_i128_not_impl();

  std::optional<std::int64_t> emit_f128_resolve_addr(const SlotAddr& addr,
                                                     std::uint32_t ptr_id,
                                                     std::int64_t offset);
  void emit_f128_fldt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
  void emit_f128_fstpt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
  void emit_f128_store_raw_bytes(const SlotAddr& addr,
                                 std::uint32_t ptr_id,
                                 std::int64_t offset,
                                 std::uint64_t lo,
                                 std::uint64_t hi);
  void emit_f128_store_f64_via_x87(const SlotAddr& addr,
                                   std::uint32_t ptr_id,
                                   std::int64_t offset);
  void emit_f128_load_finish(const Value& dest);
  void emit_f128_to_int_from_memory(const SlotAddr& addr, IrType to_ty);
  void emit_f128_st0_to_int(IrType to_ty);
  void emit_cast_instrs_x86(IrType from_ty, IrType to_ty);
  void emit_int_to_f128_cast(IrType from_ty);
  void emit_f128_to_int_cast(IrType to_ty);
  void emit_f128_to_u64_cast();
  void emit_f128_to_f32_cast();
  void emit_fild_to_f64_via_stack();
  void emit_fisttp_from_f64_via_stack();
  void emit_extend_to_rax(IrType ty);
  void emit_sign_extend_to_rax(IrType ty);
  void emit_zero_extend_to_rax(IrType ty);
  void emit_generic_cast(IrType from_ty, IrType to_ty);
  void emit_float_to_unsigned(bool from_f64, bool to_u64, IrType to_ty);
  void emit_int_to_float_conv(bool to_f64);
  void emit_u64_to_float(bool to_f64);
  void emit_f128_load_to_x87(const Operand& operand);

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
                                AtomicOrdering success_ordering,
                                AtomicOrdering failure_ordering,
                                bool returns_bool);
  void emit_atomic_load_impl(const Value& dest,
                             const Operand& ptr,
                             IrType ty,
                             AtomicOrdering ordering);
  void emit_atomic_store_impl(const Operand& ptr,
                              const Operand& val,
                              IrType ty,
                              AtomicOrdering ordering);
  void emit_fence_impl(AtomicOrdering ordering);

  void emit_va_arg_impl(const Value& dest, const Value& va_list_ptr, IrType result_ty);
  void emit_va_arg_struct_impl(const Value& dest_ptr, const Value& va_list_ptr, std::size_t size);
  void emit_va_arg_struct_ex_impl(const Value& dest_ptr,
                                  const Value& va_list_ptr,
                                  std::size_t size,
                                  bool from_register_area);
  void emit_va_start_impl(const Value& va_list_ptr);
  void emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr);
  void emit_partial_copy(std::int64_t offset, std::size_t remaining);

  AsmOperandKind classify_constraint(const std::string& constraint);
  void setup_operand_metadata(AsmOperand& op, const Operand& val, bool is_output);
  bool resolve_memory_operand(AsmOperand& op,
                              const Operand& val,
                              const std::vector<std::string>& excluded);
  std::string assign_scratch_reg(const AsmOperandKind& kind,
                                 const std::vector<std::string>& excluded);
  void load_input_to_reg(const AsmOperand& op,
                         const Operand& val,
                         const std::string& constraint);
  void preload_readwrite_output(const AsmOperand& op, const Value& ptr);
  std::string substitute_template_line(
      const std::string& line,
      const std::vector<AsmOperand>& operands,
      const std::vector<std::size_t>& gcc_to_internal,
      const std::vector<IrType>& operand_types,
      const std::vector<std::pair<std::string, BlockId>>& goto_labels);
  void store_output_from_reg(const AsmOperand& op,
                             const Value& ptr,
                             const std::string& constraint,
                             const std::vector<const char*>& all_output_regs);
  void reset_scratch_state();
  static std::string substitute_x86_asm_operands(
      const std::string& line,
      const std::vector<std::string>& op_regs,
      const std::vector<std::optional<std::string>>& op_names,
      const std::vector<bool>& op_is_memory,
      const std::vector<std::string>& op_mem_addrs,
      const std::vector<IrType>& op_types,
      const std::vector<std::size_t>& gcc_to_internal,
      const std::vector<std::pair<std::string, BlockId>>& goto_labels,
      const std::vector<std::optional<std::int64_t>>& op_imm_values,
      const std::vector<std::optional<std::string>>& op_imm_symbols);
  static void emit_operand_with_modifier(
      std::string& result,
      std::size_t idx,
      std::optional<char> modifier,
      const std::vector<std::string>& op_regs,
      const std::vector<bool>& op_is_memory,
      const std::vector<std::string>& op_mem_addrs,
      const std::vector<IrType>& op_types,
      const std::vector<std::optional<std::int64_t>>& op_imm_values,
      const std::vector<std::optional<std::string>>& op_imm_symbols);
  static std::optional<char> default_modifier_for_type(std::optional<IrType> ty);
  static std::string format_x86_reg(const std::string& reg, std::optional<char> modifier);
  static std::string reg_to_32(const std::string& reg);
  static std::string reg_to_64(const std::string& reg);
  static std::string reg_to_16(const std::string& reg);
  static std::string reg_to_8l(const std::string& reg);
  static const char* gcc_cc_to_x86(const std::string& cond);

  void float_operand_to_xmm0(const Operand& op, bool is_f32);
  void emit_nontemporal_store(const IntrinsicOp& op,
                              const Operand& ptr,
                              const Operand& val,
                              std::optional<Value> dest);
  void emit_sse_binary_128(const Value& dest_ptr,
                           const Operand& lhs,
                           const Operand& rhs,
                           const char* mnemonic);
  void emit_sse_unary_imm_128(const Value& dest_ptr,
                              const Operand& src,
                              std::uint8_t imm,
                              const char* mnemonic);
  void emit_sse_shuffle_imm_128(const Value& dest_ptr,
                                const Operand& lhs,
                                const Operand& rhs,
                                std::uint8_t imm,
                                const char* mnemonic);
  void emit_intrinsic_impl(const std::optional<Value>& dest,
                           const IntrinsicOp& intrinsic,
                           const std::vector<Operand>& args);
};

void emit_unaryop_default(X86Codegen& codegen,
                          const Value& dest,
                          IrUnaryOp op,
                          const Operand& src,
                          IrType ty);

struct MinimalGlobalStoreReturnAndEntryReturnSlice {
  std::string global_name;
  std::string helper_name;
  std::string entry_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  std::int64_t helper_imm = 0;
  std::int64_t entry_imm = 0;
  std::size_t align_bytes = 4;
  bool zero_initializer = false;
};

const char* reg_name_to_32(std::string_view name);
std::string x86_reg_name_to_64(std::string_view reg);
std::string x86_reg_name_to_16(std::string_view reg);
std::string x86_reg_name_to_8l(std::string_view reg);
std::string x86_reg_name_to_8h(std::string_view reg);
std::string x86_format_reg(std::string_view reg, std::optional<char> modifier);
const char* x86_gcc_cc_to_x86(std::string_view cond);
const char* phys_reg_name(c4c::backend::PhysReg reg);
const char* phys_reg_name_32(c4c::backend::PhysReg reg);
const char* x86_alu_mnemonic(c4c::backend::bir::BinaryOpcode op);
std::pair<const char*, const char*> x86_shift_mnemonic(c4c::backend::bir::BinaryOpcode op);
std::vector<c4c::backend::PhysReg> x86_callee_saved_regs();
std::vector<c4c::backend::PhysReg> x86_caller_saved_regs();
std::vector<c4c::backend::PhysReg> x86_prune_caller_saved_regs(bool has_indirect_call,
                                                               bool has_i128_ops,
                                                               bool has_atomic_rmw);
ParamClassification classify_params_full(const IrFunction& func, const CallAbiConfig& config);
std::int64_t named_params_stack_bytes(const std::vector<ParamClassInfo>& classes);
void collect_inline_asm_callee_saved_x86(const IrFunction& func,
                                         std::vector<c4c::backend::PhysReg>& asm_clobbered_regs);
std::vector<c4c::backend::PhysReg> filter_available_regs(
    const std::vector<c4c::backend::PhysReg>& callee_saved,
    const std::vector<c4c::backend::PhysReg>& asm_clobbered);
std::pair<std::unordered_map<std::string, std::uint8_t>,
          std::optional<c4c::backend::LivenessResult>>
run_regalloc_and_merge_clobbers(
    const IrFunction& func,
    const std::vector<c4c::backend::PhysReg>& available_regs,
    const std::vector<c4c::backend::PhysReg>& caller_saved_regs,
    const std::vector<c4c::backend::PhysReg>& asm_clobbered_regs,
    std::unordered_map<std::string, std::uint8_t>& reg_assignments,
    std::vector<std::uint8_t>& used_callee_saved,
    bool allow_inline_asm_regalloc);
std::int64_t calculate_stack_space_common(
    X86CodegenState& state,
    const IrFunction& func,
    std::int64_t initial_space,
    const std::function<std::pair<std::int64_t, std::int64_t>(
        std::int64_t, std::int64_t, std::int64_t)>& alloc_fn,
    const std::unordered_map<std::string, std::uint8_t>& reg_assigned,
    const std::vector<c4c::backend::PhysReg>& callee_saved,
    const std::optional<c4c::backend::LivenessResult>& cached_liveness,
    bool track_param_allocas);
std::optional<c4c::backend::PhysReg> x86_constraint_to_callee_saved(std::string_view constraint);
std::optional<c4c::backend::PhysReg> x86_clobber_name_to_callee_saved(std::string_view name);
const char* x86_arg_reg_name(std::size_t reg_index);
const char* x86_float_arg_reg_name(std::size_t reg_index);
std::int64_t x86_param_stack_base_offset();
std::int64_t x86_param_stack_offset(std::int64_t class_stack_offset);
std::string x86_param_slot_name(std::string_view param_name);
bool x86_param_slot_matches(std::string_view slot_name, std::string_view param_name);
bool x86_allow_struct_split_reg_stack();
const char* x86_param_ref_scalar_load_instr(std::string_view scalar_type);
const char* x86_param_ref_scalar_dest_reg(std::string_view scalar_type);
const char* x86_param_ref_scalar_arg_reg(std::size_t reg_index, std::string_view scalar_type);
const char* x86_param_ref_float_reg_move_instr(std::string_view scalar_type);
const char* x86_param_ref_float_arg_reg(std::size_t reg_index, std::string_view scalar_type);
std::string x86_param_ref_scalar_stack_operand(std::int64_t class_stack_offset,
                                               std::string_view scalar_type);
bool x86_phys_reg_is_callee_saved(c4c::backend::PhysReg reg);
bool x86_param_can_prestore_direct_to_reg(bool has_stack_slot,
                                          std::optional<c4c::backend::PhysReg> assigned_reg,
                                          std::size_t assigned_param_count);
const char* x86_param_prestore_move_instr();
const char* x86_param_prestore_arg_reg(std::size_t reg_index);
const char* x86_param_prestore_dest_reg(c4c::backend::PhysReg reg);
const char* x86_param_prestore_float_move_instr(std::string_view scalar_type);
const char* x86_param_prestore_float_arg_reg(std::size_t reg_index, std::string_view scalar_type);
const char* x86_param_prestore_dest_reg(c4c::backend::PhysReg reg, std::string_view scalar_type);
std::size_t x86_param_struct_reg_qword_count(std::size_t size_bytes);
const char* x86_param_struct_reg_arg_reg(std::size_t base_reg_index, std::size_t qword_index);
std::int64_t x86_param_struct_reg_dest_offset(std::int64_t slot_offset, std::size_t qword_index);
const char* x86_param_struct_sse_arg_reg(std::size_t fp_reg_index);
std::int64_t x86_param_struct_sse_dest_offset(std::int64_t slot_offset, std::size_t qword_index);
const char* x86_param_struct_mixed_int_sse_int_arg_reg(std::size_t int_reg_index);
const char* x86_param_struct_mixed_int_sse_fp_arg_reg(std::size_t fp_reg_index);
std::int64_t x86_param_struct_mixed_int_sse_int_dest_offset(std::int64_t slot_offset);
std::int64_t x86_param_struct_mixed_int_sse_fp_dest_offset(std::int64_t slot_offset);
const char* x86_param_struct_mixed_sse_int_fp_arg_reg(std::size_t fp_reg_index);
const char* x86_param_struct_mixed_sse_int_int_arg_reg(std::size_t int_reg_index);
std::int64_t x86_param_struct_mixed_sse_int_fp_dest_offset(std::int64_t slot_offset);
std::int64_t x86_param_struct_mixed_sse_int_int_dest_offset(std::int64_t slot_offset);
std::size_t x86_param_aggregate_copy_qword_count(std::size_t size_bytes);
std::int64_t x86_param_aggregate_copy_src_offset(std::int64_t class_stack_offset,
                                                 std::size_t qword_index);
std::int64_t x86_param_aggregate_copy_dest_offset(std::int64_t slot_offset,
                                                  std::size_t qword_index);
const char* x86_param_split_reg_stack_arg_reg(std::size_t reg_index);
std::size_t x86_param_split_reg_stack_qword_count(std::size_t size_bytes);
std::int64_t x86_param_split_reg_stack_src_offset(std::int64_t stack_offset,
                                                  std::size_t qword_index);
std::int64_t x86_param_split_reg_stack_dest_offset(std::int64_t slot_offset,
                                                   std::size_t qword_index);
void x86_mark_param_prestored(std::unordered_set<std::size_t>& pre_stored_params,
                              std::size_t param_index);
bool x86_param_is_prestored(const std::unordered_set<std::size_t>& pre_stored_params,
                            std::size_t param_index);
std::int64_t x86_variadic_reg_save_area_size(bool no_sse);
std::int64_t x86_aligned_frame_size(std::int64_t raw_space);
std::int64_t x86_stack_probe_page_size();
bool x86_needs_stack_probe(std::int64_t frame_size);
std::int64_t x86_callee_saved_slot_offset(std::int64_t frame_size, std::size_t save_index);
std::int64_t x86_variadic_gp_save_offset(std::int64_t reg_save_area_base, std::size_t reg_index);
std::int64_t x86_variadic_sse_save_offset(std::int64_t reg_save_area_base, std::size_t reg_index);
std::string decode_llvm_byte_string(std::string_view text);
std::string escape_asm_string(std::string_view raw_bytes);
std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name);
std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name);
std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name);
std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool is_zero_init);
std::string emit_minimal_scalar_global_load_slice_asm(std::string_view target_triple,
                                                      std::string_view function_name,
                                                      std::string_view global_name,
                                                      std::int64_t init_imm,
                                                      std::size_t align_bytes,
                                                      bool zero_initializer);
std::string emit_minimal_extern_scalar_global_load_slice_asm(std::string_view target_triple,
                                                             std::string_view function_name,
                                                             std::string_view global_name);
std::string emit_minimal_extern_global_array_load_slice_asm(std::string_view target_triple,
                                                            std::string_view function_name,
                                                            std::string_view global_name,
                                                            std::int64_t byte_offset);
std::string emit_minimal_scalar_global_store_reload_slice_asm(std::string_view target_triple,
                                                              std::string_view function_name,
                                                              std::string_view global_name,
                                                              std::int64_t init_imm,
                                                              std::int64_t store_imm,
                                                              std::size_t align_bytes);
std::string emit_minimal_global_store_return_and_entry_return_asm(
    std::string_view target_triple,
    const MinimalGlobalStoreReturnAndEntryReturnSlice& slice);
std::string emit_minimal_local_temp_asm(std::string_view target_triple,
                                        std::string_view function_name,
                                        std::int64_t stored_imm);
std::string emit_minimal_constant_branch_return_asm(std::string_view target_triple,
                                                    std::string_view function_name,
                                                    std::int64_t returned_imm);
c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::backend::LivenessInput& liveness_input);

inline std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  const auto resolved_target_profile = module.target_profile.arch != c4c::TargetArch::Unknown
                                           ? module.target_profile
                                           : c4c::target_profile_from_triple(
                                                 module.module.target_triple.empty()
                                                     ? c4c::default_host_target_triple()
                                                     : module.module.target_triple);
  const auto prepared_arch = resolved_target_profile.arch;
  if (prepared_arch != c4c::TargetArch::X86_64 && prepared_arch != c4c::TargetArch::I686) {
    throw std::invalid_argument(
        "x86 backend emitter requires an x86 target for the canonical prepared-module handoff");
  }

  if (module.module.functions.size() != 1) {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module through the canonical prepared-module handoff");
  }

  const auto& function = module.module.functions.front();
  if (function.is_declaration || function.params.size() > 1 ||
      function.return_type != c4c::backend::bir::TypeKind::I32 || function.blocks.empty()) {
    throw std::invalid_argument(
        "x86 backend emitter only supports a minimal i32 return function through the canonical prepared-module handoff");
  }

  const auto& entry = function.blocks.front();
  const auto asm_prefix = ".intel_syntax noprefix\n.text\n.globl " + function.name +
                          "\n.type " + function.name + ", @function\n" + function.name + ":\n";
  const auto narrow_abi_register = [](std::string_view wide_register) -> std::optional<std::string> {
    if (wide_register == "rax") return std::string("eax");
    if (wide_register == "rdi") return std::string("edi");
    if (wide_register == "rsi") return std::string("esi");
    if (wide_register == "rdx") return std::string("edx");
    if (wide_register == "rcx") return std::string("ecx");
    if (wide_register == "r8") return std::string("r8d");
    if (wide_register == "r9") return std::string("r9d");
    return std::string(wide_register);
  };
  const auto narrow_register = [&](const std::optional<std::string>& wide_register)
      -> std::optional<std::string> {
    if (!wide_register.has_value()) {
      return std::nullopt;
    }
    return narrow_abi_register(*wide_register);
  };
  const auto return_register = [&]() -> std::optional<std::string> {
    if (!function.return_abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_result_destination_register_name(
        resolved_target_profile, *function.return_abi));
  }();
  if (!return_register.has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires prepared return ABI metadata for the canonical prepared-module handoff");
  }
  const auto minimal_param_register =
      [&](const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    if (param.is_varargs || param.is_sret || param.is_byval || !param.abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_arg_destination_register_name(
        resolved_target_profile, *param.abi, 0));
  };
  const auto find_block = [&](std::string_view label) -> const c4c::backend::bir::Block* {
    for (const auto& block : function.blocks) {
      if (block.label == label) {
        return &block;
      }
    }
    return nullptr;
  };
  struct LocalSlotLayout {
    std::unordered_map<std::string_view, std::size_t> offsets;
    std::size_t frame_size = 0;
  };
  const auto align_up = [](std::size_t value, std::size_t align) -> std::size_t {
    if (align <= 1) {
      return value;
    }
    const auto remainder = value % align;
    return remainder == 0 ? value : value + (align - remainder);
  };
  const auto build_local_slot_layout = [&]() -> std::optional<LocalSlotLayout> {
    if (prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }
    LocalSlotLayout layout;
    std::size_t next_offset = 0;
    std::size_t max_align = 16;
    for (const auto& slot : function.local_slots) {
      if (slot.type != c4c::backend::bir::TypeKind::I8 &&
          slot.type != c4c::backend::bir::TypeKind::I32 &&
          slot.type != c4c::backend::bir::TypeKind::Ptr) {
        return std::nullopt;
      }
      const auto slot_size = slot.size_bytes != 0
                                 ? slot.size_bytes
                                 : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                    : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                                                                    : 1u);
      const auto slot_align = slot.align_bytes != 0 ? slot.align_bytes : slot_size;
      if (slot_size == 0 || slot_size > 8 || slot_align > 16) {
        return std::nullopt;
      }
      next_offset = align_up(next_offset, slot_align);
      layout.offsets.emplace(slot.name, next_offset);
      next_offset += slot_size;
      max_align = std::max(max_align, slot_align);
    }
    layout.frame_size = align_up(next_offset, max_align);
    return layout;
  };
  const auto render_stack_memory_operand =
      [](std::size_t byte_offset, std::string_view size_name) -> std::string {
    if (byte_offset == 0) {
      return std::string(size_name) + " PTR [rsp]";
    }
    return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
  };
  const auto render_stack_address_expr = [](std::size_t byte_offset) -> std::string {
    if (byte_offset == 0) {
      return "[rsp]";
    }
    return "[rsp + " + std::to_string(byte_offset) + "]";
  };
  const auto try_render_local_address_operand =
      [&](const LocalSlotLayout& local_layout,
          const std::optional<c4c::backend::bir::MemoryAddress>& address,
          std::string_view size_name) -> std::optional<std::string> {
    if (!address.has_value() ||
        address->base_kind != c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot) {
      return std::nullopt;
    }
    const auto slot_it = local_layout.offsets.find(address->base_name);
    if (slot_it == local_layout.offsets.end()) {
      return std::nullopt;
    }
    const auto signed_byte_offset =
        static_cast<std::int64_t>(slot_it->second) + address->byte_offset;
    if (signed_byte_offset < 0) {
      return std::nullopt;
    }
    return render_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset), size_name);
  };
  const auto try_render_minimal_local_slot_return = [&]() -> std::optional<std::string> {
    if (function.params.empty() == false || function.blocks.size() != 1 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value() || entry.insts.empty()) {
      return std::nullopt;
    }
    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }
    const auto& returned = *entry.terminator.value;
    const auto* final_load =
        std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts.back());
    if (final_load == nullptr || returned.kind != c4c::backend::bir::Value::Kind::Named ||
        returned.type != c4c::backend::bir::TypeKind::I32 ||
        final_load->result.name != returned.name ||
        final_load->result.type != c4c::backend::bir::TypeKind::I32 ||
        final_load->byte_offset != 0) {
      return std::nullopt;
    }

    auto asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }

    for (const auto& inst : entry.insts) {
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (store->byte_offset != 0) {
          return std::nullopt;
        }
        std::optional<std::string> memory;
        if (store->address.has_value()) {
          memory = try_render_local_address_operand(
              *layout,
              store->address,
              store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                                                                     : "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(store->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_stack_memory_operand(
              slot_it->second,
              store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
        }
        if (!memory.has_value()) {
          return std::nullopt;
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
            store->value.type == c4c::backend::bir::TypeKind::I32) {
          asm_text += "    mov " + *memory + ", " +
                      std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
          continue;
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
            store->value.type == c4c::backend::bir::TypeKind::Ptr) {
          const auto pointee_slot_it = layout->offsets.find(store->value.name);
          if (pointee_slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          asm_text += "    lea rax, " +
                      render_stack_memory_operand(pointee_slot_it->second, "QWORD").substr(10) +
                      "\n";
          asm_text += "    mov " + *memory + ", rax\n";
          continue;
        }
        return std::nullopt;
      }

      const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
      if (load == nullptr || load->byte_offset != 0) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (load->address.has_value()) {
        memory = try_render_local_address_operand(
            *layout,
            load->address,
            load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
      } else {
        const auto slot_it = layout->offsets.find(load->slot_name);
        if (slot_it == layout->offsets.end()) {
          return std::nullopt;
        }
        memory = render_stack_memory_operand(
            slot_it->second,
            load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
        asm_text += "    mov rax, " + *memory + "\n";
        continue;
      }
      if (load->result.type == c4c::backend::bir::TypeKind::I32) {
        asm_text += "    mov eax, " + *memory + "\n";
        continue;
      }
      return std::nullopt;
    }

    if (layout->frame_size != 0) {
      asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    asm_text += "    ret\n";
    return asm_text;
  };
  const auto try_render_local_slot_guard_chain = [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.empty() ||
        prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }
    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }
    struct GuardJoinContinuation {
      std::string_view join_label;
      const c4c::backend::bir::Block* true_block = nullptr;
      const c4c::backend::bir::Block* false_block = nullptr;
      const c4c::backend::bir::BinaryInst* hoisted_compare = nullptr;
    };
    struct ShortCircuitPlan {
      const c4c::backend::bir::Block* on_compare_true = nullptr;
      bool on_compare_true_uses_continuation = false;
      const c4c::backend::bir::Block* on_compare_false = nullptr;
      bool on_compare_false_uses_continuation = false;
      GuardJoinContinuation continuation;
    };
    struct MaterializedI32Compare {
      std::string_view i1_name;
      std::optional<std::string_view> i32_name;
      c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
      std::string compare_setup;
    };
    const auto resolve_empty_branch_chain =
        [&](std::string_view label) -> const c4c::backend::bir::Block* {
      std::unordered_set<std::string_view> visited;
      const auto* current = find_block(label);
      while (current != nullptr && visited.insert(current->label).second &&
             current->insts.empty() &&
             current->terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
        current = find_block(current->terminator.target_label);
      }
      return current;
    };

    const auto render_function_return =
        [&](std::int32_t returned_imm) -> std::string {
      std::string rendered = "    mov eax, " + std::to_string(returned_imm) + "\n";
      if (layout->frame_size != 0) {
        rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
      }
      rendered += "    ret\n";
      return rendered;
    };

    std::unordered_set<std::string_view> rendered_blocks;
    std::function<std::optional<std::string>(const c4c::backend::bir::Block&,
                                             const std::optional<GuardJoinContinuation>&)>
        render_block =
            [&](const c4c::backend::bir::Block& block,
                const std::optional<GuardJoinContinuation>& continuation)
        -> std::optional<std::string> {
          if (!rendered_blocks.insert(block.label).second &&
              block.terminator.kind != c4c::backend::bir::TerminatorKind::Return) {
            return std::nullopt;
          }

          auto rendered_load_or_store =
              [&](const c4c::backend::bir::Inst& inst,
                  std::optional<std::string_view>* current_i32_name,
                  std::optional<std::string_view>* current_i8_name,
                  std::optional<std::string_view>* current_ptr_name,
                  std::optional<MaterializedI32Compare>* current_materialized_compare)
              -> std::optional<std::string> {
            if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
              std::optional<std::string> memory;
              if (load->address.has_value()) {
                memory = try_render_local_address_operand(
                    *layout,
                    load->address,
                    load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                    : load->result.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                           : "DWORD");
              } else {
                if (load->byte_offset != 0) {
                  return std::nullopt;
                }
                const auto slot_it = layout->offsets.find(load->slot_name);
                if (slot_it == layout->offsets.end()) {
                  return std::nullopt;
                }
                memory = render_stack_memory_operand(
                    slot_it->second,
                    load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                    : load->result.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                           : "DWORD");
              }
              if (!memory.has_value()) {
                return std::nullopt;
              }
              *current_materialized_compare = std::nullopt;
              if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
                *current_i32_name = std::nullopt;
                *current_i8_name = std::nullopt;
                *current_ptr_name = load->result.name;
                return "    mov rax, " + *memory + "\n";
              }
              if (load->result.type == c4c::backend::bir::TypeKind::I32) {
                *current_i32_name = load->result.name;
                *current_i8_name = std::nullopt;
                *current_ptr_name = std::nullopt;
                return "    mov eax, " + *memory + "\n";
              }
              if (load->result.type == c4c::backend::bir::TypeKind::I8) {
                *current_i32_name = std::nullopt;
                *current_i8_name = load->result.name;
                *current_ptr_name = std::nullopt;
                return "    movsx eax, " + *memory + "\n";
              }
              return std::nullopt;
            }

            const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
            if (store == nullptr || store->byte_offset != 0) {
              return std::nullopt;
            }
            std::optional<std::string> memory;
            if (store->address.has_value()) {
              memory = try_render_local_address_operand(
                  *layout,
                  store->address,
                  store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                  : store->value.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                         : "DWORD");
            } else {
              const auto slot_it = layout->offsets.find(store->slot_name);
              if (slot_it == layout->offsets.end()) {
                return std::nullopt;
              }
              memory = render_stack_memory_operand(
                  slot_it->second,
                  store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                  : store->value.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                         : "DWORD");
            }
            if (!memory.has_value()) {
              return std::nullopt;
            }
            *current_materialized_compare = std::nullopt;
            if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
                store->value.type == c4c::backend::bir::TypeKind::I32) {
              *current_i32_name = std::nullopt;
              *current_i8_name = std::nullopt;
              *current_ptr_name = std::nullopt;
              return "    mov " + *memory + ", " +
                     std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
            }
            if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
                store->value.type == c4c::backend::bir::TypeKind::I32 &&
                current_i32_name->has_value() && *current_i32_name == store->value.name) {
              *current_i32_name = std::nullopt;
              *current_i8_name = std::nullopt;
              *current_ptr_name = std::nullopt;
              return "    mov " + *memory + ", eax\n";
            }
            if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
                store->value.type == c4c::backend::bir::TypeKind::I8) {
              *current_i32_name = std::nullopt;
              *current_i8_name = std::nullopt;
              *current_ptr_name = std::nullopt;
              return "    mov " + *memory + ", " +
                     std::to_string(static_cast<std::int8_t>(store->value.immediate)) + "\n";
            }
            if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
                store->value.type == c4c::backend::bir::TypeKind::Ptr) {
              *current_i32_name = std::nullopt;
              *current_i8_name = std::nullopt;
              if (current_ptr_name->has_value() && *current_ptr_name == store->value.name) {
                *current_ptr_name = std::nullopt;
                return "    mov " + *memory + ", rax\n";
              }
              const auto pointee_slot_it = layout->offsets.find(store->value.name);
              if (pointee_slot_it == layout->offsets.end()) {
                *current_ptr_name = std::nullopt;
                return std::string{};
              }
              *current_ptr_name = std::nullopt;
              return "    lea rax, " + render_stack_address_expr(pointee_slot_it->second) +
                     "\n    mov " + *memory + ", rax\n";
            }
            return std::nullopt;
          };

          std::string body;
          std::optional<std::string_view> current_i32_name;
          std::optional<std::string_view> current_i8_name;
          std::optional<std::string_view> current_ptr_name;
          std::optional<MaterializedI32Compare> current_materialized_compare;
          std::size_t compare_index = block.insts.size();
          if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch) {
            if (block.insts.empty()) {
              return std::nullopt;
            }
            compare_index = block.insts.size() - 1;
          } else if (continuation.has_value() &&
                     block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch &&
                     !block.insts.empty()) {
            const auto* branch_compare =
                std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
            if (branch_compare != nullptr &&
                (branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Eq ||
                 branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Ne) &&
                branch_compare->operand_type == c4c::backend::bir::TypeKind::I32 &&
                (branch_compare->result.type == c4c::backend::bir::TypeKind::I1 ||
                 branch_compare->result.type == c4c::backend::bir::TypeKind::I32)) {
              compare_index = block.insts.size() - 1;
            }
          }

          for (std::size_t index = 0; index < compare_index; ++index) {
            const auto rendered_inst =
                rendered_load_or_store(block.insts[index],
                                       &current_i32_name,
                                       &current_i8_name,
                                       &current_ptr_name,
                                       &current_materialized_compare);
            if (rendered_inst.has_value()) {
              body += *rendered_inst;
              continue;
            }

            const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[index]);
            if (binary != nullptr &&
                (binary->opcode == c4c::backend::bir::BinaryOpcode::Eq ||
                 binary->opcode == c4c::backend::bir::BinaryOpcode::Ne) &&
                binary->operand_type == c4c::backend::bir::TypeKind::I32 &&
                binary->result.type == c4c::backend::bir::TypeKind::I1) {
              auto compare_setup = [&]() -> std::optional<std::string> {
                if (current_i32_name.has_value()) {
                  const bool lhs_is_current_rhs_is_imm =
                      binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                      binary->lhs.name == *current_i32_name &&
                      binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                      binary->rhs.type == c4c::backend::bir::TypeKind::I32;
                  const bool rhs_is_current_lhs_is_imm =
                      binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                      binary->rhs.name == *current_i32_name &&
                      binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                      binary->lhs.type == c4c::backend::bir::TypeKind::I32;
                  if (lhs_is_current_rhs_is_imm || rhs_is_current_lhs_is_imm) {
                    const auto compare_immediate =
                        lhs_is_current_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
                    if (compare_immediate == 0) {
                      return std::string("    test eax, eax\n");
                    }
                    return "    cmp eax, " +
                           std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
                  }
                }
                if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                    binary->lhs.type == c4c::backend::bir::TypeKind::I32 &&
                    binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                    binary->rhs.type == c4c::backend::bir::TypeKind::I32) {
                  return "    mov eax, " +
                         std::to_string(static_cast<std::int32_t>(binary->lhs.immediate)) +
                         "\n    cmp eax, " +
                         std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
                }
                return std::nullopt;
              }();
              if (!compare_setup.has_value()) {
                return std::nullopt;
              }
              current_materialized_compare = MaterializedI32Compare{
                  .i1_name = binary->result.name,
                  .opcode = binary->opcode,
                  .compare_setup = *compare_setup,
              };
              current_i32_name = std::nullopt;
              current_i8_name = std::nullopt;
              current_ptr_name = std::nullopt;
              continue;
            }

            const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&block.insts[index]);
            if (cast != nullptr && cast->opcode == c4c::backend::bir::CastOpcode::ZExt &&
                cast->operand.type == c4c::backend::bir::TypeKind::I1 &&
                cast->result.type == c4c::backend::bir::TypeKind::I32 &&
                cast->operand.kind == c4c::backend::bir::Value::Kind::Named &&
                current_materialized_compare.has_value() &&
                current_materialized_compare->i1_name == cast->operand.name) {
              current_materialized_compare->i32_name = cast->result.name;
              current_i32_name = cast->result.name;
              current_i8_name = std::nullopt;
              current_ptr_name = std::nullopt;
              continue;
            }
            if (cast == nullptr || cast->opcode != c4c::backend::bir::CastOpcode::SExt ||
                cast->operand.type != c4c::backend::bir::TypeKind::I8 ||
                cast->result.type != c4c::backend::bir::TypeKind::I32 ||
                cast->operand.kind != c4c::backend::bir::Value::Kind::Named ||
                !current_i8_name.has_value() || *current_i8_name != cast->operand.name) {
              return std::nullopt;
            }
            current_materialized_compare = std::nullopt;
            current_i8_name = std::nullopt;
            current_i32_name = cast->result.name;
            current_ptr_name = std::nullopt;
          }

          const auto render_false_branch_compare =
              [&](const c4c::backend::bir::BinaryInst& compare)
              -> std::optional<std::pair<std::string, std::string>> {
            if (current_materialized_compare.has_value() &&
                current_materialized_compare->i32_name.has_value()) {
              const bool lhs_is_materialized_rhs_is_zero =
                  compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                  compare.lhs.name == *current_materialized_compare->i32_name &&
                  compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                  compare.rhs.type == c4c::backend::bir::TypeKind::I32 &&
                  compare.rhs.immediate == 0;
              const bool rhs_is_materialized_lhs_is_zero =
                  compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                  compare.rhs.name == *current_materialized_compare->i32_name &&
                  compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                  compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
                  compare.lhs.immediate == 0;
              if (lhs_is_materialized_rhs_is_zero || rhs_is_materialized_lhs_is_zero) {
                const bool branch_when_original_false =
                    compare.opcode == c4c::backend::bir::BinaryOpcode::Ne;
                const char* branch_opcode = nullptr;
                if (current_materialized_compare->opcode == c4c::backend::bir::BinaryOpcode::Eq) {
                  branch_opcode = branch_when_original_false ? "jne" : "je";
                } else {
                  branch_opcode = branch_when_original_false ? "je" : "jne";
                }
                return std::pair<std::string, std::string>{current_materialized_compare->compare_setup,
                                                           branch_opcode};
              }
            }

            if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
                compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.rhs.type == c4c::backend::bir::TypeKind::I32) {
              const auto compare_setup =
                  "    mov eax, " +
                  std::to_string(static_cast<std::int32_t>(compare.lhs.immediate)) +
                  "\n    cmp eax, " +
                  std::to_string(static_cast<std::int32_t>(compare.rhs.immediate)) + "\n";
              const char* branch_opcode =
                  compare.opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je";
              return std::pair<std::string, std::string>{compare_setup, branch_opcode};
            }

            if (!current_i32_name.has_value()) {
              return std::nullopt;
            }
            const bool lhs_is_current_rhs_is_imm =
                compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                compare.lhs.name == *current_i32_name &&
                compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.rhs.type == c4c::backend::bir::TypeKind::I32;
            const bool rhs_is_current_lhs_is_imm =
                compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                compare.rhs.name == *current_i32_name &&
                compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.lhs.type == c4c::backend::bir::TypeKind::I32;
            if (!lhs_is_current_rhs_is_imm && !rhs_is_current_lhs_is_imm) {
              return std::nullopt;
            }
            const auto compare_immediate =
                lhs_is_current_rhs_is_imm ? compare.rhs.immediate : compare.lhs.immediate;
            const auto compare_setup =
                compare_immediate == 0
                    ? std::string("    test eax, eax\n")
                    : "    cmp eax, " +
                          std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
            const char* branch_opcode =
                compare.opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je";
            return std::pair<std::string, std::string>{compare_setup, branch_opcode};
          };

          if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Return) {
            if (compare_index != block.insts.size() || !block.terminator.value.has_value()) {
              return std::nullopt;
            }
            const auto& returned = *block.terminator.value;
            if (returned.kind == c4c::backend::bir::Value::Kind::Immediate &&
                returned.type == c4c::backend::bir::TypeKind::I32) {
              return body + render_function_return(static_cast<std::int32_t>(returned.immediate));
            }
            if (returned.kind == c4c::backend::bir::Value::Kind::Named &&
                returned.type == c4c::backend::bir::TypeKind::I32 &&
                current_i32_name.has_value() && *current_i32_name == returned.name) {
              if (layout->frame_size != 0) {
                body += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
              }
              body += "    ret\n";
              return body;
            }
            return std::nullopt;
          }

          if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
            if (continuation.has_value()) {
              const auto* chained_target = resolve_empty_branch_chain(block.terminator.target_label);
              if (chained_target != nullptr && chained_target->label == continuation->join_label) {
                const c4c::backend::bir::BinaryInst* compare = nullptr;
                if (compare_index + 1 == block.insts.size()) {
                  compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
                } else if (compare_index == block.insts.size()) {
                  compare = continuation->hoisted_compare;
                }
                if (compare == nullptr ||
                    (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
                     compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
                    compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
                    (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
                     compare->result.type != c4c::backend::bir::TypeKind::I32)) {
                  return std::nullopt;
                }
                const auto false_branch_compare = render_false_branch_compare(*compare);
                if (!false_branch_compare.has_value()) {
                  return std::nullopt;
                }
                const auto rendered_true = render_block(*continuation->true_block, std::nullopt);
                const auto rendered_false = render_block(*continuation->false_block, std::nullopt);
                if (!rendered_true.has_value() || !rendered_false.has_value()) {
                  return std::nullopt;
                }
                const std::string false_label =
                    ".L" + function.name + "_" + std::string(continuation->false_block->label);
                return body + false_branch_compare->first + "    " + false_branch_compare->second + " " +
                       false_label + "\n" + *rendered_true + false_label + ":\n" +
                       *rendered_false;
              }
            }
            if (compare_index != block.insts.size()) {
              return std::nullopt;
            }
            const auto* target = find_block(block.terminator.target_label);
            if (target == nullptr || target == &block) {
              return std::nullopt;
            }
            const auto rendered_target = render_block(*target, continuation);
            if (!rendered_target.has_value()) {
              return std::nullopt;
            }
            return body + *rendered_target;
          }

          if (block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
              compare_index != block.insts.size() - 1) {
            return std::nullopt;
          }

          const auto* compare =
              std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
          if (compare == nullptr ||
              (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
               compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
              compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
              (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
               compare->result.type != c4c::backend::bir::TypeKind::I32) ||
              block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
              block.terminator.condition.name != compare->result.name) {
            return std::nullopt;
          }
          const auto false_branch_compare = render_false_branch_compare(*compare);
          if (!false_branch_compare.has_value()) {
            return std::nullopt;
          }

          const auto* true_block = find_block(block.terminator.true_label);
          const auto* false_block = find_block(block.terminator.false_label);
          if (true_block == nullptr || false_block == nullptr || true_block == &block ||
              false_block == &block) {
            return std::nullopt;
          }

          const auto detect_short_circuit_plan = [&]() -> std::optional<ShortCircuitPlan> {
            const auto* direct_true_join = resolve_empty_branch_chain(true_block->label);
            const auto* direct_false_join = resolve_empty_branch_chain(false_block->label);
            const bool true_short_circuits =
                direct_true_join != nullptr && direct_true_join != true_block;
            const bool false_short_circuits =
                direct_false_join != nullptr && direct_false_join != false_block;
            if (true_short_circuits == false_short_circuits) {
              return std::nullopt;
            }
            const auto* join_block = true_short_circuits ? direct_true_join : direct_false_join;
            const auto* rhs_entry = true_short_circuits ? false_block : true_block;
            if (join_block == nullptr ||
                join_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
                join_block->insts.size() < 2 || join_block->insts.size() > 3) {
              return std::nullopt;
            }

            const c4c::backend::bir::BinaryInst* hoisted_compare = nullptr;
            std::size_t select_index = 0;
            if (join_block->insts.size() == 3) {
              hoisted_compare =
                  std::get_if<c4c::backend::bir::BinaryInst>(&join_block->insts.front());
              if (hoisted_compare == nullptr ||
                  (hoisted_compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
                   hoisted_compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
                  hoisted_compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
                  (hoisted_compare->result.type != c4c::backend::bir::TypeKind::I1 &&
                   hoisted_compare->result.type != c4c::backend::bir::TypeKind::I32)) {
                return std::nullopt;
              }
              select_index = 1;
            }
            const auto* select =
                std::get_if<c4c::backend::bir::SelectInst>(&join_block->insts[select_index]);
            if (select == nullptr || select->compare_type != c4c::backend::bir::TypeKind::I32 ||
                select->predicate != compare->opcode) {
              return std::nullopt;
            }
            const bool select_matches_compare =
                (select->lhs == compare->lhs && select->rhs == compare->rhs) ||
                (select->lhs == compare->rhs && select->rhs == compare->lhs);
            if (!select_matches_compare) {
              return std::nullopt;
            }

            const c4c::backend::bir::BinaryInst* branch_compare = nullptr;
            branch_compare =
                std::get_if<c4c::backend::bir::BinaryInst>(&join_block->insts.back());
            if (branch_compare == nullptr ||
                (branch_compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
                 branch_compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
                branch_compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
                join_block->terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
                join_block->terminator.condition.name != branch_compare->result.name) {
              return std::nullopt;
            }
            const bool lhs_is_select_rhs_zero =
                branch_compare->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                branch_compare->lhs.name == select->result.name &&
                branch_compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                branch_compare->rhs.type == c4c::backend::bir::TypeKind::I32 &&
                branch_compare->rhs.immediate == 0;
            const bool rhs_is_select_lhs_zero =
                branch_compare->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                branch_compare->rhs.name == select->result.name &&
                branch_compare->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                branch_compare->lhs.type == c4c::backend::bir::TypeKind::I32 &&
                branch_compare->lhs.immediate == 0;
            if (!lhs_is_select_rhs_zero && !rhs_is_select_lhs_zero) {
              return std::nullopt;
            }

            const auto* join_true = find_block(join_block->terminator.true_label);
            const auto* join_false = find_block(join_block->terminator.false_label);
            if (join_true == nullptr || join_false == nullptr || join_true == join_block ||
                join_false == join_block) {
              return std::nullopt;
            }

            GuardJoinContinuation continuation_plan{
                .join_label = join_block->label,
                .true_block = nullptr,
                .false_block = nullptr,
                .hoisted_compare = nullptr,
            };
            if (branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Ne) {
              continuation_plan.true_block = join_true;
              continuation_plan.false_block = join_false;
            } else {
              continuation_plan.true_block = join_false;
              continuation_plan.false_block = join_true;
            }

            const auto classify_bool_value =
                [&](const c4c::backend::bir::Value& value)
                -> std::variant<std::monostate, bool, std::string_view> {
              if (value.kind == c4c::backend::bir::Value::Kind::Immediate &&
                  value.type == c4c::backend::bir::TypeKind::I32 &&
                  (value.immediate == 0 || value.immediate == 1)) {
                return value.immediate != 0;
              }
              if (value.kind == c4c::backend::bir::Value::Kind::Named) {
                return value.name;
              }
              return std::monostate{};
            };

            const auto true_value_kind = classify_bool_value(select->true_value);
            const auto false_value_kind = classify_bool_value(select->false_value);
            const bool true_is_bool = std::holds_alternative<bool>(true_value_kind);
            const bool false_is_bool = std::holds_alternative<bool>(false_value_kind);
            const bool true_is_named = std::holds_alternative<std::string_view>(true_value_kind);
            const bool false_is_named = std::holds_alternative<std::string_view>(false_value_kind);
            if (!((true_is_bool && false_is_named) || (true_is_named && false_is_bool))) {
              return std::nullopt;
            }
            const auto named_value =
                true_is_named ? std::get<std::string_view>(true_value_kind)
                              : std::get<std::string_view>(false_value_kind);
            if (hoisted_compare != nullptr && hoisted_compare->result.name == named_value) {
              continuation_plan.hoisted_compare = hoisted_compare;
            } else if (hoisted_compare != nullptr) {
              return std::nullopt;
            }

            ShortCircuitPlan plan;
            plan.continuation = continuation_plan;
            auto assign_short_circuit = [&](bool compare_truth, bool bool_value) {
              const auto* target =
                  bool_value ? continuation_plan.true_block : continuation_plan.false_block;
              if (compare_truth) {
                plan.on_compare_true = target;
                plan.on_compare_true_uses_continuation = false;
              } else {
                plan.on_compare_false = target;
                plan.on_compare_false_uses_continuation = false;
              }
            };
            auto assign_rhs = [&](bool compare_truth) {
              if (compare_truth) {
                plan.on_compare_true = rhs_entry;
                plan.on_compare_true_uses_continuation = true;
              } else {
                plan.on_compare_false = rhs_entry;
                plan.on_compare_false_uses_continuation = true;
              }
            };

            if (true_is_bool) {
              assign_short_circuit(true, std::get<bool>(true_value_kind));
              assign_rhs(false);
            } else {
              assign_rhs(true);
              assign_short_circuit(false, std::get<bool>(false_value_kind));
            }
            if (plan.on_compare_true == nullptr || plan.on_compare_false == nullptr) {
              return std::nullopt;
            }
            return plan;
          };
          if (const auto short_circuit_plan = detect_short_circuit_plan();
              short_circuit_plan.has_value()) {
            if (short_circuit_plan->on_compare_true_uses_continuation &&
                short_circuit_plan->on_compare_false ==
                    short_circuit_plan->continuation.false_block) {
              const auto rendered_rhs = render_block(
                  *short_circuit_plan->on_compare_true,
                  std::optional<GuardJoinContinuation>(short_circuit_plan->continuation));
              if (!rendered_rhs.has_value()) {
                return std::nullopt;
              }
              const std::string false_label =
                  ".L" + function.name + "_" +
                  std::string(short_circuit_plan->on_compare_false->label);
              return body + false_branch_compare->first + "    " + false_branch_compare->second + " " +
                     false_label + "\n" + *rendered_rhs;
            }
            const auto rendered_true =
                render_block(*short_circuit_plan->on_compare_true,
                             short_circuit_plan->on_compare_true_uses_continuation
                                 ? std::optional<GuardJoinContinuation>(
                                       short_circuit_plan->continuation)
                                 : std::nullopt);
            const auto rendered_false =
                render_block(*short_circuit_plan->on_compare_false,
                             short_circuit_plan->on_compare_false_uses_continuation
                                 ? std::optional<GuardJoinContinuation>(
                                       short_circuit_plan->continuation)
                                 : std::nullopt);
            if (!rendered_true.has_value() || !rendered_false.has_value()) {
              return std::nullopt;
            }
            const std::string false_label =
                ".L" + function.name + "_" + std::string(short_circuit_plan->on_compare_false->label);
            return body + false_branch_compare->first + "    " + false_branch_compare->second + " " +
                   false_label + "\n" + *rendered_true + false_label + ":\n" +
                   *rendered_false;
          }

          const auto rendered_true = render_block(*true_block, std::nullopt);
          const auto rendered_false = render_block(*false_block, std::nullopt);
          if (!rendered_true.has_value() || !rendered_false.has_value()) {
            return std::nullopt;
          }

          const std::string false_label = ".L" + function.name + "_" + false_block->label;
          return body + false_branch_compare->first + "    " + false_branch_compare->second + " " +
                 false_label + "\n" + *rendered_true + false_label + ":\n" +
                 *rendered_false;
        };

    auto asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    const auto rendered_entry = render_block(entry, std::nullopt);
    if (!rendered_entry.has_value()) {
      return std::nullopt;
    }
    asm_text += *rendered_entry;
    return asm_text;
  };
  const auto try_render_local_i32_arithmetic_guard = [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 3 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        prepared_arch != c4c::TargetArch::X86_64 || entry.insts.empty()) {
      return std::nullopt;
    }

    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }

    const auto* true_block = find_block(entry.terminator.true_label);
    const auto* false_block = find_block(entry.terminator.false_label);
    if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
        false_block == &entry ||
        true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
        !true_block->insts.empty() || !false_block->insts.empty()) {
      return std::nullopt;
    }

    struct NamedLocalI32Expr {
      enum class Kind { LocalLoad, Add, Sub };
      Kind kind = Kind::LocalLoad;
      std::string operand;
      c4c::backend::bir::Value lhs;
      c4c::backend::bir::Value rhs;
    };
    std::unordered_map<std::string_view, NamedLocalI32Expr> named_i32_exprs;
    const auto render_guard_return =
        [&](std::int32_t returned_imm) -> std::string {
      std::string rendered = "    mov eax, " + std::to_string(returned_imm) + "\n";
      if (layout->frame_size != 0) {
        rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
      }
      rendered += "    ret\n";
      return rendered;
    };

    std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_operand;
    std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_value;
    render_i32_operand =
        [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
          if (value.type != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }
          if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
            return std::to_string(static_cast<std::int32_t>(value.immediate));
          }
          if (value.kind != c4c::backend::bir::Value::Kind::Named) {
            return std::nullopt;
          }
          const auto expr_it = named_i32_exprs.find(value.name);
          if (expr_it == named_i32_exprs.end() ||
              expr_it->second.kind != NamedLocalI32Expr::Kind::LocalLoad) {
            return std::nullopt;
          }
          return expr_it->second.operand;
        };
    render_i32_value =
        [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
          if (value.type != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }
          if (const auto operand = render_i32_operand(value); operand.has_value()) {
            return "    mov eax, " + *operand + "\n";
          }
          if (value.kind != c4c::backend::bir::Value::Kind::Named) {
            return std::nullopt;
          }
          const auto expr_it = named_i32_exprs.find(value.name);
          if (expr_it == named_i32_exprs.end()) {
            return std::nullopt;
          }
          const auto& expr = expr_it->second;
          if (expr.kind == NamedLocalI32Expr::Kind::Add) {
            if (const auto lhs_render = render_i32_value(expr.lhs); lhs_render.has_value()) {
              if (const auto rhs_operand = render_i32_operand(expr.rhs); rhs_operand.has_value()) {
                return *lhs_render + "    add eax, " + *rhs_operand + "\n";
              }
            }
            if (const auto rhs_render = render_i32_value(expr.rhs); rhs_render.has_value()) {
              if (const auto lhs_operand = render_i32_operand(expr.lhs); lhs_operand.has_value()) {
                return *rhs_render + "    add eax, " + *lhs_operand + "\n";
              }
            }
            return std::nullopt;
          }
          if (expr.kind == NamedLocalI32Expr::Kind::Sub) {
            if (const auto lhs_render = render_i32_value(expr.lhs); lhs_render.has_value()) {
              if (const auto rhs_operand = render_i32_operand(expr.rhs); rhs_operand.has_value()) {
                return *lhs_render + "    sub eax, " + *rhs_operand + "\n";
              }
            }
            return std::nullopt;
          }
          return "    mov eax, " + expr.operand + "\n";
        };

    auto asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }

    for (std::size_t index = 0; index + 1 < entry.insts.size(); ++index) {
      const auto& inst = entry.insts[index];
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (store->byte_offset != 0 || store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
            store->value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        std::optional<std::string> memory;
        if (store->address.has_value()) {
          memory = try_render_local_address_operand(*layout, store->address, "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(store->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_stack_memory_operand(slot_it->second, "DWORD");
        }
        if (!memory.has_value()) {
          return std::nullopt;
        }
        asm_text += "    mov " + *memory + ", " +
                    std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
        continue;
      }

      if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
        if (load->byte_offset != 0 || load->result.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        std::optional<std::string> memory;
        if (load->address.has_value()) {
          memory = try_render_local_address_operand(*layout, load->address, "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(load->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_stack_memory_operand(slot_it->second, "DWORD");
        }
        if (!memory.has_value()) {
          return std::nullopt;
        }
        named_i32_exprs[load->result.name] = NamedLocalI32Expr{
            .kind = NamedLocalI32Expr::Kind::LocalLoad,
            .operand = *memory,
        };
        continue;
      }

      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
      if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
          binary->result.type != c4c::backend::bir::TypeKind::I32 ||
          (binary->opcode != c4c::backend::bir::BinaryOpcode::Add &&
           binary->opcode != c4c::backend::bir::BinaryOpcode::Sub)) {
        return std::nullopt;
      }
      named_i32_exprs[binary->result.name] = NamedLocalI32Expr{
          .kind = binary->opcode == c4c::backend::bir::BinaryOpcode::Add
                      ? NamedLocalI32Expr::Kind::Add
                      : NamedLocalI32Expr::Kind::Sub,
          .lhs = binary->lhs,
          .rhs = binary->rhs,
      };
    }

    const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.back());
    if (compare == nullptr ||
        (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
         compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
        compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
        (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
         compare->result.type != c4c::backend::bir::TypeKind::I32) ||
        entry.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
        entry.terminator.condition.name != compare->result.name) {
      return std::nullopt;
    }

    const bool lhs_is_value_rhs_is_imm =
        compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->rhs.type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_value_lhs_is_imm =
        compare->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->lhs.type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
      return std::nullopt;
    }

    const auto compared_value = lhs_is_value_rhs_is_imm ? compare->lhs : compare->rhs;
    const auto compared_render = render_i32_value(compared_value);
    if (!compared_render.has_value()) {
      return std::nullopt;
    }
    const auto compare_immediate =
        lhs_is_value_rhs_is_imm ? compare->rhs.immediate : compare->lhs.immediate;
    asm_text += *compared_render;
    asm_text += compare_immediate == 0
                    ? "    test eax, eax\n"
                    : "    cmp eax, " +
                          std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
    asm_text += std::string("    ") +
                (compare->opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je") + " .L" +
                function.name + "_" + false_block->label + "\n";

    const auto render_return_block =
        [&](const c4c::backend::bir::Block& block) -> std::optional<std::string> {
      const auto& value = *block.terminator.value;
      if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      return render_guard_return(static_cast<std::int32_t>(value.immediate));
    };
    const auto rendered_true = render_return_block(*true_block);
    const auto rendered_false = render_return_block(*false_block);
    if (!rendered_true.has_value() || !rendered_false.has_value()) {
      return std::nullopt;
    }
    asm_text += *rendered_true;
    asm_text += ".L" + function.name + "_" + false_block->label + ":\n";
    asm_text += *rendered_false;
    return asm_text;
  };
  const auto try_render_param_derived_value =
      [&](const c4c::backend::bir::Value& value,
          const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
              named_binaries,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    if (value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      return "    mov " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(value.immediate)) +
             "\n";
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }
    if (value.name == param.name) {
      if (const auto param_register = minimal_param_register(param); param_register.has_value()) {
        return "    mov " + *return_register + ", " + *param_register + "\n";
      }
      return std::nullopt;
    }

    const auto binary_it = named_binaries.find(value.name);
    if (binary_it == named_binaries.end()) {
      return std::nullopt;
    }

    const auto& binary = *binary_it->second;
    if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary.result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const bool lhs_is_param_rhs_is_imm =
        binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary.lhs.name == param.name &&
        binary.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary.rhs.type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_param_lhs_is_imm =
        binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary.rhs.name == param.name &&
        binary.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary.lhs.type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_param_rhs_is_imm && !rhs_is_param_lhs_is_imm) {
      return std::nullopt;
    }

    const auto immediate =
        lhs_is_param_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
    const auto param_register = minimal_param_register(param);
    if (!param_register.has_value()) {
      return std::nullopt;
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
      return "    mov " + *return_register + ", " + *param_register + "\n    add " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    sub " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
      return "    mov " + *return_register + ", " + *param_register + "\n    imul " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
      return "    mov " + *return_register + ", " + *param_register + "\n    and " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
      return "    mov " + *return_register + ", " + *param_register + "\n    or " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
      return "    mov " + *return_register + ", " + *param_register + "\n    xor " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    shl " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    shr " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    sar " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    return std::nullopt;
  };
  const auto apply_supported_immediate_binary =
      [&](const c4c::backend::bir::BinaryInst& binary,
          std::string_view source_name) -> std::optional<std::string> {
    if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary.result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const bool lhs_is_source_rhs_is_imm =
        binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary.lhs.name == source_name &&
        binary.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary.rhs.type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_source_lhs_is_imm =
        binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary.rhs.name == source_name &&
        binary.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary.lhs.type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_source_rhs_is_imm && !rhs_is_source_lhs_is_imm) {
      return std::nullopt;
    }

    const auto immediate =
        lhs_is_source_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
      return "    add " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_source_rhs_is_imm) {
      return "    sub " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
      return "    imul " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
      return "    and " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
      return "    or " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
      return "    xor " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_source_rhs_is_imm) {
      return "    shl " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_source_rhs_is_imm) {
      return "    shr " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_source_rhs_is_imm) {
      return "    sar " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    return std::nullopt;
  };
  const auto try_render_param_derived_return =
      [&](const c4c::backend::bir::Value& value,
          const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
              named_binaries,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    const auto value_render = try_render_param_derived_value(value, named_binaries, param);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    return *value_render + "    ret\n";
  };
  const auto try_render_minimal_compare_branch = [&]() -> std::optional<std::string> {
    if (function.blocks.size() != 3 || function.params.size() != 1 ||
        prepared_arch != c4c::TargetArch::X86_64 || entry.insts.size() != 1 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      return std::nullopt;
    }

    const auto& param = function.params.front();
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
        param.is_byval) {
      return std::nullopt;
    }
    const auto param_register = minimal_param_register(param);
    if (!param_register.has_value()) {
      return std::nullopt;
    }

    const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
    if (compare == nullptr || compare->opcode != c4c::backend::bir::BinaryOpcode::Eq ||
        compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
        compare->result.type != c4c::backend::bir::TypeKind::I32 ||
        entry.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
        entry.terminator.condition.name != compare->result.name) {
      return std::nullopt;
    }

    const bool lhs_is_param_rhs_is_zero =
        compare->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        compare->lhs.name == param.name &&
        compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->rhs.type == c4c::backend::bir::TypeKind::I32 && compare->rhs.immediate == 0;
    const bool rhs_is_param_lhs_is_zero =
        compare->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        compare->rhs.name == param.name &&
        compare->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->lhs.type == c4c::backend::bir::TypeKind::I32 && compare->lhs.immediate == 0;
    if (!lhs_is_param_rhs_is_zero && !rhs_is_param_lhs_is_zero) {
      return std::nullopt;
    }

    const auto* true_block = find_block(entry.terminator.true_label);
    const auto* false_block = find_block(entry.terminator.false_label);
    if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
        false_block == &entry || true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
        !true_block->insts.empty() || !false_block->insts.empty()) {
      return std::nullopt;
    }

    const auto& true_value = *true_block->terminator.value;
    const auto& false_value = *false_block->terminator.value;
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
    const auto true_return = try_render_param_derived_return(true_value, named_binaries, param);
    const auto false_return = try_render_param_derived_return(false_value, named_binaries, param);
    if (!true_return.has_value() || !false_return.has_value()) {
      return std::nullopt;
    }

    const std::string false_label = ".L" + function.name + "_" + false_block->label;
    return asm_prefix + "    test " + *param_register + ", " + *param_register + "\n    jne " +
           false_label + "\n" + *true_return +
           false_label + ":\n" + *false_return;
  };
  const auto try_render_materialized_compare_join = [&]() -> std::optional<std::string> {
    if (function.blocks.size() != 4 || function.params.size() != 1 ||
        prepared_arch != c4c::TargetArch::X86_64 || entry.insts.size() != 1 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      return std::nullopt;
    }

    const auto& param = function.params.front();
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
        param.is_byval) {
      return std::nullopt;
    }
    const auto param_register = minimal_param_register(param);
    if (!param_register.has_value()) {
      return std::nullopt;
    }

    const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
    if (compare == nullptr || compare->opcode != c4c::backend::bir::BinaryOpcode::Eq ||
        compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
        compare->result.type != c4c::backend::bir::TypeKind::I32 ||
        entry.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
        entry.terminator.condition.name != compare->result.name) {
      return std::nullopt;
    }

    const bool lhs_is_param_rhs_is_zero =
        compare->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        compare->lhs.name == param.name &&
        compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->rhs.type == c4c::backend::bir::TypeKind::I32 && compare->rhs.immediate == 0;
    const bool rhs_is_param_lhs_is_zero =
        compare->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        compare->rhs.name == param.name &&
        compare->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->lhs.type == c4c::backend::bir::TypeKind::I32 && compare->lhs.immediate == 0;
    if (!lhs_is_param_rhs_is_zero && !rhs_is_param_lhs_is_zero) {
      return std::nullopt;
    }

    const auto* true_block = find_block(entry.terminator.true_label);
    const auto* false_block = find_block(entry.terminator.false_label);
    if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
        false_block == &entry || true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        !true_block->insts.empty() || !false_block->insts.empty() ||
        true_block->terminator.target_label != false_block->terminator.target_label) {
      return std::nullopt;
    }

    const auto* join_block = find_block(true_block->terminator.target_label);
    if (join_block == nullptr || join_block == &entry || join_block == true_block ||
        join_block == false_block || join_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !join_block->terminator.value.has_value() || join_block->insts.empty()) {
      return std::nullopt;
    }

    std::size_t select_index = join_block->insts.size() - 1;
    const c4c::backend::bir::BinaryInst* trailing_binary = nullptr;
    if (const auto* trailing =
            std::get_if<c4c::backend::bir::BinaryInst>(&join_block->insts.back());
        trailing != nullptr &&
        join_block->terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
        join_block->terminator.value->name == trailing->result.name) {
      if (join_block->insts.size() < 2) {
        return std::nullopt;
      }
      trailing_binary = trailing;
      select_index = join_block->insts.size() - 2;
    }

    const auto* select =
        std::get_if<c4c::backend::bir::SelectInst>(&join_block->insts[select_index]);
    if (select == nullptr || select->predicate != c4c::backend::bir::BinaryOpcode::Eq ||
        select->compare_type != c4c::backend::bir::TypeKind::I32 ||
        select->result.type != c4c::backend::bir::TypeKind::I32 ||
        join_block->terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
        (trailing_binary == nullptr && join_block->terminator.value->name != select->result.name)) {
      return std::nullopt;
    }

    const bool select_lhs_is_param_rhs_is_zero =
        select->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        select->lhs.name == param.name &&
        select->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        select->rhs.type == c4c::backend::bir::TypeKind::I32 && select->rhs.immediate == 0;
    const bool select_rhs_is_param_lhs_is_zero =
        select->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        select->rhs.name == param.name &&
        select->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        select->lhs.type == c4c::backend::bir::TypeKind::I32 && select->lhs.immediate == 0;
    const bool select_matches_compare =
        (select->lhs.kind == compare->lhs.kind && select->lhs.name == compare->lhs.name &&
         select->lhs.immediate == compare->lhs.immediate && select->rhs.kind == compare->rhs.kind &&
         select->rhs.name == compare->rhs.name &&
         select->rhs.immediate == compare->rhs.immediate) ||
        (select->lhs.kind == compare->rhs.kind && select->lhs.name == compare->rhs.name &&
         select->lhs.immediate == compare->rhs.immediate && select->rhs.kind == compare->lhs.kind &&
         select->rhs.name == compare->lhs.name &&
         select->rhs.immediate == compare->lhs.immediate);
    if ((!select_lhs_is_param_rhs_is_zero && !select_rhs_is_param_lhs_is_zero) ||
        !select_matches_compare) {
      return std::nullopt;
    }

    std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
    for (std::size_t inst_index = 0; inst_index < select_index; ++inst_index) {
      const auto* binary =
          std::get_if<c4c::backend::bir::BinaryInst>(&join_block->insts[inst_index]);
      if (binary == nullptr) {
        return std::nullopt;
      }
      named_binaries.emplace(binary->result.name, binary);
    }

    const auto render_join_return =
        [&](const c4c::backend::bir::Value& selected_value) -> std::optional<std::string> {
      const auto value_render =
          try_render_param_derived_value(selected_value, named_binaries, param);
      if (!value_render.has_value()) {
        return std::nullopt;
      }
      if (trailing_binary == nullptr) {
        return *value_render + "    ret\n";
      }
      const auto trailing_render =
          apply_supported_immediate_binary(*trailing_binary, select->result.name);
      if (!trailing_render.has_value()) {
        return std::nullopt;
      }
      return *value_render + *trailing_render + "    ret\n";
    };
    const auto true_return = render_join_return(select->true_value);
    const auto false_return = render_join_return(select->false_value);
    if (!true_return.has_value() || !false_return.has_value()) {
      return std::nullopt;
    }

    const std::string false_label = ".L" + function.name + "_" + false_block->label;
    return asm_prefix + "    test " + *param_register + ", " + *param_register + "\n    jne " +
           false_label + "\n" + *true_return +
           false_label + ":\n" + *false_return;
  };
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    if (const auto rendered_branch = try_render_minimal_compare_branch(); rendered_branch.has_value()) {
      return *rendered_branch;
    }
    if (const auto rendered_join = try_render_materialized_compare_join(); rendered_join.has_value()) {
      return *rendered_join;
    }
    if (const auto rendered_local_i32_guard = try_render_local_i32_arithmetic_guard();
        rendered_local_i32_guard.has_value()) {
      return *rendered_local_i32_guard;
    }
    if (const auto rendered_local_slot_guard_chain = try_render_local_slot_guard_chain();
        rendered_local_slot_guard_chain.has_value()) {
      return *rendered_local_slot_guard_chain;
    }
    throw std::invalid_argument(
        "x86 backend emitter only supports a minimal single-block i32 return terminator, a bounded equality-against-immediate guard family with immediate return leaves, or one bounded compare-against-zero branch family through the canonical prepared-module handoff");
  }

  const auto& returned = *entry.terminator.value;
  if (returned.type != c4c::backend::bir::TypeKind::I32) {
    throw std::invalid_argument(
        "x86 backend emitter only supports i32 return values through the canonical prepared-module handoff");
  }
  if (const auto rendered_local_slot = try_render_minimal_local_slot_return();
      rendered_local_slot.has_value()) {
    return *rendered_local_slot;
  }
  if (entry.insts.empty()) {
    if (returned.kind == c4c::backend::bir::Value::Kind::Immediate) {
      if (!function.params.empty()) {
        throw std::invalid_argument(
            "x86 backend emitter only supports parameter-free immediate i32 returns through the canonical prepared-module handoff");
      }
      return asm_prefix + "    mov " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(returned.immediate)) + "\n    ret\n";
    }

    if (function.params.size() == 1 && prepared_arch == c4c::TargetArch::X86_64) {
      const auto& param = function.params.front();
      if (param.type == c4c::backend::bir::TypeKind::I32 && !param.is_varargs &&
          !param.is_sret && !param.is_byval && returned.name == param.name) {
        if (const auto param_register = minimal_param_register(param); param_register.has_value()) {
          return asm_prefix + "    mov " + *return_register + ", " + *param_register + "\n    ret\n";
        }
      }
    }
  }

  if (entry.insts.size() == 1 && function.params.size() == 1 &&
      returned.kind == c4c::backend::bir::Value::Kind::Named &&
      prepared_arch == c4c::TargetArch::X86_64) {
    const auto& param = function.params.front();
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
    if (binary != nullptr && param.type == c4c::backend::bir::TypeKind::I32 && !param.is_varargs &&
        !param.is_sret && !param.is_byval &&
        binary->operand_type == c4c::backend::bir::TypeKind::I32 &&
        binary->result.type == c4c::backend::bir::TypeKind::I32 &&
        returned.name == binary->result.name) {
      const bool lhs_is_param_rhs_is_imm =
          binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
          binary->lhs.name == param.name &&
          binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          binary->rhs.type == c4c::backend::bir::TypeKind::I32;
      const bool rhs_is_param_lhs_is_imm =
          binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
          binary->rhs.name == param.name &&
          binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          binary->lhs.type == c4c::backend::bir::TypeKind::I32;
      const auto param_register = minimal_param_register(param);
      if (!param_register.has_value()) {
        throw std::invalid_argument(
            "x86 backend emitter requires prepared parameter ABI metadata for the canonical prepared-module handoff");
      }
      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    add " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    sub " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Mul &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    imul " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::And &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    and " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Or &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    or " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Xor &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    xor " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Shl &&
          lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    shl " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) +
               "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::LShr &&
          lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    shr " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) +
               "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::AShr &&
          lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    sar " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) +
               "\n    ret\n";
      }
    }
  }

  throw std::invalid_argument(
      "x86 backend emitter only supports direct immediate i32 returns, direct single-parameter i32 passthrough returns, single-parameter i32 add-immediate/sub-immediate/mul-immediate/and-immediate/or-immediate/xor-immediate/shl-immediate/lshr-immediate/ashr-immediate returns, a bounded equality-against-immediate guard family with immediate return leaves, or one bounded compare-against-zero branch family through the canonical prepared-module handoff");
}
std::string emit_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::x86
