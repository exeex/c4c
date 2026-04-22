# `alu.cpp`

Primary role: core lowering for integer arithmetic and shift materialization.

Key surfaces:

```cpp
void emit_simple_alu_reg_direct(X86Codegen&, const Value&, const Operand&, const Operand&, BinaryOpcode);
void emit_shift_reg_direct(X86Codegen&, const Value&, const Operand&, const Operand&, BinaryOpcode);
std::optional<PhysReg> operand_reg(const X86Codegen&, const Operand&);
```

- Resolves assigned registers and immediate-friendly shapes before choosing the emitted ALU sequence.
- Keeps integer width handling local through helpers such as `is_32bit_int_type`, `is_unsigned_int_type`, and sign-extension fixups.
- Depends on `X86CodegenState` register assignments rather than prepared metadata.
- Special-case classification: `core lowering`; the LEA-scale multiply helper is an optional local optimization, not a separate route family.
