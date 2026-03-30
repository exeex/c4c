#include "emit.hpp"

namespace c4c::backend::x86 {

void X86Codegen::float_operand_to_xmm0(const Operand& op, bool is_f32) {
  this->state.emit("    <float-operand-to-xmm0>");
}

void X86Codegen::emit_nontemporal_store(const IntrinsicOp& op,
                                        const std::optional<Value>& dest_ptr,
                                        const std::vector<Operand>& args) {
  this->state.emit("    <nontemporal-store>");
}

void X86Codegen::emit_sse_binary_128(const Value& dest_ptr,
                                     const std::vector<Operand>& args,
                                     const char* sse_inst) {
  this->state.emit("    <sse-binary-128>");
}

void X86Codegen::emit_sse_unary_imm_128(const Value& dest_ptr,
                                        const std::vector<Operand>& args,
                                        const char* sse_inst) {
  this->state.emit("    <sse-unary-imm-128>");
}

void X86Codegen::emit_sse_shuffle_imm_128(const Value& dest_ptr,
                                          const std::vector<Operand>& args,
                                          const char* sse_inst) {
  this->state.emit("    <sse-shuffle-imm-128>");
}

void X86Codegen::emit_intrinsic_impl(const std::optional<Value>& dest,
                                     const IntrinsicOp& op,
                                     const std::optional<Value>& dest_ptr,
                                     const std::vector<Operand>& args) {
  this->state.emit("    <intrinsic-dispatch>");
}

}  // namespace c4c::backend::x86
