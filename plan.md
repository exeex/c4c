# AArch64 Variadic HFA And Floating Residual Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Resume idea 326 from the post-369 umbrella classification and repair the
current AArch64 aggregate/varargs ABI call-boundary move gap represented by
`00140` and `00204`.

## Goal

Classify and repair composite/variadic call-boundary move preparation and
publication for AArch64, including the structured f128/q-register authority
path now blocking `00204`.

## Core Rule

Repair the semantic AArch64 call-boundary capability. Do not reopen older
idea-326 residuals or adjacent closed owners unless fresh generated-code
evidence moves the current first bad fact back to that exact boundary.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `todo.md`
- current `test_after.log` entries for:
  - `c_testsuite_aarch64_backend_src_00140_c`
  - `c_testsuite_aarch64_backend_src_00204_c`
- generated or prepared artifacts for `00140.c` and `00204.c` under
  `build/c_testsuite_aarch64_backend/`, when present
- nearby AArch64 call-boundary helpers in the backend before editing

## Current Scope

- AArch64 aggregate/varargs ABI call-boundary lowering for `00140` and `00204`
- selected/prepared call-boundary move nodes whose source or destination facts
  require GPR, scalar FPR, or structured f128/q-register authority
- composite argument publication across fixed and variadic call boundaries
- focused backend coverage for the repaired call-boundary owner

## Non-Goals

- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not special-case `00140.c`, `00204.c`, one struct name, one f128 value,
  one register, one stack slot, one vararg callsite, or one emitted
  instruction sequence.
- Do not reopen local/value-home publication, fixed-formal entry publication,
  byval lane publication, stdarg cursor/format, MOVI zero-extension, old
  HFA-output repairs, or prior selected-call-boundary work without direct
  evidence.
- Do not broaden into scalar comparison, indexed array/writeback,
  switch/select label ownership, file API result lowering, `sizeof`
  materialization, complex initializer layout, enum bit-field layout, or
  timeout residuals.

## Working Model

The fresh post-369 evidence selects idea 326 for a narrow current owner:
`00140` segfaults in struct-plus-variadic calls, and `00204` fails before
runtime with:

```text
deferred_unsupported: call-boundary move node requires prepared GPR registers, scalar FPR registers, or structured f128 q-register authority
```

Treat this as a call-boundary move preparation/publication gap, not as a
counts-based reopen of historical HFA, byval, fixed-formal, stdarg, MOVI, or
local/value-home routes.

## Execution Rules

- Start by localizing the first bad call-boundary fact in generated,
  prepared, or selected-machine artifacts.
- Preserve the distinction between fixed arguments, variadic extras,
  aggregate-by-value transport, scalar floating transport, HFA/HVA transport,
  and f128/q-register authority.
- Prefer focused backend tests that assert structured call-boundary facts
  before relying on the external c-testsuite representatives.
- Keep each code slice narrow enough for build proof plus focused target proof.
- Escalate to reviewer or lifecycle handoff if evidence shows the current
  owner is actually one of the parked residual buckets.

## Steps

### Step 1: Localize the Current Call-Boundary Gap

Goal: identify the concrete prepared or selected-machine fact missing for the
current `00140` / `00204` failures.

Primary target: `test_after.log`, generated artifacts, prepared dumps, and
selected-machine diagnostics for `00140.c` and `00204.c`.

Actions:

- Inspect the failing diagnostics and available generated artifacts for both
  representatives.
- For `00204`, trace the unsupported call-boundary move to the source value,
  destination authority, and expected GPR/FPR/q-register preparation.
- For `00140`, identify the first bad call or callee boundary that leads to
  the struct-plus-variadic segfault.
- Record whether the current owner is structured f128/q-register preparation,
  composite GPR/FPR lane publication, variadic overflow placement, or another
  directly evidenced call-boundary path.

Completion check:

- `todo.md` names the first bad fact, the owning backend boundary, and any
  adjacent owners rejected by generated-code or diagnostic evidence.

### Step 2: Add Focused Coverage For The Owner

Goal: lock the repaired behavior to a focused backend contract before touching
broad c-testsuite expectations.

Primary target: the smallest backend/unit tests that can assert the missing
call-boundary preparation or publication facts.

Actions:

- Add or extend focused backend coverage for the localized call-boundary move
  owner.
- Cover representative GPR, scalar FPR, and structured f128/q-register facts
  only where they are part of the localized owner.
- Include a negative guard against falling back to an unstructured or
  unsupported call-boundary move node.
- Keep external `00140` / `00204` proof as representative integration proof,
  not the only test of the repair.

Completion check:

- Focused tests fail before the repair or directly assert the previously
  missing facts, and they are narrow enough to reject testcase-shaped fixes.

### Step 3: Repair General Call-Boundary Preparation

Goal: make the selected/prepared AArch64 call-boundary path publish the
required structured facts for the localized owner.

Primary target: AArch64 backend call-boundary preparation, move publication,
and machine-printer consumption code identified in Step 1.

Actions:

- Implement the smallest general repair for the localized authority gap.
- Preserve existing scalar GPR, scalar FPR, byval aggregate, fixed-formal,
  stdarg, MOVI, and local/value-home behavior.
- Avoid adding named testcase branches or emitted-text matching.
- Run a build or compile proof before focused runtime proof.

Completion check:

- The focused backend tests pass, and no older supported call-boundary path is
  downgraded or bypassed.

### Step 4: Prove Representatives And Classify Residuals

Goal: prove the repair on the current external representatives and decide
whether idea 326 can close or needs another focused handoff.

Primary target:
`c_testsuite_aarch64_backend_src_00140_c` and
`c_testsuite_aarch64_backend_src_00204_c`.

Actions:

- Run the supervisor-delegated focused proof command for the representative
  tests and any focused backend tests added in Step 2.
- If `00140` or `00204` advances to a new first bad fact, classify it against
  idea 326's source scope before continuing.
- If the new first bad fact is outside this idea, record the handoff in
  `todo.md` and request lifecycle transition instead of expanding this plan.
- Ask the supervisor whether broader backend-regex or regression-guard proof
  is needed before closure.

Completion check:

- The representative outcome is either passing under the repaired owner or a
  new, explicitly classified first bad fact with lifecycle-ready handoff notes.
