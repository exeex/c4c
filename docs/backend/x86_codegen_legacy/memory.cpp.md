# `memory.cpp`

Primary role: core lowering for load/store/addressing and stack-slot memory access.

Key surfaces:

```cpp
void X86Codegen::emit_store_impl(const Operand&, const Value& ptr, IrType);
void X86Codegen::emit_load_with_const_offset_impl(const Value&, const Value& base, std::int64_t offset, IrType);
void X86Codegen::emit_typed_store_indirect_impl(const char* instr, IrType ty);
```

- Owns direct vs indirect vs overaligned slot addressing and the typed load/store helpers built on top of that split.
- Provides the canonical seam for memory operand formation that prepared renderers often reconstruct themselves.
- Couples tightly to `SlotAddr` resolution and frame-slot conventions.
- Special-case classification: `core lowering`.
