#include "x86_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

[[noreturn]] void throw_unwired_translated_globals_owner() {
  throw std::logic_error(
      "x86 translated globals owner methods are compiled for symbol/link coverage, but the exporter-backed X86Codegen state is not wired yet");
}

}  // namespace

void X86Codegen::emit_global_addr_impl(const Value& dest, const std::string& name) {
  (void)dest;
  (void)name;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_tls_global_addr_impl(const Value& dest, const std::string& name) {
  (void)dest;
  (void)name;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_global_addr_absolute_impl(const Value& dest, const std::string& name) {
  (void)dest;
  (void)name;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_global_load_rip_rel_impl(const Value& dest,
                                                const std::string& sym,
                                                IrType ty) {
  (void)dest;
  (void)sym;
  (void)ty;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_global_store_rip_rel_impl(const Operand& val,
                                                 const std::string& sym,
                                                 IrType ty) {
  (void)val;
  (void)sym;
  (void)ty;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_label_addr_impl(const Value& dest, const std::string& label) {
  (void)dest;
  (void)label;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_store_result_impl(const Value& dest) {
  (void)dest;
  throw_unwired_translated_globals_owner();
}

void X86Codegen::emit_load_operand_impl(const Operand& op) {
  (void)op;
  throw_unwired_translated_globals_owner();
}

}  // namespace c4c::backend::x86
