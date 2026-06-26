# RV64 Object Route Scalar And Floating Edge Lowering Runbook

Status: Active
Source Idea: ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md

## Purpose

Repair the RV64 prepared-object scalar compare/trunc and floating-cast edge
lowering bucket without weakening prepared BIR ownership or object-route
admission.

## Goal

Generalize reusable RV64 object lowering for the observed scalar compare/trunc
and floating-cast prepared forms, then prove representative cases and nearby
same-bucket cases through the backend route.

## Core Rule

Lower real prepared scalar and floating forms semantically. Do not hard-code
testcase names, compare function names, constants, or source filenames, and do
not invent missing producer facts inside RV64 emission.

## Read First

- `ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md`
- `todo.md`
- `tests/c/external/gcc_torture/src/20000313-1.c`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- `tests/c/external/gcc_torture/src/20000403-1.c`
- RV64 object emission and prepared lowering code around
  `unsupported_scalar_compare_trunc` and `unsupported_floating_cast`
- RV64 backend runner `scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Current Scope

- Classify the prepared shapes behind current `unsupported_scalar_compare_trunc`
  failures, including `src/20000403-1.c` after the separate `I16` formal ABI
  repair.
- Generalize scalar compare result publication and integer truncation support
  beyond the currently admitted narrow shape.
- Classify and repair, or precisely split, the observed `unsupported_floating_cast`
  failures.
- Prove representative and nearby same-bucket cases without expectation
  rewrites or allowlist-only progress claims.

## Non-Goals

- Do not rewrite unrelated scalar-binop semantic `lir_to_bir` failures.
- Do not implement broad floating-point ABI work beyond the observed prepared
  forms unless proof shows it is required for this bucket.
- Do not change gcc_torture expectations, supported markers, or allowlists as
  claimed capability progress.
- Do not reopen the closed `I16` formal ABI publication repair inside this
  object-route plan.

## Working Model

- The `I16` formal ABI producer gap is closed; `src/20000403-1.c` now exposes a
  later object-route scalar compare/trunc blocker.
- The scalar bucket is expected to need reusable compare-result plus truncation
  lowering, not a named-case shortcut.
- Floating-cast failures may be RV64 FPR lowering gaps, but missing prepared
  BIR facts should be split into a separate producer idea instead of patched in
  the object emitter.

## Execution Rules

- Start with classification before code changes; capture representative
  prepared forms and exact diagnostics in `todo.md`.
- Keep scalar compare/trunc and floating-cast work as separable packets unless
  the same helper contract genuinely covers both.
- Preserve prepared-object contract boundaries; RV64 emission consumes prepared
  facts and lowers target forms.
- Each code-changing packet needs the supervisor-delegated build/proof command
  recorded in `test_after.log` and `todo.md`.
- Use same-bucket nearby cases for proof, not only one representative.

## Ordered Steps

### Step 1: Classify Scalar And Floating Edge Shapes

Goal: identify the reusable prepared forms behind the scalar compare/trunc and
floating-cast diagnostics before editing lowering code.

Primary targets:

- `tests/c/external/gcc_torture/src/20000313-1.c`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- `tests/c/external/gcc_torture/src/20000403-1.c`
- RV64 object emission diagnostics for `unsupported_scalar_compare_trunc` and
  `unsupported_floating_cast`

Actions:

- Run or inspect a supervisor-selected allowlist probe for representative
  scalar and floating cases.
- Capture prepared/object-route evidence for the compare result, truncation,
  and floating-cast shapes.
- Decide whether the first implementation packet should target scalar
  compare/trunc, floating cast, or a producer-fact split.
- Record the chosen narrow proof command and any same-bucket neighbor cases in
  `todo.md`.

Completion check:

- `todo.md` records the exact prepared forms, diagnostic evidence, selected
  first packet, and proof command without claiming implementation progress.

### Step 2: Generalize Scalar Compare/Trunc Lowering

Goal: repair `unsupported_scalar_compare_trunc` for reusable prepared scalar
compare-result and integer truncation forms.

Primary targets:

- RV64 object emission/lowering code around scalar compare result publication
  and truncation
- Representative scalar cases from Step 1

Actions:

- Add semantic lowering for the prepared scalar compare/trunc family identified
  in Step 1.
- Preserve truncation semantics; do not clear the diagnostic by widening,
  dropping, or bypassing the truncation.
- Avoid matching on testcase names, function names, or one exact instruction
  sequence when nearby cases share the same prepared family.
- Run the delegated build/proof command and record before/after diagnostic
  movement in `todo.md`.

Completion check:

- Representative scalar cases no longer fail with
  `unsupported_scalar_compare_trunc`, or `todo.md` names a precise producer
  fact split required before object lowering can proceed.

### Step 3: Repair Or Split Floating-Cast Lowering

Goal: handle the observed `unsupported_floating_cast` prepared forms without
creating ABI-incoherent FPR/GPR movement.

Primary targets:

- `tests/c/external/gcc_torture/src/20020225-2.c`
- RV64 FPR register and floating-cast object lowering code

Actions:

- Classify whether the floating cast has complete prepared facts for RV64
  object emission.
- If facts are complete, implement the narrow reusable F32/F64 lowering needed
  for the observed forms.
- If facts are missing or wrong, write a precise follow-up producer idea rather
  than compensating in MIR/RV64 emission.
- Run the delegated proof command for the representative and nearby floating
  cases.

Completion check:

- `src/20020225-2.c` no longer fails with `unsupported_floating_cast`, or the
  remaining blocker is split into a source idea with concrete acceptance and
  reviewer reject signals.

### Step 4: Refresh Bucket Proof And Route Remaining Work

Goal: prove the scalar/floating bucket moved in the intended direction and
return clean lifecycle state for any distinct follow-up.

Primary targets:

- `todo.md`
- `test_after.log`
- RV64 gcc_torture backend subset output

Actions:

- Run the supervisor-selected refreshed subset or full backend progress probe.
- Confirm the scalar/floating edge bucket count decreases without new runtime
  mismatches.
- Record remaining distinct diagnostics in `todo.md`.
- If all acceptance criteria are satisfied, ask the plan owner to close the
  idea; otherwise route any separate producer or target-emission initiative
  through a new or existing `ideas/open/` file.

Completion check:

- Canonical lifecycle state says whether idea 401 is complete, still active for
  a remaining scalar/floating packet, or blocked by a distinct follow-up.
