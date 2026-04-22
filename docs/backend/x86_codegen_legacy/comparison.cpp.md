# `comparison.cpp`

Primary role: core lowering for compare, branch, and select materialization.

Key surfaces:

```cpp
void X86Codegen::emit_int_cmp_impl(const Value&, const Operand&, const Operand&, IrCmpOp, IrType);
void X86Codegen::emit_fused_cmp_branch_blocks_impl(IrCmpOp, const Operand&, const Operand&, const BlockId&, const BlockId&, IrType);
void X86Codegen::emit_select_impl(const Value&, const Operand&, const Operand&, const Operand&, IrType);
```

- Maps IR compare opcodes onto `setcc`/`jcc` families and keeps integer, floating, and `f128` comparisons in one ownership bucket.
- Owns fused compare-and-branch lowering for the canonical path.
- Exposes the seam that prepared compare/guard renderers bypass by rebuilding predicate handling locally.
- Special-case classification: `core lowering`.
