#pragma once

#include "../assembler/mod.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {
namespace bir {
struct Module;
}

struct LivenessInput;
struct RegAllocIntegrationResult;
struct ParsedBackendExternCallArg;
struct PhysReg;
}  // namespace c4c::backend

namespace c4c::backend::x86 {

// Transitional x86 codegen surface.
// The intent is that sibling translation units in this directory depend on this
// header instead of on emit.cpp-local declarations.

struct Value;
struct Operand;
struct IrFunction;
struct IntrinsicOp;
struct SlotAddr;
struct StackSlot;
struct CallAbiConfig;
struct CallArgClass;
struct AsmOperand;
struct BlockId;

enum class AddressSpace : unsigned;
enum class IrType : unsigned;
enum class IrCmpOp : unsigned;
enum class AtomicOrdering : unsigned;
enum class AtomicRmwOp : unsigned;
enum class FloatOp : unsigned;
enum class AsmOperandKind : unsigned;
enum class EightbyteClass : unsigned;
enum class RiscvFloatClass : unsigned;

struct X86Codegen {
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
  void emit_unaryop_impl(const Value& dest, unsigned op, const Operand& src, IrType ty);

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
  void emit_f128_store_raw_bytes(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
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
  std::vector<AsmOperand> substitute_x86_asm_operands(
      const std::vector<AsmOperand>& operands,
      const std::vector<IrType>& operand_types);
  void emit_operand_with_modifier(const AsmOperand& op, std::optional<char> modifier);
  std::optional<char> default_modifier_for_type(std::optional<IrType> ty);
  std::string format_x86_reg(const std::string& reg, std::optional<char> modifier);
  std::string reg_to_32(const std::string& reg);
  std::string reg_to_64(const std::string& reg);
  std::string reg_to_16(const std::string& reg);
  std::string reg_to_8l(const std::string& reg);
  const char* gcc_cc_to_x86(const std::string& cond);

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

const char* phys_reg_name(c4c::backend::PhysReg reg);
const char* phys_reg_name_32(c4c::backend::PhysReg reg);
std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name);
std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name);
std::string emit_minimal_scalar_global_load_slice_asm(std::string_view target_triple,
                                                      std::string_view function_name,
                                                      std::string_view global_name,
                                                      std::int64_t init_imm,
                                                      std::size_t align_bytes,
                                                      bool zero_initializer);
std::string emit_minimal_extern_scalar_global_load_slice_asm(std::string_view target_triple,
                                                             std::string_view function_name,
                                                             std::string_view global_name);
std::string emit_minimal_scalar_global_store_reload_slice_asm(std::string_view target_triple,
                                                              std::string_view function_name,
                                                              std::string_view global_name,
                                                              std::int64_t init_imm,
                                                              std::int64_t store_imm,
                                                              std::size_t align_bytes);
std::string emit_minimal_global_store_return_and_entry_return_asm(
    std::string_view target_triple,
    const MinimalGlobalStoreReturnAndEntryReturnSlice& slice);
std::optional<std::string> try_emit_minimal_global_two_field_struct_store_sub_sub_module(
    const c4c::backend::bir::Module& module);
std::optional<std::string> try_emit_minimal_variadic_sum2_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_variadic_double_bytes_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_repeated_printf_immediates_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_local_buffer_string_copy_printf_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_counted_printf_ternary_loop_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_string_literal_char_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_param_slot_add_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_extern_zero_arg_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_two_arg_helper_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_two_arg_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_two_arg_second_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_two_arg_second_local_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_void_helper_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_void_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::string> try_emit_minimal_void_extern_call_return_imm_module(
    const c4c::codegen::lir::LirModule& module);
c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::backend::LivenessInput& liveness_input);

std::optional<std::string> try_emit_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::backend::bir::Module& module);
std::optional<std::string> try_emit_prepared_lir_module(
    const c4c::codegen::lir::LirModule& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::x86
