# `i128_ops.cpp`

Primary role: core lowering for 128-bit integer arithmetic and shifts.

Key surfaces:

```cpp
void X86Codegen::emit_i128_add_impl();
void X86Codegen::emit_i128_mul_impl();
void X86Codegen::emit_i128_shl_const_impl(std::uint32_t amount);
```

- Materializes wide integer operations through paired-register conventions rather than a separate wide-value abstraction.
- Keeps the `i128` contract narrow and machine-facing: prep operands, issue paired instructions, and publish the result.
- Couples to the current accumulator and shift-register conventions shared with returns/calls.
- Special-case classification: `core lowering`.
