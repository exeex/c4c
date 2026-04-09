#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_global_addr_impl(const Value& dest, const std::string& name) {
  // RIP-relative addressing is preferred unless the state says a GOT entry is needed.
  if (this->state.needs_got_for_addr(name)) {
    this->state.emit("    movq <sym>@GOTPCREL(%rip), %rax");
  } else {
    this->state.emit("    leaq <sym>(%rip), %rax");
  }
  this->store_rax_to(dest);
}

void X86Codegen::emit_tls_global_addr_impl(const Value& dest, const std::string& name) {
  if (this->state.pic_mode) {
    this->state.emit("    movq <sym>@GOTTPOFF(%rip), %rax");
    this->state.emit("    addq %fs:0, %rax");
  } else {
    this->state.emit("    movq %fs:0, %rax");
    this->state.emit("    leaq <sym>@TPOFF(%rax), %rax");
  }
  this->store_rax_to(dest);
}

void X86Codegen::emit_global_addr_absolute_impl(const Value& dest, const std::string& name) {
  this->state.emit("    movq <sym>, %rax");
  this->store_rax_to(dest);
}

void X86Codegen::emit_global_load_rip_rel_impl(const Value& dest,
                                                const std::string& sym,
                                                IrType ty) {
  // The load mnemonic depends on the destination width.
  this->state.emit("    mov<ty> <sym>(%rip), <dest>");
  this->emit_store_result_impl(dest);
}

void X86Codegen::emit_global_store_rip_rel_impl(const Operand& val,
                                                 const std::string& sym,
                                                 IrType ty) {
  this->emit_load_operand_impl(val);
  this->state.emit("    mov<ty> %rax, <sym>(%rip)");
}

void X86Codegen::emit_label_addr_impl(const Value& dest, const std::string& label) {
  this->state.emit("    leaq <label>(%rip), %rax");
  this->store_rax_to(dest);
}

void X86Codegen::emit_store_result_impl(const Value& dest) {
  this->store_rax_to(dest);
}

void X86Codegen::emit_load_operand_impl(const Operand& op) {
  this->operand_to_rax(op);
}

}  // namespace c4c::backend::x86
