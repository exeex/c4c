#pragma once

#include <cstddef>
#include <cstdint>

namespace c4c::backend::riscv::codegen {

class RiscvCodegen;
struct Operand;
struct Value;
enum class FloatOp : unsigned;
enum class IrCmpOp : unsigned;
enum class IrType : unsigned;

}  // namespace c4c::backend::riscv::codegen

namespace c4c::backend {

enum class F128CmpKind : unsigned {
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
};

class F128SoftFloatHooks {
 public:
  virtual ~F128SoftFloatHooks() = default;

  virtual bool f128_try_get_frame_offset(int value_id, std::int64_t& offset) const = 0;
  virtual void f128_load_from_frame_offset_to_arg1(std::int64_t offset) = 0;
  virtual void f128_load_operand_and_extend(const riscv::codegen::Operand& operand) = 0;
  virtual void f128_flip_sign_bit() = 0;
  virtual void f128_store_arg1_to_frame_offset(std::int64_t offset) = 0;
  virtual void f128_track_self(int dest_id) = 0;
  virtual void f128_truncate_result_to_acc() = 0;
  virtual void f128_set_acc_cache(int dest_id) = 0;
  virtual void f128_alloc_temp_16() = 0;
  virtual void f128_save_arg1_to_sp() = 0;
  virtual void f128_move_arg1_to_arg2() = 0;
  virtual void f128_reload_arg1_from_sp() = 0;
  virtual void f128_free_temp_16() = 0;
  virtual void f128_call(const char* name) = 0;
  virtual void f128_cmp_result_to_bool(F128CmpKind kind) = 0;
  virtual void f128_store_bool_result(const riscv::codegen::Value& dest) = 0;
  virtual void f128_load_operand_to_acc(const riscv::codegen::Operand& operand) = 0;
  virtual void f128_sign_extend_acc(std::size_t from_size) = 0;
  virtual void f128_zero_extend_acc(std::size_t from_size) = 0;
  virtual void f128_move_acc_to_arg0() = 0;
  virtual void f128_store_result_and_truncate(const riscv::codegen::Value& dest) = 0;
  virtual void f128_move_arg0_to_acc() = 0;
  virtual void f128_narrow_acc(riscv::codegen::IrType to_ty) = 0;
  virtual void f128_store_acc_to_dest(const riscv::codegen::Value& dest) = 0;
  virtual void f128_extend_float_to_f128(riscv::codegen::IrType from_ty) = 0;
  virtual void f128_truncate_to_float_acc(riscv::codegen::IrType to_ty) = 0;
};

void emit_f128_operand_to_arg1(F128SoftFloatHooks& codegen,
                               const riscv::codegen::Operand& operand);
bool emit_f128_cast(F128SoftFloatHooks& codegen,
                    const riscv::codegen::Value& dest,
                    const riscv::codegen::Operand& src,
                    riscv::codegen::IrType from_ty,
                    riscv::codegen::IrType to_ty);
void emit_f128_neg(F128SoftFloatHooks& codegen,
                   const riscv::codegen::Value& dest,
                   const riscv::codegen::Operand& src);
void emit_riscv_f128_binop(riscv::codegen::RiscvCodegen& codegen,
                           const riscv::codegen::Value& dest,
                           riscv::codegen::FloatOp op,
                           const riscv::codegen::Operand& lhs,
                           const riscv::codegen::Operand& rhs);
void emit_riscv_f128_cmp(riscv::codegen::RiscvCodegen& codegen,
                         const riscv::codegen::Value& dest,
                         riscv::codegen::IrCmpOp op,
                         const riscv::codegen::Operand& lhs,
                         const riscv::codegen::Operand& rhs);
void emit_riscv_f128_store(riscv::codegen::RiscvCodegen& codegen,
                           const riscv::codegen::Operand& value,
                           const riscv::codegen::Value& ptr);
void emit_riscv_f128_load(riscv::codegen::RiscvCodegen& codegen,
                          const riscv::codegen::Value& dest,
                          const riscv::codegen::Value& ptr);
void emit_riscv_f128_store_with_offset(riscv::codegen::RiscvCodegen& codegen,
                                       const riscv::codegen::Operand& value,
                                       const riscv::codegen::Value& base,
                                       std::int64_t offset);
void emit_riscv_f128_load_with_offset(riscv::codegen::RiscvCodegen& codegen,
                                      const riscv::codegen::Value& dest,
                                      const riscv::codegen::Value& base,
                                      std::int64_t offset);

}  // namespace c4c::backend
