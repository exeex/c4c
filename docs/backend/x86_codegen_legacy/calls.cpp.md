# `calls.cpp`

Primary role: core lowering for SysV call setup, call issuance, cleanup, and result publication.

Key surfaces:

```cpp
CallAbiConfig X86Codegen::call_abi_config_impl() const;
void X86Codegen::emit_call_reg_args_impl(const std::vector<Operand>&, const std::vector<CallArgClass>&);
void X86Codegen::emit_call_store_result_impl(const Value&, IrType return_type);
```

- Owns stack-space computation, register-vs-stack argument placement, direct/indirect call emission, and post-call result storage.
- Encodes canonical ABI policy that prepared renderers should consume instead of reopening locally.
- Handles multi-register return cases such as `i128` and floating return moves.
- Special-case classification: `core lowering`; this is one of the main seams the prepared family duplicates.
