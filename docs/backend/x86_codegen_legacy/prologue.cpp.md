# `prologue.cpp`

Primary role: frame layout, prologue/epilogue emission, and incoming parameter publication.

Key surfaces:

```cpp
std::int64_t X86Codegen::calculate_stack_space_impl(const IrFunction& func);
void X86Codegen::emit_prologue_impl(const IrFunction& func, std::int64_t frame_size);
void X86Codegen::emit_store_params_impl(const IrFunction& func);
```

- Owns frame sizing, stack alignment, callee-save setup, parameter home materialization, and parameter-reference loads.
- Connects ABI policy from `mod.cpp` to actual frame/slot layout in emitted assembly.
- Represents one of the canonical seams that prepared local-slot rendering should reuse rather than shadow.
- Special-case classification: `core lowering`.
