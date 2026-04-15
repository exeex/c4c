# Semantic Call And Runtime Boundary From BIR

Status: Closed
Created: 2026-04-14
Closed: 2026-04-15
Derived from: `plan.md` backlog split from `ideas/open/46_backend_reboot_bir_spine.md`

## Why This Idea Exists

The active BIR reboot runbook should stay focused on the current near-term
lowering route. Richer call lowering, runtime/intrinsic lowering, and the
semantic unsupported boundary are still real backend work, but they form a
separate follow-on initiative instead of the next immediate packet queue.

## Goal

Extend semantic `lir_to_bir` so calls and runtime-facing operations are
represented honestly in BIR, while the remaining unsupported surface is
organized by semantic family rather than by testcase history.

## Primary Targets

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/bir.hpp`

## Scope

- expand call lowering beyond the current minimal direct/non-variadic baseline
- normalize callee provenance and signature meaning before later ABI shaping
- keep new semantic call parsing and inference under `BirFunctionLowerer`
  instead of extending `call_decode.cpp`
- add semantic lowering for runtime and intrinsic families such as `memcpy`,
  `memset`, `va_*`, `stacksave`, `stackrestore`, `abs`, and inline-asm
  placeholder routing where appropriate
- define which aggregate, vector, and pointer-difference forms belong in
  semantic BIR now versus later
- keep unsupported routes honest and grouped by semantic capability family

## Guardrails

- do not grow arg-count ladders as the work queue for call lowering
- do not smuggle target ABI or legality decisions back into `lir_to_bir`
- do not revive `call_decode.cpp` as the active semantic lowering owner
- do not classify progress by testcase family names

## Success Condition

This idea is complete when:

- semantic BIR can represent the common direct and indirect call forms needed
  by the shared backend route
- runtime/intrinsic-heavy functions can enter semantic BIR without route
  escape
- remaining unsupported coverage is explicit by semantic family from the
  planning layer and backend failure mode

## Completion Summary

- semantic call lowering stayed under `BirFunctionLowerer` and the split
  `lir_to_bir_*` owners while broadening the shared direct and indirect call
  route beyond the old minimal baseline
- runtime and intrinsic families on this route now enter semantic BIR through
  semantic lowering decisions or fail through explicit runtime/intrinsic family
  buckets instead of testcase-shaped fallback handling
- the remaining unsupported boundary is now explicit by semantic family in
  both planner-facing wording and backend failure notes, with focused backend
  notes coverage for the narrow and umbrella family buckets emitted by lowering

## Durable Follow-On

- later backend route work continues under the already-open prepare rebuild and
  prepared-BIR target-ingestion ideas rather than by keeping this runbook
  active
