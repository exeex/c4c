# `float_ops.cpp`

Primary role: floating binops plus shared unary instruction lowering.

Key surfaces:

```cpp
void X86Codegen::emit_float_binop_impl(const Value&, const Operand&, const Operand&, FloatOp, IrType);
const char* X86Codegen::emit_float_binop_mnemonic_impl(FloatOp op) const;
void X86Codegen::emit_unaryop_impl(const Value&, const Operand&, UnaryOp, IrType);
```

- Owns scalar float arithmetic while also housing generic unary helpers such as negation, not, count-leading-zeros, and byte-swap selection.
- Suggests the current canonical split is pragmatic rather than perfectly semantic: this file mixes float and integer-unary concerns.
- Depends on type-width helpers and the shared machine emitter rather than prepared-route metadata.
- Special-case classification: `core lowering`; the mixed scope is structural debt, not a second dispatcher.
