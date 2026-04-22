# `variadic.cpp`

Primary role: canonical variadic ABI support.

Key surfaces:

```cpp
void X86Codegen::emit_va_arg_impl(const Value&, const Value& va_list_ptr, IrType);
void X86Codegen::emit_va_start_impl(const Value& va_list_ptr);
void X86Codegen::emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr);
```

- Owns `va_start`, `va_arg`, `va_copy`, and partial-copy helpers for struct/classified variadic flows.
- Depends on the canonical frame and register-save-area assumptions from `prologue.cpp` and `mod.cpp`.
- Represents a true ABI capability family and not a prepared-only path.
- Special-case classification: `core lowering`.
