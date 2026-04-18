# X86 Backend C-Testsuite Capability Families

Status: Open
Created: 2026-04-17

## Why This Idea Exists

The current x86 backend c-testsuite failure surface is now large enough that
it needs its own durable source idea, separate from the active parser/HIR work
in `ideas/open/15_parser_and_hir_text_id_convergence.md`.

Fresh evidence from 2026-04-17:

- `ctest --test-dir build -L x86_backend --output-on-failure`
  reports `18/220` passed and `202/220` failed
- the dominant failures are explicit capability gates, not generic runtime
  mismatches
- the current failure notes cluster around:
  - minimal prepared-module emitter gates
  - semantic `lir_to_bir` gaps in local-memory
  - scalar-control-flow gaps
  - scalar-cast gaps
  - bounded global-data limitations

Representative failing cases currently include:

- `00005`
- `00020`
- `00032`
- `00037`
- `00040`
- `00042`
- `00050`
- `00051`
- `00073`
- `00086`
- `00204`
- `00213`
- `00214`
- `00215`
- `00217`
- `00218`

## Relationship To Earlier Routes

This is not a return to the old adapter-growth route.

- `ideas/closed/34_backend_x86_c_testsuite_backend_convergence_plan.md`
  correctly parked the older convergence route behind backend-ready IR work
- `ideas/closed/45_fix_x86_backend_fails.md` was intentionally closed when the
  truthful route became the shared BIR reboot rather than x86-local patching

This new idea starts from the current repo shape:

- semantic lowering should grow through `lir_to_bir_*` capability families
- target codegen should consume honest prepared modules
- progress must be explained by real backend capability growth, not by adding
  testcase-shaped recognition

## Route Reset Note

As of 2026-04-18, the previously activated local-memory runbook has been
retracted without closing this source idea.

Reason:

- the original proving cluster stopped being coherent after recent local-memory
  capability movement
- reviewer analysis says
  `src/backend/bir/lir_to_bir_memory.cpp` now needs an ownership-based split
  before more honest local-memory family work keeps widening one monolithic TU
- reviewer analysis also says
  `src/backend/mir/x86/codegen/x86_codegen.hpp` should be de-headerized, but as
  a separate initiative rather than hidden inside the local-memory packet

Durable route consequence:

- this idea remains the umbrella source idea for x86 backend capability-family
  convergence
- the old local-memory runbook is no longer the active execution route
- prerequisite refactors are now tracked as separate open ideas before a new
  capability-family runbook is activated here

Explicitly out of scope for this idea reset:

- AArch64 cleanup or emitter restructuring; that backend is expected to be
  rewritten separately

## Goal

Convert the current x86 backend c-testsuite failure surface into capability
family work packets, then land bounded backend improvements that move real
groups of external C cases onto the backend-owned path without expectation
weakening or testcase overfit.

## Core Rules

- do not weaken `x86_backend` expectations to accept fallback LLVM IR
- do not add testcase-named shortcuts or rendered-text recognizers
- do not revive the older adapter-growth route as the primary mechanism
- do not claim progress from one-off c-testsuite fixes that leave the same
  capability family unsupported nearby
- keep semantic lowering in `lir_to_bir_*` and target-specific legality or
  ingestion at the prepared-module boundary

## Current Failure Families

The visible fail surface should be treated as a small number of backend
capability families, not `202` unrelated tests.

### 1. Local-Memory Semantic Family

Dominant unsupported forms include:

- `alloca`
- `load`
- `store`
- `gep`

This family blocks many straight-line functions before the x86 emitter even has
an honest prepared module to ingest.

### 2. Scalar Control-Flow Family

The x86 emitter still advertises only a minimal single-block return path plus a
bounded compare-against-zero branch family through the canonical prepared-module
handoff.

### 3. Scalar-Cast Family

Some c-testsuite cases currently fail because semantic scalar-cast lowering or
prepared-module legality remains incomplete.

### 4. Global/Data Family

Some cases still hit bounded support notes around:

- minimal scalar globals
- linear integer-array globals
- aggregate-backed globals with honest byte-address semantics

## Next Activation Constraint

Do not reactivate this umbrella idea directly from the earlier local-memory
runbook.

Before the next capability-family activation:

- land or consciously defer the ownership split of
  `src/backend/bir/lir_to_bir_memory.cpp`
- decide separately whether the x86 prepared-module renderer split is also a
  prerequisite for the next family packet, or should remain a parallel open
  idea
- re-baseline the x86 failure surface after that lifecycle decision instead of
  assuming the old local-memory proving cluster is still the right first slice

## Primary Targets

- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- `tests/c/external/c-testsuite/src/*.c` only as probe inputs for family
  selection and bounded proving

## Acceptance Criteria

- [ ] the first execution packet names one dominant capability family rather
      than one testcase
- [ ] at least one bounded local-memory lane is lowered honestly through
      semantic `lir_to_bir` into the prepared x86 handoff
- [ ] nearby cases in the same lane are checked so the result is not just one
      testcase-shaped repair
- [ ] backend notes and handoff tests describe the supported/unsupported
      boundary by capability family
- [ ] no fallback-to-IR acceptance is reintroduced for `x86_backend`
- [ ] the `x86_backend` label shows a truthful pass-count improvement, or the
      packet stops with an explicit capability blocker instead of pretending
      success

## Validation

Narrow proving while iterating:

- `ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure`
- `ctest --test-dir build -R '^backend_x86_handoff_boundary_test$' --output-on-failure`
- targeted `c_testsuite_x86_backend_*` cases chosen from the same local-memory
  family

Checkpoint proving after a coherent slice:

- `ctest --test-dir build -L x86_backend --output-on-failure`

Escalate to broader backend validation if the diff changes shared lowering or
prepared-module behavior outside the bounded lane.

## Non-Goals

- solving all `202` current failures in one route
- reviving legacy adapter normalization as the main path
- claiming backend readiness for the full external C corpus
- weakening expected-output contracts or converting supported tests to
  unsupported

## Good First Patch

Pick a small cluster of currently failing x86 backend c-testsuite cases whose
common blocker is local stack/object addressing, then implement one honest
semantic local-memory lane and prove it across that cluster plus the backend
notes and handoff tests.
