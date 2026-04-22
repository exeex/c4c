# `intrinsics.cpp`

Primary role: lowering for x86-targeted intrinsic operations, especially SSE vector helpers.

Key surfaces:

```cpp
void X86Codegen::emit_nontemporal_store(const IntrinsicOp&, const Value& dest_ptr, const Operand& value);
void X86Codegen::emit_sse_binary_128(const Value& dest_ptr, const Operand& lhs, const Operand& rhs, IntrinsicOp);
void X86Codegen::emit_intrinsic_impl(const std::optional<Value>& dest, const std::vector<Operand>& args, IntrinsicOp op);
```

- Maps intrinsic enums onto mnemonic families for binary, unary-immediate, and shuffle instructions.
- Uses explicit XMM staging helpers such as `float_operand_to_xmm0(...)` instead of routing through generic scalar lowering.
- Represents target-specific capability, but still sits on the canonical lowering side rather than the prepared-route side.
- Special-case classification: `optional fast path` around explicit ISA intrinsics.
