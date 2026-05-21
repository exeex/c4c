# AArch64 Local Aggregate Address Call Publication Runbook

Status: Active
Source Idea: ideas/open/374_aarch64_local_aggregate_address_call_publication.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused local aggregate address call-argument publication residual
selected by the backend inventory.

## Goal

Make address-of local aggregate values publish to scalar pointer call
arguments, beginning with `00218` and guarded by `00216`.

## Core Rule

Repair local aggregate address call publication generally. Do not special-case
`00218`, `00216`, one local aggregate name, one function name, one register,
one stack offset, or one emitted instruction sequence.

## Read First

- `ideas/open/374_aarch64_local_aggregate_address_call_publication.md`
- `todo.md`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00218.c`
- `tests/c/external/c-testsuite/src/00216.c`
- generated:
  - `build/c_testsuite_aarch64_backend/src/00218.c.s`
  - `build/c_testsuite_aarch64_backend/src/00216.c.s`
- prepared BIR for `00218` `main` and `convert_like_real`
- prepared BIR for `00216` `foo`

## Current Scope

- address-of local aggregate values used as scalar pointer call arguments
- publication of local aggregate addresses at direct-call boundaries
- representative proof for `c_testsuite_aarch64_backend_src_00218_c`
- crash-guard classification for `c_testsuite_aarch64_backend_src_00216_c`

## Non-Goals

- Do not broaden into scalar constant/`sizeof` stack-home publication,
  external call-result publication, scalar FP materialization, dynamic-stack
  timeout repair, shift-promotion timeout repair, bit-field semantics, or
  broad aggregate initializer/relocation work.
- Do not reopen closed direct-call or address-valued publication owners from
  counts alone.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not implement under umbrella idea 295.

## Working Model

`00218` currently fails before bit-field semantics matter: `main` passes stale
`x21` to `convert_like_real` instead of materializing `&convs`. `00216` has an
early analogous crash guard: `foo` passes stale `x13` to `print(ls)` instead
of materializing `&ls`. Treat this as local aggregate address publication to
call arguments until localization proves otherwise.

## Execution Rules

- Start from generated AArch64 and prepared records for the failing callsites.
- Trace local aggregate homes and address-valued call operands before editing
  code.
- Compare against closed direct-call and address-valued publication owners so
  this route does not reopen from counts alone.
- Add focused coverage before or with the repair.
- If `00216` advances to initializer or function-pointer-table facts, record
  that residual in `todo.md` instead of broadening this owner silently.

## Steps

### Step 1: Localize Local Aggregate Address Publication Gap

Goal: identify where `&convs` / `&ls` address values stop reaching scalar
pointer call arguments.

Primary target: generated `00218.c.s`, generated `00216.c.s`, and prepared
call operand records for the relevant callsites.

Actions:

- Inspect `00218` `main` around `convert_like_real(&convs)`.
- Inspect `00216` `foo` around the first `print(ls)` call.
- Trace whether local aggregate addresses are present in semantic/prepared
  state and where they are lost before call-argument publication.
- Identify whether stale call carriers are register-state, stack-home, MIR
  handoff, or AArch64 lowering issues.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and focused
  coverage requirement for local aggregate address call publication.

### Step 2: Add Focused Local Aggregate Address Coverage

Goal: guard address-of local aggregate publication to scalar pointer call
arguments independently of external representatives.

Primary target: backend tests or dump coverage for local aggregate address
call operands.

Actions:

- Add coverage where a local aggregate address is passed to a direct call that
  expects a scalar pointer argument.
- Assert the call argument materializes the local aggregate address, not a
  stale scratch or callee-saved carrier.
- Keep coverage semantic and not tied to one filename, aggregate name,
  register, stack offset, or emitted instruction sequence.

Completion check:

- Focused coverage fails before the repair or directly guards the localized
  local aggregate address publication fact.

### Step 3: Repair Local Aggregate Address Call Publication

Goal: publish address-of local aggregate values into scalar pointer call
arguments.

Primary target: the semantic/prepared/MIR/AArch64 helper localized in Step 1.

Actions:

- Implement the smallest semantic repair at the owning publication boundary.
- Preserve recent pointer-local, selected-value, scalar publication, and
  direct-call guardrails.
- Avoid broad rewrites of unrelated constant, external-call, FP, timeout,
  bit-field, initializer, or relocation owners.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 no longer passes stale
  carriers for `&convs` or the analogous local aggregate address shape.

### Step 4: Prove Representative And Classify Guard

Goal: prove `00218` advances and classify whether `00216` shares the same
boundary or moves to a new first bad fact.

Primary target: focused backend coverage,
`c_testsuite_aarch64_backend_src_00218_c`, and
`c_testsuite_aarch64_backend_src_00216_c`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00218.c.s` enough to confirm `&convs` reaches
  `convert_like_real`.
- Sample `00216` enough to classify whether `print(ls)` now receives `&ls` or
  whether a new initializer/function-pointer-table first bad fact remains.
- If either representative still fails, classify the new first bad fact in
  `todo.md`.
- Ask the supervisor whether backend-regex or broader regression guard proof
  is needed before closure.

Completion check:

- `00218` no longer fails from stale local aggregate address publication, and
  `00216` is either advanced by the same owner or reclassified by a new first
  bad fact in `todo.md`.
