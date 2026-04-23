# `emit.cpp` extraction

## Purpose and current responsibility

This file is a legacy entry shim for the x86 backend. It preserves historic public symbols under `c4c::backend::x86`, resolves a usable x86 target profile for direct callers, and immediately hands control to the reviewed `api/` seam.

It is not the backend implementation anymore. Real ownership has already moved into `api::emit_module`, `api::assemble_module`, and the prepared-module/module-emission path behind them.

## Important APIs and contract surfaces

The file exports three compatibility-facing functions and no broader local lowering surface:

```cpp
std::string emit_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(
    const c4c::codegen::lir::LirModule& module,
    const std::string& output_path);
```

Internal contract helpers:

```cpp
const c4c::TargetProfile& resolve_legacy_direct_bir_target_profile(...);
const c4c::TargetProfile& resolve_legacy_direct_lir_target_profile(...);
[[noreturn]] void throw_x86_rewrite_in_progress();
```

Observed contract rules:

- BIR callers may omit an explicit target profile; this shim derives one from `module.target_triple` or host default.
- LIR callers may omit an explicit target profile; this shim accepts `module.target_profile` when present or falls back to host default.
- Only `X86_64` and `I686` are accepted. Non-x86 profiles fail fast with `std::invalid_argument`.
- LIR lowering failure text containing `lir_to_bir lowering` is rewritten into a stronger rebuild-era message that tells callers not to regrow local `emit.cpp` ownership.
- `assemble_module` adapts `BackendAssembleResult` from `api/` into the older `assembler::AssembleResult` shape.

## Dependency direction and hidden inputs

Dependency direction is one-way outward:

- `emit.cpp` depends on `x86_codegen.hpp` for legacy public declarations.
- It depends on `api/x86_codegen_api.hpp` for canonical emission and assembly ownership.
- It includes `peephole/peephole.hpp` but does not use it here; that is residual compatibility/header drag rather than real local behavior.

Hidden inputs that matter:

- Host environment defaults via `default_host_target_triple()`.
- Stringly error-message matching from downstream lowering failure text.
- Target-profile derivation helpers in common target code.
- The downstream `api/` seam, which now owns semantic lowering, prepared-module handoff, staged text emission, and object-file assembly.

## Responsibility buckets

- Legacy symbol preservation: keep old `c4c::backend::x86::*` entry points linkable.
- Target normalization: recover or infer an x86 `TargetProfile` for old call sites.
- Compatibility adaptation: convert canonical API results into older result structs.
- Compatibility diagnostics: remap a specific LIR-lowering failure into a rewrite-in-progress message.

What it does not do anymore:

- no instruction selection
- no module text emission logic
- no prepared-module traversal
- no peephole ownership
- no new matcher growth

## Fast paths, compatibility paths, and overfit pressure

Classification of the remaining behaviors:

- Core lowering: none in this file.
- Optional fast path: none; every public entry forwards immediately to `api/`.
- Legacy compatibility: target-profile fallback, old symbol retention, result-shape adaptation, and the LIR failure-message remap.
- Overfit pressure: the `error.what()` substring check is a narrow compatibility bridge. It is intentionally localized here so callers still get a transition-era diagnostic, but it would be the wrong place to accumulate more case-shaped translation logic.

The strongest rebuild pressure is cultural rather than algorithmic: this file sits at a historically attractive choke point for "just one more shim." The explicit rewrite message is there to block that drift.

## Rebuild ownership

In a rebuild, this file should own only legacy entry compatibility, target-profile normalization for old call sites, and thin result/error adaptation into the canonical x86 API.

It should not own semantic lowering, prepared-module handling, assembly rendering, peephole behavior, target-specific matchers, or any new special-case recovery logic.
