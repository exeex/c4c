# Typed/Aggregate Branch Stack-Source Publication Runbook

Status: Active
Source Idea: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Activated From: ideas/open/592_typed_aggregate_branch_stack_source_publication.md

## Purpose

Define and implement the narrow typed/aggregate producer contract that lets
branch `Lhs` or `Rhs` stack-load consumers rely on explicit shared-prealloc
freshness at the branch terminator point.

## Goal

Move one blocked pointer or aggregate-adjacent branch stack-load consumer from
inventory-only `policy=none` to selected `BranchStackLoadSource` freshness
authority, without target-local inference or stack-home-only acceptance.

## Core Rule

Only producer-published `PreparedValueFreshnessSourceKind::BranchStackSlot`
facts proven at the exact branch terminator point may authorize
`PreparedValueFreshnessUseKind::BranchStackLoadSource`; prepared homes, frame
slots, aggregate lanes, and clobber-safety facts are context, not freshness
authority.

## Read First

- `ideas/open/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- Shared-prealloc branch stack-source producer facts.
- Branch terminator freshness publication for typed and aggregate-adjacent
  sources.
- One blocked branch stack-load consumer, preferably pointer `Lhs` or pointer
  `Rhs`, that currently remains inventory-only.
- Focused backend tests and/or prepared dumps for accepted and fail-closed
  authority cases.

## Non-Goals

- Do not migrate every branch, select, edge-publication, RV64, AArch64, x86, or
  target-emission consumer.
- Do not infer freshness in target branch emission.
- Do not redesign control-flow lowering, instruction selection, terminator
  emission, ABI classification, or physical register identity policy.
- Do not treat prepared homes, frame slots, aggregate lanes, or clobber-safety
  facts as freshness by themselves.
- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist edits, or runtime output changes.

## Working Model

- Idea 590 established that branch stack-load authority is selected through
  `PreparedValueFreshnessUseKind::BranchStackLoadSource`,
  `PreparedValueFreshnessSourceKind::BranchStackSlot`,
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering`, and
  `PreparedValueFreshnessSourceRank::BranchStackSlot`.
- The scalar condition route is already authorized by selected shared
  freshness.
- Pointer `Lhs` and `Rhs` rows are visible but remain blocked until a producer
  contract publishes branch stack-slot freshness for those source roles.
- This runbook must first make the producer/publication rule explicit, then
  migrate only the narrowest consumer that the rule actually authorizes.

## Execution Rules

- Keep changes inside shared-prealloc producer/publication and matching tests
  unless a step explicitly proves another surface is required.
- Preserve fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.
- Prefer semantic producer rules tied to branch terminator ordering over
  testcase-shaped matching.
- Use diagnostics or prepared dumps to make selected and rejected freshness
  visible.
- For each code-changing step, run at least:

```bash
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Step 1: Audit Typed And Aggregate Branch Stack-Source Producers

Goal: identify producer paths that can legitimately feed branch condition,
`Lhs`, or `Rhs` stack-load consumers.

Primary targets:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- existing branch stack-load freshness tests under `tests/backend/bir/`

Actions:
- Inspect current `PreparedBranchStackLoadRole::{Condition,Lhs,Rhs}` planning
  and collection paths.
- Identify typed pointer and aggregate-adjacent source facts that reach branch
  stack-load inventory rows.
- Separate facts that can publish branch freshness from structural context such
  as stack homes, frame slots, aggregate lanes, and clobber-safety.
- Record the narrowest consumer candidate, preferably pointer `Lhs` or `Rhs`,
  that can be migrated without broad target changes.

Completion check:
- The executor can name the producer paths, the blocked consumer candidate, and
  the exact facts that are allowed or forbidden to publish branch stack-slot
  freshness.

## Step 2: Define The Producer Publication Contract

Goal: encode or document in code-adjacent structure which typed/aggregate
producer facts may publish `BranchStackSlot` freshness for
`BranchStackLoadSource`.

Primary targets:
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/value_locations.hpp` only if existing vocabulary is
  insufficient

Actions:
- Reuse `BranchStackLoadSource`, `BranchStackSlot`, and
  `BranchTerminatorOrdering` when sufficient.
- Add the smallest helper or data path needed for eligible typed/aggregate
  producers to publish branch-point source freshness.
- Ensure publication is tied to the exact branch terminator block/instruction
  point.
- Keep producer authority separate from stack-home completion and target-local
  branch emission.

Completion check:
- Eligible producers can create explicit branch stack-slot freshness; ineligible
  structural facts cannot create authority by themselves.

## Step 3: Migrate One Blocked Branch Stack-Load Consumer

Goal: move one previously blocked pointer or aggregate-adjacent branch
stack-load consumer from inventory-only status to selected shared freshness
authority.

Primary targets:
- `src/backend/prealloc/publication_plans.cpp`
- focused tests for `PreparedBranchStackLoadRole::Lhs` or `Rhs`

Actions:
- Choose the smallest consumer from Step 1 that has producer-published branch
  freshness.
- Require selected shared freshness before reporting the consumer available.
- Keep all other branch, select, edge, and target consumers out of scope.
- Preserve current blocked behavior for consumers whose producer facts are not
  covered by the new contract.

Completion check:
- At least one previously blocked `Lhs`, `Rhs`, or aggregate-adjacent branch
  stack-load route becomes available only when selected branch freshness is
  present.

## Step 4: Prove Accepted And Fail-Closed Cases

Goal: add focused proof for both the accepted route and negative authority
cases.

Primary targets:
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- prepared printer or dump tests only if needed for visible status

Actions:
- Prove the accepted route is authorized by explicit producer-published
  `BranchStackSlot` freshness.
- Prove missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
  stack-home-only authority fail closed.
- Keep existing 587, 588, 589, and 590 freshness coverage passing.
- Avoid expectation rewrites that weaken an existing contract.

Completion check:
- Focused tests fail without the new producer-published freshness and pass with
  the semantic publication rule.

## Step 5: Backend Proof And Closure Inventory

Goal: validate the slice and leave lifecycle-ready completion notes in
`todo.md`.

Actions:
- Run the backend proof command selected by the supervisor, normally:

```bash
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Summarize in `todo.md` which producers were audited, which consumer was
  migrated, which fail-closed cases are covered, and which consumers remain
  deliberately blocked.
- Do not move or close the source idea; closure is a separate plan-owner
  decision after supervisor validation.

Completion check:
- The proof log is green, `todo.md` contains the closure inventory, and no
  out-of-scope target or expectation-only changes are needed to claim progress.
