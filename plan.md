# AArch64 Local Aggregate Bit-Field Layout Publication Runbook

Status: Active
Source Idea: ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md
Activated From: ideas/closed/374_aarch64_local_aggregate_address_call_publication.md

## Purpose

Repair the focused residual exposed after local aggregate address call
publication was fixed: local aggregate bit-field stores must publish into the
same layout positions that callees load from.

## Goal

Make local aggregate bit-field and small-field writes use the correct AArch64
frame layout for scalar pointer consumers, beginning with `00218`.

## Core Rule

Repair local aggregate layout/store publication generally. Do not special-case
`00218`, `convs`, `convert_like_real`, `AMBIG_CONV`, one stack offset, one
field width, one register, or one emitted instruction sequence.

## Read First

- `ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md`
- `todo.md`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00218.c`
- generated `build/c_testsuite_aarch64_backend/src/00218.c.s`
- prepared BIR / layout records for `00218` `main` and `convert_like_real`
- `ideas/closed/374_aarch64_local_aggregate_address_call_publication.md`

## Current Scope

- local aggregate member and bit-field store publication
- agreement between stack-frame layout, aggregate member offsets, and callee
  loads through scalar pointer arguments
- representative proof for `c_testsuite_aarch64_backend_src_00218_c`
- adjacency classification only for `00216` if a shared aggregate layout fact
  becomes visible

## Non-Goals

- Do not reopen local aggregate address call publication; idea 374 closed that
  boundary.
- Do not broaden into `00216` compound-relocation or selected
  function-pointer-table lowering unless fresh evidence proves the same layout
  owner.
- Do not broaden into scalar constant/`sizeof` stack-home publication,
  external call-result publication, scalar FP materialization, dynamic-stack
  timeout repair, or shift-promotion timeout repair.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not implement under umbrella idea 295.

## Working Model

After idea 374, `00218` `main` correctly passes `&convs` to
`convert_like_real`. The new first bad fact is layout publication: generated
`main` stores `AMBIG_CONV` near `sp+2`, while `convert_like_real` reads and
masks the code field from `[x0,#16]`. Treat this as a local aggregate member
or bit-field layout/store boundary until localization proves otherwise.

`00216` is not the lead owner here. It already advanced past the first local
aggregate address calls and currently points at a later compound
relocation/function-pointer residual. Sample it only if the `00218` layout
work exposes a clearly shared aggregate layout fault.

## Execution Rules

- Start from the generated AArch64 and prepared records for `00218`.
- Compare the source aggregate layout, prepared offsets, frame slots, and
  generated stores before editing code.
- Add focused coverage for local aggregate bit-field or small-field stores
  before or with the repair.
- Keep the repair semantic; do not key on testcase filenames or emitted text.
- Preserve recent pointer-local, selected static/global value, scalar
  publication, and local aggregate address publication guardrails.

## Steps

### Step 1: Localize Local Aggregate Bit-Field Layout Gap

Goal: identify why the `AMBIG_CONV` store lands at a different frame position
than `convert_like_real` later reads.

Primary target: `00218` source, generated `00218.c.s`, and prepared aggregate
layout / frame-slot records for `main` and `convert_like_real`.

Actions:

- Inspect `union tree_node` and the `base.code` bit-field layout expected by
  the callee.
- Compare prepared member offsets against generated stores in `main`.
- Determine whether the mismatch is in semantic layout, BIR/prepared handoff,
  frame-slot allocation, bit-field store lowering, or AArch64 store
  publication.
- Decide focused backend coverage for the localized layout/store fact.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and focused
  coverage requirement for local aggregate bit-field layout publication.

### Step 2: Add Focused Layout Publication Coverage

Goal: guard the localized bit-field or small-field store publication fact
independently of the external representative.

Primary target: backend tests or dump coverage for local aggregate field
stores consumed through a scalar pointer call.

Actions:

- Add a focused case where a local aggregate field or bit-field is written,
  the aggregate address is passed to a scalar pointer consumer, and the
  consumer reads from the ABI/layout-consistent offset.
- Assert layout consistency without depending on one filename, field name,
  register, stack offset, or exact instruction spelling.
- Keep protected local aggregate address publication paths green.

Completion check:

- Focused coverage fails before the repair or directly guards the localized
  layout/store publication fact.

### Step 3: Repair Layout/Store Publication

Goal: publish local aggregate bit-field or small-field stores at the layout
offset consumed by scalar pointer readers.

Primary target: the semantic/prepared/MIR/AArch64 helper localized in Step 1.

Actions:

- Implement the smallest semantic repair at the owning layout or store
  publication boundary.
- Preserve idea 374 local aggregate address call publication behavior.
- Avoid broad aggregate, relocation, function-pointer, timeout, FP, external
  call, or scalar constant rewrites.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated `00218` no longer stores the code
  field at the mismatched local frame offset.

### Step 4: Prove Representative And Classify Residuals

Goal: prove `00218` advances or passes, then classify any next first bad fact
without broadening this owner silently.

Primary target: focused backend coverage and
`c_testsuite_aarch64_backend_src_00218_c`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00218.c.s` enough to confirm the store and callee load
  agree on layout.
- If `00218` still fails, classify the new first bad fact in `todo.md`.
- Sample `00216` only if the `00218` repair suggests a shared aggregate
  layout boundary.
- Ask the supervisor whether backend-regex or broader regression guard proof
  is needed before closure.

Completion check:

- `00218` no longer fails from the local aggregate bit-field layout/store
  mismatch, and any remaining residual is classified by a new first bad fact
  in `todo.md`.
