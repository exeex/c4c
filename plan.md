# Semantic BIR Pointer-Derived String Loads Runbook

Status: Active
Source Idea: ideas/open/356_semantic_bir_pointer_derived_string_loads.md

## Purpose

Reactivate the semantic pointer-derived string-load source only long enough to
verify whether its parked first bad fact has returned. Do not assume the
historical `00173` dynamic byte-load failure is still current.

## Goal

Repair semantic BIR pointer-derived byte loads only if fresh evidence shows
loads through incremented pointer values are again lowered as fixed global
string-byte loads.

## Core Rule

Do not implement from stale parked evidence. Step 1 must refresh the current
first bad fact and either confirm this semantic-BIR owner has a live failure or
route the plan back to lifecycle handling.

## Read First

- `ideas/open/356_semantic_bir_pointer_derived_string_loads.md`
- The parked outcome in that source idea, especially the note that commit
  `96b80ee21` repaired dynamic pointer-derived byte loads and split the
  remaining `00173` failure to string literal pointer-address publication.
- Semantic BIR, prepared BIR, and generated AArch64 artifacts for `00173` only
  as evidence, not as a testcase-specific target.

## Current Targets / Scope

- Semantic BIR lowering for loads through pointer values derived from strings
  or arrays, such as incremented `char *` carriers.
- Preservation of the current pointer value as the memory base for
  pointer-derived byte loads.
- Focused semantic or backend-route coverage proving dynamic pointer-derived
  loads do not collapse to fixed `LoadGlobalInst @.str0` byte loads.
- Representative proof through `c_testsuite_aarch64_backend_src_00173_c` only
  after the semantic owner is confirmed.

## Non-Goals

- Do not reopen AArch64 address-valued memory or call-argument publication from
  idea 355.
- Do not absorb string literal pointer-value publication, recursive
  pointer-formal home publication, scalar formal preservation, frontend
  admission, ABI composite, variadic/floating, or dynamic-stack work unless
  fresh evidence proves this source idea is the wrong owner and lifecycle state
  is switched.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Do not special-case `00173`, one string literal, one global symbol, one loop
  body, one source variable name, or one emitted instruction sequence.

## Working Model

The historical owner was a semantic-BIR value-modeling boundary: loads such as
`*b` and `*src` lost their dynamic pointer base before AArch64 lowering and
became fixed global string-byte loads. The parked outcome says this owner was
repaired, and the remaining representative failure moved to publication of the
string literal pointer value itself.

This activation must first prove whether the dynamic pointer-derived load
collapse is again present. If the focused representative is green or fails for
a different owner, the executor should classify that fact in `todo.md` and
return for lifecycle routing.

## Execution Rules

- Keep routine execution state in `todo.md`.
- Preserve source idea stability; update the source only for durable lifecycle
  decisions such as deactivation, closure, or a true source-intent correction.
- Prefer focused semantic-BIR or backend-route evidence before using the
  external representative as acceptance proof.
- Treat expectation rewrites, unsupported downgrades, or named-case shortcuts
  as route failure, not progress.
- Every code-changing step needs fresh build proof plus the supervisor-selected
  focused CTest subset.

## Steps

### Step 1: Refresh Current First Bad Fact

Goal: determine whether the semantic pointer-derived string-load failure is
currently live.

Concrete actions:

- Build the current tree.
- Run the supervisor-selected focused proof for `00173` and any nearby
  existing semantic/backend pointer-derived byte-load guardrails.
- If `00173` passes, report that no current representative first bad fact
  remains for this idea.
- If `00173` fails, inspect current semantic BIR, prepared BIR, and generated
  AArch64 enough to classify the first bad fact.
- Confirm whether pointer-derived byte loads still use the current pointer
  value as their base, or whether they have collapsed back to fixed
  `@.str0` global-byte loads.

Completion check:

- `todo.md` records the proof command, result, and classification.
- The classification either confirms this idea owns a live semantic dynamic
  byte-load failure or recommends lifecycle routing because the failure is
  green or belongs to another owner.

### Step 2: Localize the Semantic Load Boundary

Goal: if Step 1 confirms ownership, identify the semantic lowering boundary
where a dynamic pointer-derived load loses its pointer base.

Primary target:

- Semantic BIR construction for byte loads through pointer-valued expressions.

Concrete actions:

- Compare source expression lowering, semantic BIR, prepared BIR, and generated
  AArch64 for the smallest focused shape that reproduces the collapse.
- Identify the pointer carrier, increment or derivation operation, byte-load
  consumer, expected dynamic base, and actual fixed global load if present.
- Add or update focused semantic or backend-route coverage that fails on the
  localized boundary without relying only on `00173`.

Completion check:

- The owner is described in `todo.md` with concrete semantic-BIR and
  prepared/generated evidence.
- Focused coverage exists or a precise blocker explains why the coverage must
  wait for implementation.

### Step 3: Repair General Pointer-Derived Byte Loads

Goal: preserve the dynamic pointer value as the memory base for
pointer-derived byte loads.

Concrete actions:

- Implement the narrow semantic-BIR repair at the localized owner.
- Keep the repair general for pointer-derived byte loads, including incremented
  string or array pointer carriers.
- Avoid testcase-shaped matching on `00173`, literal names, symbol names,
  source variable names, loop shape, or generated instruction text.
- Preserve AArch64 address-valued publication, recursive formal preservation,
  and unrelated frontend/backend behavior outside this owner.

Completion check:

- Focused semantic or backend-route coverage for the localized load shape
  passes.
- Semantic BIR shows pointer-derived byte loads use the dynamic pointer value
  as the memory base.
- No expectation, runner, timeout, unsupported, or CTest-registration changes
  are used as proof.

### Step 4: Representative and Guardrail Proof

Goal: prove the repair moves the active representative and keeps adjacent
guardrails stable.

Concrete actions:

- Run the supervisor-selected proof subset, including the focused dynamic
  pointer-load tests and `c_testsuite_aarch64_backend_src_00173_c`.
- Include adjacent semantic-BIR, prepared memory operand, AArch64 instruction
  dispatch, and address-publication guardrails if the implementation touched
  shared lowering.
- If `00173` advances to a new first bad fact, classify it without expanding
  this source idea.

Completion check:

- `todo.md` records passing focused proof or the remaining classified failure.
- If the semantic pointer-derived string-load owner is satisfied, return to
  lifecycle routing for closure, parking, or split handling.
