# BIR Local-Memory Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/557_bir_local_memory_semantic_producer_admission.md

## Purpose

Repair BIR local-memory semantic producer admission for the largest current
exact `semantic lir_to_bir` lane without letting RV64/MIR infer missing
producer facts.

## Goal

Publish the local-memory semantic facts needed by load, GEP, store,
scalar/local-memory, and alloca admission families, then prove representative
RV64 backend-object rows advance for producer-owned reasons.

## Core Rule

Do not claim progress from named-case shortcuts, downstream fact inference,
diagnostic rewrites, expectation changes, unsupported downgrades, allowlist
changes, or weakened semantic admission. Progress must come from BIR producer
facts that generalize across the current local-memory families.

## Read First

- `ideas/open/557_bir_local_memory_semantic_producer_admission.md`
- `todo.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_outcome.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/memory/`
- `tests/backend/bir/`

## Current Targets

- Exact local-memory semantic rows: `264`.
- Topic counts: `79` load, `62` GEP, `58` store, `49`
  scalar/local-memory, and `16` alloca.
- Representative RV64 proof seeds:
  - `src/20000314-1.c` for load.
  - `src/20000717-4.c` for GEP.
  - `src/20001026-1.c` for store.
  - `src/20000519-1.c` for scalar/local-memory.
  - `src/20050604-1.c` for alloca.
- Primary implementation surfaces:
  `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/`.

## Current Checkpoint

The first four-step route is exhausted, but the source idea is not complete.
Focused backend tests passed after publishing local-slot provenance, requested
range, layout authority, and known extent where available. The refreshed RV64
backend-object representative scan still passed `0/5` rows and failed `5/5`
rows with the same semantic admission families:

- `src/20000314-1.c`: load local-memory in `main`.
- `src/20000717-4.c`: GEP local-memory in `x`.
- `src/20001026-1.c`: store local-memory in `build_real_from_int_cst_1`.
- `src/20000519-1.c`: scalar/local-memory in `foo`.
- `src/20050604-1.c`: alloca local-memory in `foo`.

The next route must inspect the post-repair remaining admission boundary before
adding another producer packet.

Step 6 exposed that direct local-slot address publication is also a
prepared/route consumer compatibility boundary. The next packet must pin both
the producer-published direct local-slot facts and the prepared/codegen route
behavior that consumes or rejects those facts before any implementation is
reintroduced.

## Non-Goals

- Do not implement call metadata, runtime/intrinsic, scalar/signature/control,
  or bootstrap/global data-shape lanes in this runbook.
- Do not repair RV64/MIR by guessing address, provenance, or memory facts
  missing from BIR.
- Do not weaken semantic admission, expectations, unsupported markers,
  allowlists, or runtime comparison behavior.
- Do not use one testcase shape as proof for all local-memory families.
- Do not fold unrelated aggregate, publication, or prepared-contract claims
  into this lane without fresh evidence.

## Working Model

- The first repair published additional facts on local-slot `MemoryAddress`
  records, but direct same-slot scalar load/store paths that do not publish a
  `MemoryAddress` may still be outside that repair.
- Representative RV64 rows remain proof seeds, not the whole acceptance
  surface.
- Each remaining family should be repaired by making BIR publish or admit the
  semantic facts it already owns, not by downstream inference.
- Direct local-slot address facts may affect prepared/source identity and
  byval aggregate route decisions. Treat those consumers as part of the same
  local-memory semantic admission boundary when they are reacting to newly
  published producer facts.
- A producer-side exclusion is acceptable only when it preserves existing
  prepared/source identity contracts for semantic reasons such as slot
  ownership, not target, helper name, or testcase shape.
- If inspection proves a remaining family belongs to a distinct producer
  boundary outside local-memory semantic admission, stop and request a
  lifecycle split instead of absorbing that work.

## Execution Rules

- Keep routine packet progress, commands, row seeds, and proof notes in
  `todo.md`.
- Each code-changing packet needs fresh build proof plus the exact focused
  command delegated by the supervisor.
- Add or update focused BIR tests and prepared/route compatibility coverage
  before using RV64 representatives as proof when a producer fact changes route
  identity behavior.
- Use RV64 backend-object representative rows to confirm the current failure
  family moved for the producer reason.
- Keep `src/20001026-1.c` as a store-family representative proof seed, not as
  the implementation contract shape.
- Escalate validation when shared memory helpers affect more than one
  local-memory family.
- If a proposed fix changes expectations, unsupported markers, allowlists, or
  runtime comparison behavior, reject the route and request supervisor review.

## Steps

### Step 5: Reinspect Remaining Local-Memory Admission Boundary

Goal: identify the first remaining producer/admission fact boundary after the
local-slot `MemoryAddress` publication repair.

Actions:

- Inspect the five representative `case.log` files under
  `build/rv64_gcc_c_torture_backend/`.
- Compare those failures with the focused BIR coverage added by the completed
  producer repair.
- Inspect direct same-slot scalar load/store paths, local address publication,
  alloca modeling, and GEP range/provenance emission in
  `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/`.
- Choose one coherent next packet that can move at least one representative
  backend-object row without weakening semantic admission.

Completion check:

- `todo.md` names the selected family or shared boundary, the representative
  row to prove first, and the focused BIR test gap to add next.

### Step 6: Pin Direct Local-Slot Publication And Route Compatibility

Goal: define the selected direct local-slot address publication contract and
the prepared/route consumer behavior for those same facts before implementation
resumes.

Actions:

- Add or extend focused BIR coverage for direct local-slot address publication
  on same-slot scalar store/load paths, including provenance, requested range,
  known extent, layout authority, access kind, and local-slot identity where
  available.
- Add or extend prepared/codegen route compatibility coverage for the same
  facts, including byval aggregate and source-identity guardrails exposed by
  the blocked Step 6 attempt.
- State in the test or surrounding helper behavior whether prepared consumers
  should accept the newly published direct facts or reject/exclude them for a
  semantic slot-ownership reason.
- Keep `src/20001026-1.c` as the representative store-family proof seed only;
  do not encode target exclusions, helper names, allowlist changes, expectation
  rewrites, or RV64/MIR inference as part of the contract.
- Run the delegated build and backend-focused test command after any code or
  test changes.

Completion check:

- Focused BIR and prepared/route coverage together pin the contract for direct
  local-slot address facts, existing byval/source-identity expectations remain
  semantically explained, and `todo.md` records whether implementation can
  proceed in Step 7 or whether the route needs a lifecycle split.

### Step 7: Repair The Direct Local-Slot Producer/Consumer Boundary

Goal: make BIR publish or admit the selected direct local-slot semantic facts
and make prepared/route consumers handle those facts consistently without
bypassing admission checks.

Actions:

- Implement the minimal producer and route-consumer compatibility repair for
  the contract pinned in Step 6.
- Preserve semantic admission checks and diagnostics; do not move the repair
  into RV64/MIR.
- Preserve existing byval aggregate and source-identity behavior unless the
  Step 6 contract proves a semantic reason to change it.
- Keep unrelated local-memory families untouched unless code evidence proves
  they share the same helper or consumer boundary.
- Run the delegated focused backend proof.

Completion check:

- Focused BIR and prepared/route tests pass, direct local-slot facts are
  present or semantically excluded as specified by Step 6, and `todo.md`
  records any neighboring families intentionally left for later.

### Step 8: Prove One Representative Backend-Object Row

Goal: prove the selected representative row advances for producer-owned
reasons in the RV64 backend-object route.

Actions:

- Run the supervisor-delegated RV64 backend-object command for the selected
  representative row.
- Inspect the row `case.log` to confirm it no longer fails at the same
  semantic local-memory admission point.
- If the row advances to a different in-scope local-memory semantic admission
  family, record the next boundary in `todo.md`.
- If it advances to runtime comparison or a downstream prepared-object issue,
  ask the supervisor to choose broader validation or lifecycle handling.

Completion check:

- `todo.md` records the exact backend-object command, result, current
  `case.log` outcome, and whether the next packet should stay in the same
  family, broaden to a shared boundary, split, or close.

### Step 9: Reconcile The Five Representative Families

Goal: decide whether the repaired packets now satisfy the source idea or need a
new runbook checkpoint.

Actions:

- Rerun the five-row RV64 backend-object representative scan using the
  supervisor-provided allowlist and command.
- Compare the resulting failures against the five original local-memory
  families.
- Confirm no expectation, unsupported-marker, allowlist, runtime comparison, or
  semantic admission weakening occurred.
- Request supervisor escalation when multiple local-memory packets have landed
  or shared helpers changed broadly.

Completion check:

- `todo.md` records which representative rows moved, which remain in semantic
  local-memory admission, and whether this source idea should continue, split,
  deactivate, or close.
