# `returns.cpp`

Primary role: canonical return-value lowering and epilogue return flow.

Key surfaces:

```cpp
void X86Codegen::emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size);
void X86Codegen::emit_return_i128_to_regs_impl();
void X86Codegen::emit_return_f128_to_reg_impl();
```

- Publishes integer, float, `i128`, and `f128` results into the ABI-defined return registers before issuing the final epilogue and `ret`.
- Keeps the current return-type contract inside the canonical lowering stack.
- Forms the return-side counterpart to `calls.cpp` result handling.
- Special-case classification: `core lowering`.
