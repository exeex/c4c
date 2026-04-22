# `emit.cpp`

Primary role: top-level dispatch from BIR or LIR input into the prepared-module x86 emitter.

Key surfaces:

```cpp
std::string emit_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule&, const std::string& output_path);
```

- Resolves the target triple for direct BIR entry, prepares the semantic module, and hands the result to `emit_prepared_module(...)`.
- Treats failed direct LIR-to-BIR lowering as a rewrite-boundary error instead of adding new `emit.cpp`-local matchers.
- Establishes that prepared-module handoff is the real front door for current x86 emission.
- Special-case classification: `dispatch`.
