# `atomics.cpp`

Primary role: core lowering for atomic load/store/RMW/cmpxchg/fence operations.

Key surfaces:

```cpp
void X86Codegen::emit_atomic_rmw_impl(const Value&, const Value&, const Operand&, AtomicRmwOp, IrType);
void X86Codegen::emit_atomic_cmpxchg_impl(const Value&, const Value&, const Operand&, const Operand&, IrType);
void X86Codegen::emit_fence_impl(AtomicOrdering ordering);
```

- Chooses destination registers by type, then emits lock-prefixed or compare-exchange sequences directly.
- Reuses the shared machine state instead of prepared-route metadata or separate dispatcher contexts.
- Keeps the atomic contract narrow: operation kind, pointer/value operands, and memory ordering.
- Special-case classification: `core lowering`; no distinct prepared fast path lives here.
