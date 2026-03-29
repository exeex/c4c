# Backend Emitter Backend-IR Migration Plan

## Status

Open as of 2026-03-29.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/35_backend_ready_ir_contract_plan.md`
- `ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md`

Should precede:

- `ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md`

## Goal

Move target emitters and backend dispatch from direct LIR consumption to the new
backend-ready IR so testsuite convergence work is measured against real backend
ownership, not fallback seams.

## Why This Slice Exists

Even after a backend-ready IR contract and lowering layer exist, the backend is
still not cleanly split until the target emitters stop reading frontend LIR
types directly.

Today:

- `src/backend/backend.cpp` dispatches target emitters with
  `const c4c::codegen::lir::LirModule&`
- `src/backend/x86/codegen/emit.hpp`
- `src/backend/aarch64/codegen/emit.hpp`

all expose LIR as the backend-facing contract.

That keeps frontend data structures coupled to target emission logic and makes
it too easy to keep papering over ingestion gaps in emitter-local code.

## Primary Scope

- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- `src/backend/x86/codegen/emit.hpp`
- `src/backend/x86/codegen/emit.cpp`
- `src/backend/aarch64/codegen/emit.hpp`
- `src/backend/aarch64/codegen/emit.cpp`
- any immediately-adjacent backend helper files needed to consume backend IR

## Proposed API Changes

Change backend dispatch and emitter entrypoints to:

- `std::string c4c::backend::x86::emit_module(const BackendModule& module);`
- `assembler::AssembleResult c4c::backend::x86::assemble_module(const BackendModule& module, const std::string& output_path);`
- `std::string c4c::backend::aarch64::emit_module(const BackendModule& module);`
- `assembler::AssembleResult c4c::backend::aarch64::assemble_module(const BackendModule& module, const std::string& output_path);`

Update central dispatch so:

- `src/backend/backend.cpp` calls `lower_to_backend_ir(input.module)` once
- validators run before target emission
- LLVM-text passthrough is treated as an explicit temporary fallback path, not
  the default seam for unsupported backend cases

## Required Refactor Work

- replace emitter-local reads of `LirModule`, `LirFunction`, `LirBlock`, and
  LIR string payloads with backend IR structures
- move any remaining adapter-like normalization out of target emitters and into
  `lower_to_backend_ir(...)`
- add backend-IR-based dump points so x86 / AArch64 emitter tests can snapshot
  pre-emission state
- delete or quarantine `src/backend/lir_adapter.cpp` once no production path
  depends on it

## Reference Files

Use these reference files to shape shared backend-vs-target responsibilities:

- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/traits.rs`
- `ref/claudes-c-compiler/src/backend/state.rs`
- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`

## Non-Goals

- no broad new x86 instruction-selection ambition beyond what the migrated
  backend IR can already express
- no testsuite family growth before emitter ingress has been migrated
- no frontdoor re-expansion of LLVM IR printing as a tolerated backend success
  mode

## Validation

- x86 and AArch64 assembly emission still builds through the migrated APIs
- backend emitter tests can consume backend IR dumps without reading LIR types
- any remaining unsupported backend-ready IR cases fail at a single explicit
  lowering boundary instead of scattered emitter-local pattern checks

## Success Condition

This idea is complete when target emitters accept only backend-owned IR, the
shared backend dispatch performs the LIR-to-backend lowering once, and later
testsuite convergence work can be scoped as backend coverage rather than IR
fallback triage.
