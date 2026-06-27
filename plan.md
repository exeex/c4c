# Prepared Packed And FP128 Global Initializer Admission Runbook

Status: Active
Source Idea: ideas/open/409_prepared_packed_fp128_global_initializer_admission.md
Activated after: ideas/closed/399_rv64_object_route_global_data_and_symbol_memory.md

## Purpose

Repair the producer-side global initializer admission boundary where packed
aggregate and `fp128` global forms stop before prepared handoff.

## Goal

Publish explicit prepared global storage facts for supportable packed
aggregate and `fp128` initializer forms, or preserve precise producer-side
unsupported diagnostics when the byte layout or object representation is not
yet supportable.

## Core Rule

Prepared producers must own packed aggregate layout and `fp128` initializer
admission. RV64 object emission must not reconstruct initializer bytes,
aggregate layout, or floating representation from source shape when prepared
facts are missing.

## Read First

- `ideas/open/409_prepared_packed_fp128_global_initializer_admission.md`
- `ideas/closed/399_rv64_object_route_global_data_and_symbol_memory.md`
- `tests/c/external/gcc_torture/src/20040709-2.c`
- Current refresh artifacts under `build/agent_state/399_step1_global_refresh/`

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/20040709-2.c`
- Focused backend/prepared tests covering global initializer admission,
  aggregate global storage, floating or extended-precision global storage, and
  RV64 object emission where relevant.

## Non-Goals

- Do not reopen RV64 object-emitter global-data lowering from idea 399 without
  fresh prepared facts proving a target-side bug.
- Do not teach RV64 object emission to synthesize packed aggregate layout,
  `fp128` bytes, or source-level global initializer semantics.
- Do not absorb direct-call semantic, memset runtime, or vector local alloca
  failures from the same 399 refresh.
- Do not downgrade expectations, filter allowlists, or use testcase-specific
  branches.

## Working Model

Idea 399 proved the current RV64 object-route global-data representatives are
green. The remaining `20040709-2.c` evidence stops before prepared handoff at
bootstrap global admission. The visible shape is many packed aggregate globals,
including `fp128`-containing globals. Step 1 should classify the exact
producer admission rule and determine the first supportable initializer family.

## Execution Rules

- Start with classification proof before implementation.
- Keep the repair in producer/prepared global admission unless evidence proves
  a consumer contract defect.
- Preserve byte layout, field packing, alignment, endian representation,
  floating width, and initializer identity.
- Preserve precise unsupported diagnostics for unsupported or ambiguous global
  initializer forms.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat RV64 target inference, diagnostic-only churn, expectation rewrites, and
  named-case green proof as route failures.

## Step 1: Classify Packed And FP128 Global Admission

Goal: identify the producer admission rule blocking `20040709-2.c` and the
first supportable packed/`fp128` initializer family.

Actions:

- Reproduce or inspect the fresh object-route compile and prepared dump for
  `20040709-2.c`.
- Record the rejected global declarations, initializer forms, packed aggregate
  layout, `fp128` fields, alignment, byte-size facts, and first diagnostic.
- Locate the producer/global admission code that rejects the current forms.
- Decide whether the first repair packet should support packed integer
  aggregate globals, `fp128` global payloads, mixed packed aggregate storage,
  or a narrower producer diagnostic split.

Completion check:

- `todo.md` records the exact producer diagnostic, the first supportable
  initializer family, the intended producer file/function area, and the
  supervisor-delegated proof command.
- Any non-409 residual is routed with precise evidence instead of patched in
  RV64 object emission.

## Step 2: Publish The First Supported Initializer Facts

Goal: make prepared output publish explicit global storage facts for the first
supportable packed/`fp128` global initializer family.

Actions:

- Update the producer/prepared global admission path to emit explicit storage
  facts for the selected supported initializer family.
- Preserve fail-closed behavior for unsupported or ambiguous forms.
- Add or update focused backend/prepared coverage for supported and
  unsupported global initializer admission.

Completion check:

- `20040709-2.c` advances past the selected producer admission failure, or the
  selected supportable family is proven by focused coverage with a precise
  residual owner for the representative.
- Focused tests prove the facts are present and unsupported adjacent forms are
  still precise.

## Step 3: Prove Representative And Residual Ownership

Goal: prove the producer repair advanced 409 without hiding target-side or
runtime residuals.

Actions:

- Run the supervisor-selected `20040709-2.c` object-route compile plus
  prepared dump proof.
- Run the supervisor-selected backend CTest subset.
- If the case advances to an RV64 object-route, link, qemu, runtime mismatch,
  or unrelated semantic boundary, classify it separately instead of absorbing
  it into 409.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering, target
  inference, or filename-specific fixes are used as acceptance evidence.
- The supervisor has enough evidence to continue, request route review, or ask
  the plan owner for close/deactivation handling.
