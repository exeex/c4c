# `mod.cpp`

Primary role: shared x86 contract surface for register naming, ABI constants, symbol/data helpers, and regalloc entry.

Key surfaces:

```cpp
std::string x86_format_reg(std::string_view reg, std::optional<char> modifier);
std::vector<c4c::backend::PhysReg> x86_prune_caller_saved_regs(bool has_indirect_call, bool has_inline_asm);
c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(const c4c::backend::LivenessInput&);
```

- Centralizes register-name narrowing, calling-convention register lists, stack-offset formulas, stack-probe thresholds, and symbol/data prelude helpers.
- Publishes both low-level machine facts and higher-level helper emitters, which makes it a broad dependency hub.
- Supplies helpers reused by `inline_asm.cpp`, `globals.cpp`, `prologue.cpp`, and the prepared dispatcher.
- Special-case classification: `core lowering` for machine facts and `dispatch` support for shared symbol/prelude helpers.
