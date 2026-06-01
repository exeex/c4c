# AArch64 Call And Publication Prepared Authority Cleanup Runbook

Status: Active
Source Idea: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md

## Purpose

Make AArch64 call and publication lowering consume prepared call,
move-bundle, after-call result, and publication source facts directly before
target-local record construction.

Goal: remove local re-derivation of call/publication authority while preserving
AArch64 register conversion, copy instruction selection, call spelling, and
machine-record construction.

## Core Rule

Consume existing prepared facts as authority; do not replace local
re-derivation with new target-local policy or testcase-shaped shortcuts.

## Read First

- `ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/prealloc/call_plans.*`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/publication_plans.*`

## Current Targets

- call-plan lookup and argument/result binding
- call-boundary move bundles
- after-call result lane bindings
- edge publication and edge-copy source facts
- current-block join parallel copy sources
- typed stack-source publication
- store-source publication

## Non-Goals

- Do not move AArch64 ABI register conversion, direct/indirect call spelling,
  copy instruction selection, or call-boundary record construction into shared
  prealloc.
- Do not rewrite the shared call-plan or publication-plan model when the
  needed fact already exists.
- Do not reopen broad dispatch-family contraction or line-count cleanup.
- Do not weaken tests, diagnostics, supported-path expectations, or unsupported
  markers to make this route pass.

## Working Model

- Shared prepared authority names the semantic facts.
- AArch64 code converts those facts into target registers, moves, branches,
  calls, stores, and machine records.
- Shared prealloc changes are allowed only to expose or consume an existing
  prepared fact cleanly; they are not a place for AArch64 instruction policy.

## Execution Rules

- Start each step by identifying the prepared fact that should own the
  decision.
- Prefer one narrow migration per packet, with focused build or CTest proof.
- If a needed publication ordering fact is missing, stop and report that as a
  candidate handoff to the publication-ordering probe instead of inventing a
  broad local workaround.
- Keep `todo.md` as the mutable packet state; update this runbook only for a
  real route correction.
- Before accepting a code slice, prove behavior through focused AArch64 call
  and publication tests and preserve regression-log expectations requested by
  the supervisor.

## Ordered Steps

### Step 1: Map Prepared Call And Publication Facts

Goal: establish the exact prepared queries AArch64 should consume before
editing call or publication lowering.

Primary targets:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/prealloc/call_plans.*`
- `src/backend/prealloc/publication_plans.*`
- `src/backend/prealloc/prepared_lookups.*`

Concrete actions:
- Inspect local call-plan lookup, argument/result binding, move-bundle,
  after-call result, edge publication, edge-copy source, join-source,
  typed-stack-source, and store-source decisions.
- Match each local decision to an existing prepared fact named in the source
  idea.
- Record any missing or ambiguous authority in `todo.md`; do not add a new
  query unless the existing prepared surface is proven insufficient.

Completion check:
- `todo.md` identifies the first narrow migration target and any missing
  authority without changing implementation files.

### Step 2: Consume Prepared Call Plans And Boundary Move Facts

Goal: make call lowering consume prepared call plans, argument plans,
move bundles, and after-call result lane bindings.

Primary target:
- `src/backend/mir/aarch64/codegen/calls.cpp`

Concrete actions:
- Replace local call-plan and argument/result re-derivation with
  `PreparedCallPlan`, `PreparedCallArgumentPlan`,
  `PreparedMoveBundle`, and
  `PreparedAfterCallResultLaneBinding` queries where those facts already
  exist.
- Keep AArch64 register conversion, scratch/register spelling, direct and
  indirect call emission, and call-boundary machine records local.
- Add or adjust focused tests only to prove prepared call facts drive AArch64
  lowering.

Completion check:
- Focused build or CTest subset covers call lowering and call-boundary move or
  result publication, and no target-local helper still owns the migrated
  call-plan decisions.

### Step 3: Consume Prepared Edge Publication Sources

Goal: make dispatch publication consume prepared edge and edge-copy source
facts before target-local copy/move record construction.

Primary target:
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

Concrete actions:
- Route edge publication source selection through
  `PreparedEdgePublication*`, `PreparedEdgeCopySourceFacts`,
  `PreparedCurrentBlockJoinParallelCopySourceFacts`, and
  `PreparedEdgePublicationSourceProducer*` as applicable.
- Preserve AArch64 copy instruction selection and machine-record construction
  locally.
- Avoid changing publication ordering unless existing prepared facts already
  provide that ordering; otherwise record the blocker for the ordering probe.

Completion check:
- Focused proof covers edge-copy publication and current-block join sources,
  and the diff does not introduce named-case publication shortcuts.

### Step 4: Exhaust In-Scope Typed Stack And Store-Source Publication Facts

Goal: finish the active source idea's owned Step 4 surface without expanding
the runbook into out-of-scope edge-copy ownership.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- call or shared prepared lookup sites only where needed

Concrete actions:
- Treat the `dispatch_publication.cpp` Step 4 audit as the in-scope packet:
  store-source publication already consumes prepared store-source plans there.
- Do not thread `PreparedEdgePublication` into
  `dispatch_publication.cpp` solely to create a typed stack-source consumer.
- Record the typed stack-source edge-publication owner as separate follow-up
  work when the viable consumer is in
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.
- Keep target-local address/register conversion and instruction selection in
  AArch64 code.

Completion check:
- `todo.md` records that Step 4 is exhausted for active owned files, names the
  follow-up idea for `dispatch_edge_copies.cpp`, and execution advances to
  Step 5 consolidation.

### Step 5: Consolidate Proof And Residue Audit

Goal: verify the cleanup satisfies the source idea and leaves only
target-local AArch64 residue.

Primary targets:
- migrated call and publication files
- focused tests or dumps proving prepared fact consumption

Concrete actions:
- Re-scan the touched AArch64 helpers for remaining local call/publication
  authority duplication.
- Confirm Step 4 residue is limited to the out-of-scope typed stack-source
  edge-copy owner recorded in the follow-up idea, not a missed
  `dispatch_publication.cpp` migration.
- Confirm any shared prealloc edits expose existing facts rather than moving
  AArch64 policy into shared code.
- Run the supervisor-selected broader or regression guard proof when the slice
  is an acceptance candidate.

Completion check:
- `todo.md` records focused proof and any broader regression-log result, with
  no weakened expectations, unsupported downgrades, broad dispatch rewrites, or
  local re-derivation hidden behind renamed helpers.
