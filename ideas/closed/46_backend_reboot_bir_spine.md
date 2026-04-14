# Backend Reboot From BIR Spine

Status: Closed
Created: 2026-04-13
Closed: 2026-04-14
Supersedes: ideas/closed/45_fix_x86_backend_fails.md

## Closure Summary

Closed after the backend reboot source-idea scope was satisfied on the shared
semantic-BIR route:

- semantic BIR now covers the active scalar / CFG baseline, non-variadic
  call-signature lane, local object/address lane, and addressed-global lane
  without raw-LIR escape hatches in the checked backend case inventory
- `BirFunctionLowerer` now owns the active semantic call/signature parsing path
  under `lir_to_bir_*`; `call_decode.cpp` remains only as legacy compatibility
  instead of the route owner
- `prepare` remains the stated legality owner, and the remaining long-tail work
  now belongs to the already-split follow-on ideas rather than this reboot idea

Remaining work was intentionally split out and stays open under:

- `ideas/open/47_semantic_call_runtime_boundary.md`
- `ideas/open/48_prepare_pipeline_rebuild.md`
- `ideas/open/49_prepared_bir_target_ingestion.md`

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

The active lowering shape is now split by ownership instead of one monolithic
`lir_to_bir_module.cpp` bucket:

- `src/backend/lowering/lir_to_bir.cpp` keeps the module-level pipeline entry
- `src/backend/lowering/lir_to_bir.hpp` exposes the searchable lowering API
- `BirFunctionLowerer` owns function-body lowering across the split
  `lir_to_bir_cfg.cpp`, `lir_to_bir_calling.cpp`,
  `lir_to_bir_aggregate.cpp`, `lir_to_bir_memory.cpp`,
  `lir_to_bir_types.cpp`, and `lir_to_bir_globals.cpp` lanes
- semantic call/signature parsing for this route belongs under
  `lir_to_bir_calling.cpp`; `src/backend/lowering/call_decode.cpp` is now a
  legacy compatibility layer rather than the active semantic-lowering owner

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

## Current Route Checkpoint

- backlog item 1, CFG merge / explicit-phi work, is parked at an accepted
  reducible explicit-phi baseline; the next honest seam there is broader
  prepare-side explicit-phi materialization rather than more select/phi
  testcase scouting
- backlog item 2, non-variadic params / function signatures, is parked at an
  accepted semantic-signature baseline covering scalar params, aggregate
  `byval`, hidden `sret`, direct and indirect function-pointer calls, and
  aggregate call-through on the semantic BIR route
- the active near-term capability focus is backlog item 3:
  broaden local memory and address formation around one shared local
  object-plus-offset model for aggregates, arrays, pointer-bearing locals, and
  their addressed load/store/call flows
- backlog item 4, addressed globals and broader module data, stays adjacent to
  the active route and should reuse the same semantic object/address model as
  honest local lowering where possible

## Active Priorities

1. generalize CFG merge and `phi` without regressing to testcase-shaped
   select/merge routing
2. keep params and function signatures honest as the accepted non-variadic
   baseline for semantic BIR
3. broaden local memory and address formation through shared local-object
   semantics
4. broaden global data and addressed globals through the same semantic
   object-address principles

## First Proving Surface

Start with small internal backend-route cases that force semantic lowering to
be real, not guessed from constants:

- `tests/c/internal/backend_case/branch_if_eq.c`
- `tests/c/internal/backend_route_case/single_param_select_eq.c`
- nearby scalar compare/select families

These are not the end goal; they are the first observation surface for whether
semantic BIR is shaped correctly.

Those early proving surfaces are now baseline only. The current active proving
focus should come from the remaining honest seams in local memory/address
formation and then addressed globals, not from reopening already-parked
signature or select/phi harness churn.

## Deferred Follow-On Ideas

The long-tail work that used to sit behind this route now lives in separate
ideas and should not be silently reabsorbed here:

- `ideas/open/47_semantic_call_runtime_boundary.md`
- `ideas/open/48_prepare_pipeline_rebuild.md`
- `ideas/open/49_prepared_bir_target_ingestion.md`

## Success Condition

This idea is complete only when:

- semantic BIR covers the active scalar / CFG / local-memory / addressed-global
  lanes without direct LIR escape hatches
- `prepare` owns target legality instead of `lir_to_bir`
- this idea's route no longer depends on legacy `call_decode.cpp` ownership for
  semantic call/signature parsing
- later target-ingestion and prepare-rebuild work can proceed from the deferred
  follow-on ideas instead of reopening this source idea into another monolithic
  backlog
- backend progress is explained by capability growth, not testcase-shaped
  routing
