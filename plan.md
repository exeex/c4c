# AArch64 Local Aggregate Copy Load Publication Runbook

Status: Active
Source Idea: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Switched From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the current non-timeout AArch64 backend residual where a local
aggregate copy or pointer-derived load consumes an uninitialized frame-slot
value inside `00216.c` `foo`.

## Goal

Publish the correct local aggregate address or copied value before generated
AArch64 loads from copied local aggregate state, then prove the current `foo`
segfault advances without filename-shaped matching.

## Core Rule

Fix the semantic/backend lowering rule for local aggregate copy or
load-local-memory publication. Do not special-case `00216`, `foo`, one stack
offset, one register, one aggregate type, or the observed `ldrb` neighborhood.

## Read First

- `ideas/open/379_aarch64_local_aggregate_copy_load_publication.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00216.c`
- `build/c_testsuite_aarch64_backend/src/00216.c.s`
- recent local aggregate publication coverage near the closed idea 374 route

## Current Scope

- the current first bad fact in `c_testsuite_aarch64_backend_src_00216_c`
- local aggregate initializers and copies inside `foo`
- pointer-derived loads such as `const struct S *pls = &ls` and
  `struct S ls21 = *pls`
- wrapper/subobject aggregate loads such as `(struct S)w->t.s` and
  `((const struct W *)w)->t.t`
- AArch64 frame-slot publication for local aggregate copy/load-local-memory
  paths

## Non-Goals

- Do not reopen closed idea 374 unless fresh evidence again shows stale call
  operands for `&local_aggregate`.
- Do not include compound relocation, global relocation, or
  function-pointer-table dispatch work after `foo` unless the current crash
  advances and fresh evidence proves shared ownership.
- Do not include timeout-only `00200` or `00207` routes.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, CTest registration, proof-log policy, or external
  test contracts.
- Do not claim progress through helper renames, emitted-text reshuffling, or
  classification-only changes.

## Working Model

The post-378 inventory reports 358 backend-regex selected tests, 355 passed,
and 3 residual failures. Local backend/unit tests are clean. The only
non-timeout residual is `c_testsuite_aarch64_backend_src_00216_c`; `00200` and
`00207` are timeout-only parked buckets.

Focused proof reproduces a runtime segfault inside `00216.c` `foo` before
`test_compound_with_relocs` or `test_multi_relocs` can execute. The observed
generated sequence loads a pointer through stack slots derived from
`ldr x13, [sp, #496]` without a prior local initialization on the crashing
path, then executes `ldrb w9, [x10]`.

Treat `00216` as a representative and advancement proof, not as the only
contract. Add or use focused backend coverage that demonstrates the same local
aggregate copy/load-local-memory publication rule independently of the
external filename.

## Execution Rules

- Localize the first missing publication boundary before changing lowering.
- Trace the value through semantic BIR, prepared BIR, MIR, and generated
  AArch64 stack slots as needed.
- Prefer existing aggregate/local-memory helper patterns over a new one-off
  path.
- Keep proof narrow while repairing, then run the supervisor-delegated broader
  guard once the focused failure advances.
- If the first bad fact changes after repair, classify the new failure in
  `todo.md` and stop for supervisor routing when it leaves this source idea.

## Steps

### Step 1: Localize First Bad Fact

Goal: identify the exact local aggregate copy, pointer-derived source,
prepared value, MIR handoff, or frame-slot publication boundary that feeds the
uninitialized pointer consumed by the current `foo` `ldrb`.

Primary target: `tests/c/external/c-testsuite/src/00216.c`,
`build/c_testsuite_aarch64_backend/src/00216.c.s`, and compiler dumps for the
representative path.

Actions:

- Reproduce or inspect the current `foo` crash path and map the failing
  generated instructions back to the source-level aggregate operation.
- Trace the value publication through semantic BIR, prepared BIR, MIR, and
  AArch64 frame-slot assignment.
- Identify whether the missing publication is an aggregate address, copied
  aggregate value, pointer-derived subobject load, or wrapper-subobject load.
- Record the first bad fact and the smallest focused backend shape needed to
  cover it.

Completion check:

- `todo.md` names the exact publication boundary and the focused coverage
  shape to add or update before repair.

### Step 2: Add Focused Coverage

Goal: create backend coverage for the local aggregate copy/load-local-memory
publication rule without depending on `00216.c` by name.

Primary target: the repo's focused backend/AArch64 tests for aggregate local
memory publication.

Actions:

- Add a minimal focused case for the localized local aggregate copy or
  pointer-derived load shape.
- Keep the test semantic enough to reject filename, stack-offset, register, or
  exact-instruction matching.
- Verify the focused case fails before the repair for the same missing
  publication reason.
- Avoid expectation, runner, timeout, allowlist, CTest, or proof-log changes.

Completion check:

- Focused coverage exists and fails before the repair for a local aggregate
  copy/load-local-memory publication gap.

### Step 3: Repair Publication

Goal: make AArch64 lowering publish the needed local aggregate address or value
before copied aggregate state or pointer-derived subobject loads consume it.

Primary target: the localized semantic/backend lowering boundary from Step 1.

Actions:

- Apply the smallest general repair at the publication boundary.
- Reuse local aggregate/local-memory helper conventions already present in the
  backend.
- Preserve closed idea 374 address-call behavior and nearby local backend/unit
  tests.
- Keep relocation and function-pointer-table observations parked until `foo`
  advances.

Completion check:

- The focused coverage from Step 2 passes without special-casing names,
  offsets, registers, or emitted instruction neighborhoods.

### Step 4: Prove Advancement And Reclassify

Goal: prove `00216` advances past the current `foo` segfault and classify any
new first bad fact without expanding this owner silently.

Primary target: supervisor-delegated focused and broader backend proof.

Actions:

- Run the supervisor-delegated build and focused backend test command.
- Confirm `c_testsuite_aarch64_backend_src_00216_c` no longer crashes at the
  current `foo` `ldrb w9, [x10]` through an uninitialized stack-slot-derived
  pointer.
- If `00216` still fails, classify the new first bad fact in `todo.md`.
- Stop for lifecycle routing if the new owner is relocation,
  function-pointer-table dispatch, `00200`, `00207`, or another parked bucket.

Completion check:

- `todo.md` records focused proof, broader proof if delegated, and either
  acceptance for idea 379 or a new first-bad-fact handoff note.

### Step 5: Repair Same-Owner Pointer-Derived Copy Mismatch

Goal: repair the remaining same-owner `foo` local aggregate copy mismatch
where `struct S ls21 = *pls` reads from `pls = &ls` but corrupts trailing
bytes after the prior segfault repair.

Primary target: the local aggregate copy/load-from-local-address lowering
boundary that feeds `struct S ls21 = *pls`, plus the focused backend coverage
from Steps 2 and 3.

Actions:

- Localize why the `pls = &ls` source address or copied aggregate value
  produces `ls21` bytes other than `1 2 3 4` after the address-publication
  repair.
- Decide whether the remaining mismatch is a copy-width, byte-layout,
  load-from-local-address, prepared-value, MIR handoff, or AArch64 frame-slot
  publication problem.
- Repair the general local aggregate copy/load publication path without
  special-casing `00216`, `foo`, `ls21`, `struct S`, stack offsets, registers,
  or output text.
- Keep later `lv3`, `flow`, relocation, and function-pointer-table/table
  observations parked unless fresh proof makes one of them the first bad fact.
- Re-run the supervisor-delegated focused proof and preserve the existing
  local aggregate address-call guardrails selected by the supervisor.

Completion check:

- Focused local aggregate publication coverage passes, `ls21` copies
  `1 2 3 4` from `pls = &ls`, and `todo.md` records either acceptance for
  idea 379 or the next first bad fact with its owner classified.
