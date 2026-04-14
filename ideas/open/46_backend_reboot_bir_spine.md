# Backend Reboot From BIR Spine

Status: Open
Created: 2026-04-13
Supersedes: ideas/closed/45_fix_x86_backend_fails.md

## Why This Idea Exists

The previous backend repeatedly failed because BIR was not a credible shared
middle layer. As soon as lowering ran into params, phi, select, memory, calls,
or target legality, execution drifted back to direct LIR routes, `try_emit_*`
helpers, and other testcase-shaped escape hatches.

Those escape hatches are now gone. The remaining work is to make the backend
honestly flow through one spine:

`LIR -> semantic BIR -> prepare/legalize -> target backend`

## Goal

Rebuild the backend around a trustworthy BIR-first pipeline so x86, aarch64,
and riscv64 no longer need direct LIR fallbacks or testcase-shaped routing.

## Core Rules

- do not reintroduce `try_emit_*`, `try_lower_*`, direct-dispatch ladders, or
  any renamed equivalent
- do not add testcase-family matchers based on rendered IR text or named case
  shapes
- keep semantic lowering separate from target-dependent legalization
- make target backends consume prepared BIR, not raw LIR

## Required Design Shape

### Semantic Lowering

`src/backend/lowering/lir_to_bir.cpp` and siblings should produce semantic BIR.

This layer owns:

- function params and returns
- scalar arithmetic / compare
- branch / cond-branch
- select
- phi canonicalization
- memory, globals, and calls

This layer does **not** own target-specific integer widening, ABI legalization,
stack layout, or register allocation.

### Prepare

`src/backend/prepare/legalize.cpp` and follow-on prepare phases should turn
semantic BIR into target-ready BIR.

This layer owns:

- target-dependent type legalization such as `i1 -> i32`
- ABI-oriented value shaping
- stack layout
- liveness
- regalloc

### Target Backends

Each target backend should ingest prepared BIR with an explicit contract.

No target should recover by falling back to raw LIR or by introducing direct
special-case emission routes.

## Initial Priorities

1. make semantic BIR credible for scalar control flow
2. make semantic BIR credible for params and returns
3. add canonical `select` / `phi` handling
4. add memory/global/call lowering
5. make `prepare` real enough that targets consume legalized BIR only

## First Proving Surface

Start with small internal backend-route cases that force semantic lowering to
be real, not guessed from constants:

- `tests/c/internal/backend_case/branch_if_eq.c`
- `tests/c/internal/backend_route_case/single_param_select_eq.c`
- nearby scalar compare/select families

These are not the end goal; they are the first observation surface for whether
semantic BIR is shaped correctly.

## Success Condition

This idea is complete only when:

- semantic BIR covers the core scalar/memory/call lanes without direct LIR
  escape hatches
- `prepare` owns target legality instead of `lir_to_bir`
- x86, aarch64, and riscv64 target backends ingest prepared BIR as the normal
  path
- backend progress is explained by capability growth, not testcase-shaped
  routing
