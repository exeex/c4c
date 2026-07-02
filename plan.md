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

Steps 5-9 are exhausted, but the source idea is not complete. The direct
local-slot and pointer-addressed aggregate-store producer packets moved the
store-family representative for producer-owned reasons:

- `src/20001026-1.c` moved off semantic `store local-memory` admission and now
  fails downstream as `unsupported_instruction_fragment` in the RV64 object
  route.

The remaining representative failures are still in the local-memory semantic
producer-admission lane and stay under this source idea:

- `src/20000314-1.c`: `load local-memory` in `main`.
- `src/20000717-4.c`: `gep local-memory` in `x`.
- `src/20000519-1.c`: `scalar/local-memory` in `foo`.
- `src/20050604-1.c`: `alloca local-memory` in `foo`.

The next route must repair one of these remaining producer/admission families
without absorbing the downstream `src/20001026-1.c` object-lowering failure.
Start with the load-family representative because it is the largest remaining
classified family and still fails at semantic producer admission.

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

- Representative RV64 rows remain proof seeds, not the whole acceptance
  surface.
- Each remaining family should be repaired by making BIR publish or admit the
  semantic facts it already owns, not by downstream inference.
- Direct local-slot address facts and pointer-addressed aggregate leaf stores
  are already repaired enough to move the store representative. Do not keep
  routing `src/20001026-1.c` as an unchanged local-memory semantic failure.
- Load, GEP, scalar/local-memory, and alloca may share local-slot fact
  publication helpers, but each packet should name the concrete missing fact
  before implementation.
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

### Step 10: Inspect Load Local-Memory Producer Boundary

Goal: identify the missing or malformed producer facts behind the
`src/20000314-1.c` load-family representative.

Actions:

- Inspect `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log` and
  any available semantic/prepared BIR dumps for the failing `main` load.
- Trace the failing load through `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/`, especially the local-slot address,
  requested range, layout authority, and load-source admission paths.
- Compare the failing shape with existing focused BIR tests in
  `tests/backend/bir/`.
- Select the smallest focused BIR coverage gap that pins the load-family
  producer contract before implementation.

Completion check:

- `todo.md` names the exact load-family boundary, the focused BIR test to add
  or extend, and the RV64 representative command the supervisor should use
  after repair.

### Step 11: Repair Load Local-Memory Admission

Goal: publish or admit the selected load-family semantic facts in BIR without
moving the repair downstream.

Actions:

- Add or extend focused BIR tests for the load-family contract selected in
  Step 10.
- Implement the minimal producer-side repair in the BIR lowering/memory
  surfaces.
- Preserve semantic admission checks, diagnostics, prepared identity behavior,
  and existing store-family behavior.
- Run the delegated build and backend-focused test command after code or test
  changes.

Completion check:

- Focused BIR tests pass, the load-family fact is present or semantically
  excluded for the documented reason, and `todo.md` records the code surfaces
  touched plus any neighboring families intentionally left for later.

### Step 12: Prove Load Representative And Classify Residual

Goal: prove `src/20000314-1.c` moves off the same `load local-memory`
semantic admission failure for producer-owned reasons.

Actions:

- Run the supervisor-delegated RV64 backend-object command for
  `src/20000314-1.c`.
- Inspect the row `case.log` to confirm whether the old load semantic
  admission failure moved.
- If it moves to another in-scope local-memory semantic family, record that
  boundary in `todo.md`.
- If it moves downstream, keep that downstream issue out of this source idea
  unless the supervisor explicitly opens a separate route.

Completion check:

- `todo.md` records the exact command, result, current `case.log` outcome, and
  whether the next local-memory packet should target GEP, scalar/local-memory,
  alloca, or a shared boundary discovered by the load repair.

### Step 13: Repair The Next Remaining Semantic Family

Goal: continue with one coherent producer packet for the next still-failing
semantic local-memory family.

Actions:

- Choose exactly one of the remaining families based on Step 12 evidence:
  GEP `src/20000717-4.c`, scalar/local-memory `src/20000519-1.c`, alloca
  `src/20050604-1.c`, or a shared producer boundary that Step 12 proves covers
  more than one of them.
- Add focused BIR coverage before or with implementation.
- Keep target-specific inference, expectation changes, allowlist edits, and
  named-case shortcuts out of the repair.
- Run the delegated focused backend proof.

Completion check:

- `todo.md` records the selected family, the repaired producer fact, focused
  proof, and the remaining representative semantic-admission rows.

### Step 14: Reconcile Remaining Local-Memory Representatives

Goal: decide whether the source idea is complete, needs another runbook
checkpoint, or needs a separate idea for an actually distinct boundary.

Actions:

- Rerun the supervisor-delegated representative RV64 backend-object scan for
  the remaining local-memory rows.
- Compare current failures against the original load, GEP, store,
  scalar/local-memory, and alloca families.
- Confirm no expectation, unsupported-marker, allowlist, runtime comparison, or
  semantic admission weakening occurred.
- Request supervisor escalation when multiple local-memory packets have landed
  or shared helpers changed broadly.

Completion check:

- `todo.md` records which representative rows moved, which still fail in
  semantic local-memory admission, and whether this source idea should
  continue with another checkpoint, split a distinct initiative, deactivate, or
  close.
