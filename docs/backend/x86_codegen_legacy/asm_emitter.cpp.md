# `asm_emitter.cpp`

Primary role: shared text-emission support for inline-asm style instruction formatting.

Key surfaces:

```cpp
std::string join_line(std::string_view mnemonic, std::string_view lhs, std::string_view rhs);
void emit_instr_reg(X86CodegenState&, std::string_view mnemonic, std::string_view reg);
std::optional<std::string> condition_code_suffix(std::string_view constraint);
```

- Normalizes register text, memory operands, scratch-state bookkeeping, and constraint parsing into reusable string helpers.
- Sits below the main lowering files as a formatter layer rather than a route-selection owner.
- Couples directly to `X86CodegenState` and inline-asm constraint vocabulary.
- Special-case classification: `legacy compatibility` for constraint-shaped helpers that mirror inline-asm syntax quirks.
