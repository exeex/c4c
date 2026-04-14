// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs
// Structural mirror of the ref Rust source; the shared C++ `RiscvCodegen` /
// `CodegenState` surface does not exist yet, so this file keeps the method
// boundaries and emission intent as comments only.

#include "riscv_codegen.hpp"
//
// //! RiscvCodegen: global address loading, PIC-relative.
//
// use crate::ir::reexports::Value;
// use super::emit::RiscvCodegen;
//
// impl RiscvCodegen {
//     pub(super) fn emit_global_addr_impl(&mut self, dest: &Value, name: &str) {
//         if self.state.needs_got(name) {
//             // PIC mode, external symbol: use `la` which expands to GOT-indirect
//             // auipc + ld with R_RISCV_GOT_HI20 relocation.
//             self.state.emit_fmt(format_args!("    la t0, {}", name));
//         } else {
//             // Non-PIC mode or local symbol: use `lla` (local load address) which
//             // expands to PC-relative auipc + addi with R_RISCV_PCREL_HI20.
//             self.state.emit_fmt(format_args!("    lla t0, {}", name));
//         }
//         self.store_t0_to(dest);
//     }
//
//     pub(super) fn emit_label_addr_impl(&mut self, dest: &Value, label: &str) {
//         // Labels are always local symbols -- use lla (local load address) to avoid
//         // GOT indirection that `la` may produce in PIC mode.
//         self.state.emit_fmt(format_args!("    lla t0, {}", label));
//         self.store_t0_to(dest);
//     }
//
//     pub(super) fn emit_tls_global_addr_impl(&mut self, dest: &Value, name: &str) {
//         // Local Exec TLS model for RISC-V:
//         // lui t0, %tprel_hi(x)
//         // add t0, t0, tp, %tprel_add(x)
//         // addi t0, t0, %tprel_lo(x)
//         self.state.emit_fmt(format_args!("    lui t0, %tprel_hi({})", name));
//         self.state.emit_fmt(format_args!("    add t0, t0, tp, %tprel_add({})", name));
//         self.state.emit_fmt(format_args!("    addi t0, t0, %tprel_lo({})", name));
//         self.store_t0_to(dest);
//     }
// }

namespace c4c::backend::riscv::codegen {

void RiscvCodegen::emit_global_addr_impl(const Value& dest, const std::string& name) {
  if (state.needs_got_for_addr(name)) {
    state.emit("    la t0, " + name);
  } else {
    state.emit("    lla t0, " + name);
  }
  store_t0_to(dest);
}

void RiscvCodegen::emit_label_addr_impl(const Value& dest, const std::string& label) {
  state.emit("    lla t0, " + label);
  store_t0_to(dest);
}

void RiscvCodegen::emit_tls_global_addr_impl(const Value& dest, const std::string& name) {
  state.emit("    lui t0, %tprel_hi(" + name + ")");
  state.emit("    add t0, t0, tp, %tprel_add(" + name + ")");
  state.emit("    addi t0, t0, %tprel_lo(" + name + ")");
  store_t0_to(dest);
}

}  // namespace c4c::backend::riscv::codegen
