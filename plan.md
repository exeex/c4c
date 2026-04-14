# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Activated from: fresh activation on 2026-04-14 after closing the backend reboot BIR-spine idea and splitting long-tail follow-on initiatives

## Purpose

Keep the backend on the semantic-BIR route while widening call and
runtime-facing lowering beyond the current minimal non-variadic baseline.

## Goal

Make semantic `lir_to_bir` represent common direct/indirect calls and
runtime-facing operations honestly, with remaining unsupported behavior grouped
by semantic capability family rather than by testcase history.

## Core Rule

Do not push target ABI or legality decisions back into semantic lowering, and
do not recover progress through testcase-shaped call matchers, arg-count
ladders, or renewed `call_decode.cpp` ownership.

## Read First

- `ideas/open/47_semantic_call_runtime_boundary.md`
- `src/backend/bir.hpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/lowering/call_decode.cpp`
- `src/backend/backend.cpp`

## Current Targets

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/bir.hpp`

## Activation Checkpoint

- the closed backend-reboot idea already established the semantic-BIR baseline
  for non-variadic function signatures, direct and indirect pointer calls, and
  the local/global address lanes needed for that route
- `call_decode.cpp` is now legacy compatibility only; new semantic call work
  must stay under `BirFunctionLowerer` and the split `lir_to_bir_*` lowering
  owners
- the next honest backend follow-on is to widen semantic call provenance and
  runtime/intrinsic lowering before the later `prepare` rebuild or
  prepared-BIR-only target reconnection ideas

## Non-Goals

- do not rebuild target ABI shaping in `lir_to_bir`
- do not revive direct raw-LIR fallback paths
- do not classify progress by testcase family names
- do not grow per-signature or per-arg-count special ladders as the call route
- do not use rendered LLVM text probes as proof of backend capability

## Working Model

- semantic BIR should encode call meaning and runtime intent before later
  prepare/legalize work
- callee provenance belongs in shared lowering:
  direct symbol, loaded pointer, aggregate-contained function pointer, and
  other already-supported shapes should normalize through one semantic call
  path
- runtime and intrinsic families such as `memcpy`, `memset`, `va_*`,
  `stacksave`, `stackrestore`, `abs`, and inline-asm placeholders should enter
  semantic BIR through explicit lowering decisions, not by falling out to
  unsupported text paths
- unsupported routes should be named by semantic family so later prepare or
  target work can consume a stable contract

## Ordered Steps

### 1. Re-establish the semantic call contract

Goal: make `BirFunctionLowerer` the clear owner of semantic call parsing and
signature meaning for the next widening work.

Primary target:

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir.hpp`

Concrete actions:

- audit the current minimal direct/non-variadic baseline and name the exact
  semantic responsibilities that still leak through compatibility helpers or
  ad hoc decoding
- keep new semantic call parsing and inference inside `BirFunctionLowerer`
  helpers instead of extending `call_decode.cpp`
- normalize how callees, argument shapes, return shapes, and hidden call
  metadata are represented before later ABI shaping
- preserve the already-accepted non-variadic baseline while making room for
  broader direct and indirect call forms

Completion check:

- another agent can point to one semantic call owner in the `lir_to_bir_*`
  layer
- widened call work no longer requires routing decisions to be rediscovered
  from testcase names
- a first proving slice can be stated as `build -> narrow backend subset`
  against this shared semantic contract

### 2. Broaden semantic call lowering beyond the current baseline

Goal: support the common direct and indirect call forms needed by the shared
backend route without slipping into ABI-specific legality work.

Primary target:

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- widen callee provenance handling for direct symbols, pointer values, and
  aggregate-contained function-pointer shapes that should already stay in
  semantic BIR
- decide which aggregate, vector, and pointer-difference forms belong in
  semantic BIR now versus later prepare work
- keep semantic call representation explicit in BIR so later prepare/legalize
  can own ABI and target legality decisions
- reject shortcuts whose practical effect is only to make one named call case
  pass

Completion check:

- common direct and indirect call forms enter semantic BIR without route
  escape
- remaining unsupported call routes are described by semantic family
- proof covers at least one nearby same-shape family, not just one testcase

### 3. Lower runtime and intrinsic families semantically

Goal: make runtime-facing operations enter semantic BIR honestly instead of
  bypassing the route.

Primary target:

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- add semantic lowering decisions for runtime/intrinsic families called out by
  the source idea, including `memcpy`, `memset`, `va_*`, `stacksave`,
  `stackrestore`, `abs`, and inline-asm placeholder routing where appropriate
- keep family handling grouped by semantic meaning rather than by the printed
  LLVM operation that exposed the miss
- decide which families should gain first-class BIR representation versus
  explicit semantic unsupported markers

Completion check:

- runtime/intrinsic-heavy functions can enter semantic BIR without escaping to
  raw-LIR or text-shaped fallback handling
- unsupported families are explicit and stable enough for follow-on prepare or
  target work
- narrow proof is backed by a fresh build, with broader backend validation
  added before accepting medium- or high-blast-radius slices

### 4. Tighten the semantic unsupported boundary

Goal: leave the backend with an honest semantic contract for what is still not
  implemented.

Primary target:

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/lowering/lir_to_bir.hpp`

Concrete actions:

- replace testcase-history-oriented unsupported reporting with semantic
  capability buckets
- keep planner-facing unsupported language aligned with what the backend
  actually reports
- checkpoint broad validation before treating this runbook as exhausted or
  complete

Completion check:

- remaining unsupported behavior is explicit by semantic family
- the source idea can be judged on capability coverage, not on testcase
  inventory churn
- later follow-on ideas can consume a stable semantic boundary
