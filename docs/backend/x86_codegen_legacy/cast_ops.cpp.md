# `cast_ops.cpp`

Primary role: core lowering for scalar cast and width-conversion sequences.

Key surfaces:

```cpp
void X86Codegen::emit_cast_instrs_impl(IrType from_ty, IrType to_ty);
void X86Codegen::emit_cast_impl(const Value& dest, const Operand& src, IrType from_ty, IrType to_ty);
std::size_t type_size_bytes(IrType ty);
```

- Groups the signed/unsigned/float/i128 type predicates that drive cast selection.
- Uses local labels and width tables to bridge casts that cannot be expressed as one machine instruction.
- Couples to the same `X86Codegen` register/value helpers used by other scalar lowering files.
- Special-case classification: `core lowering`; the label-driven fixups are local implementation detail, not a separate route.
