# AArch64 Synthetic Select Label Uniqueness Runbook

Status: Active
Source Idea: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused `00143` compile/assembler blocker selected by the backend
inventory: duplicate synthetic select/materialized labels in generated AArch64
assembly.

## Goal

Make AArch64 synthetic select labels unique across all emitted select regions
so generated assembly cannot define the same `.Lselect_mat_*` true/end labels
more than once.

## Core Rule

Repair the general synthetic label allocation or emission boundary. Do not
special-case `00143`, one label suffix, one block, one selected instruction,
or one emitted assembly neighborhood.

## Read First

- `ideas/open/364_aarch64_synthetic_select_label_uniqueness.md`
- `todo.md`
- `test_after.log` from the Step 2 inventory classification
- `build/c_testsuite_aarch64_backend/src/00143.c.s`
- AArch64 select/materialized-label lowering and machine-printer label
  allocation code
- focused backend tests covering select or materialized-label emission

## Current Scope

- AArch64 synthetic labels used by select/materialized-label lowering
- duplicate label allocation, formatting, or emission within one function or
  assembly file
- focused backend coverage for multiple synthetic select regions
- representative proof for `c_testsuite_aarch64_backend_src_00143_c`

## Non-Goals

- Do not change implementation outside the synthetic label owner unless
  generated-code evidence requires it.
- Do not reopen basic block label ordering or epilogue placement from idea 352
  without fresh proof that it owns the duplicate synthetic labels.
- Do not reopen scalar-cast source-publication or the old `00143` structured
  register-source diagnostic.
- Do not repair later runtime value correctness under this idea until the
  duplicate-label assembler failure is gone and reclassified.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.

## Working Model

The current first bad fact is not missing block labels or fallthrough ordering.
Generated `00143.c.s` defines the same synthetic select true/end label names
more than once, so the assembler rejects the file before runtime. Treat the
owner as synthetic label allocation or naming for select/materialized-label
emission until focused evidence proves otherwise.

## Execution Rules

- Start from the duplicate `.Lselect_mat_*` labels in generated `00143.c.s`.
- Trace each duplicate name back to the selected instruction, block, or
  emission helper that created it.
- Add focused coverage before or with the repair so label collisions are
  guarded independently of `00143`.
- Preserve existing block-label ordering, scalar cast, aggregate writeback,
  and runtime behavior.
- If `00143` advances after the duplicate-label fix, record the new first bad
  fact in `todo.md` instead of expanding this idea silently.

## Steps

### Step 1: Localize Synthetic Label Collision

Goal: identify the exact allocation or naming boundary that reuses synthetic
select label names.

Primary target: generated `00143.c.s`, selected AArch64 records for the
matching select/materialized regions, and the relevant label helper code.

Actions:

- List the duplicated true/end labels and every definition site in the
  generated assembly.
- Map each definition site back to the selected node, block, instruction id,
  or emission helper that produced it.
- Determine whether the collision is caused by insufficient uniqueness inputs,
  counter reset, copied label records, or repeated formatting during emission.
- Confirm whether idea 352 block-label ordering is downstream or unrelated.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and the focused
  coverage shape needed for the repair.

### Step 2: Add Focused Collision Coverage

Goal: lock the duplicate synthetic label owner to a focused backend contract.

Primary target: backend tests or dump coverage that can force multiple
synthetic select/materialized-label regions in one emitted unit.

Actions:

- Add or extend focused coverage for at least two synthetic select regions
  whose labels would collide before the repair.
- Assert uniqueness of emitted labels or absence of duplicate definitions.
- Keep the test semantic, not tied to the exact `00143` label spelling.

Completion check:

- Focused coverage fails before the repair or directly guards the previously
  duplicated synthetic label allocation path.

### Step 3: Repair General Label Uniqueness

Goal: ensure synthetic select labels are unique wherever AArch64 emission uses
them.

Primary target: the allocation, naming, or emission helper localized in
Step 1.

Actions:

- Implement the smallest general uniqueness repair for synthetic
  select/materialized labels.
- Preserve deterministic assembly where practical.
- Avoid widening the fix into unrelated block traversal or runtime value
  repairs unless Step 1 proves that owner.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 no longer contains duplicate
  synthetic select labels for the covered shape.

### Step 4: Prove Representative And Classify Residual

Goal: prove `00143` advances past the duplicate-label assembler failure and
classify any next first bad fact.

Primary target: focused backend coverage and
`c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated build and focused proof command.
- Inspect generated `00143.c.s` for duplicate `.Lselect_mat_*` definitions.
- If `00143` still fails, classify the next first bad fact and decide whether
  it remains in this idea or needs lifecycle handoff.
- Ask the supervisor whether broader backend-regex or regression-guard proof
  is needed before closure.

Completion check:

- `00143` no longer fails for duplicate synthetic select labels, and any
  remaining failure is explicitly classified in `todo.md`.
