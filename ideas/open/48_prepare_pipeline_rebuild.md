# Prepare Pipeline Rebuild From Semantic BIR

Status: Open
Created: 2026-04-14
Derived from: `plan.md` backlog split from `ideas/open/46_backend_reboot_bir_spine.md`

## Why This Idea Exists

The backend reboot will stall if `prepare` remains a shell. Target legality
and the real post-lowering phases need their own initiative so the active
lowering runbook does not carry long-tail prepare reconstruction work inline.

## Goal

Make `prepare` the real owner of target legality and rebuild the stack-layout,
liveness, and regalloc phases as prepared-BIR pipeline stages.

## Primary Targets

- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`
- `src/backend/prepare/stack_layout.cpp`
- `src/backend/prepare/liveness.cpp`
- `src/backend/prepare/regalloc.cpp`

## Reference Shape

- `ref/claudes-c-compiler/src/backend/stack_layout/`
  especially `mod.rs`, `analysis.rs`, `slot_assignment.rs`, and `README.md`
- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/regalloc_helpers.rs`

## Scope

- keep semantic BIR target-neutral where possible and push target legality
  into `prepare`
- legalize `i1` and other target-specific value forms in `prepare`
- define prepared-BIR invariants expected by x86/i686/aarch64/riscv64
- turn the current stack-layout, liveness, and regalloc scaffolds into real
  phase owners with concrete outputs
- define stack objects, frame-layout inputs, liveness data collection, and
  initial register-allocation plumbing over prepared BIR

## Guardrails

- do not let targets reconstruct legality that should live in `prepare`
- do not treat the current `prepare/*.cpp` skeletons as settled local designs
- do not bypass prepared-BIR contracts by sneaking target logic back into
  semantic lowering

## Success Condition

This idea is complete when:

- the backend driver clearly routes semantic BIR through prepare before target
  codegen
- target legality lives in `prepare`
- stack layout, liveness, and regalloc produce real prepared-phase artifacts
  instead of notes or placeholders
