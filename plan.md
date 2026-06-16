# Extract AAPCS64 Variadic HFA Lane Expansion Helper

Status: Active
Source Idea: ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md

## Purpose

Move outgoing AAPCS64 variadic HFA carrier-array expansion out of generic
semantic call lowering and behind a target ABI helper with structured input and
output facts.

## Goal

Make semantic BIR call lowering delegate HFA carrier-array policy to a target
ABI helper that returns ordered lane values together with matching
`bir::CallArgAbiInfo` records.

## Core Rule

This is a behavior-preserving ownership cleanup. Keep the completed 286 stdarg
behavior green, keep incomplete HFA carriers fail-closed, and do not broaden
AArch64 ABI classification or prepared/prealloc call-plan behavior.

## Read First

- `ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- Existing tests covering AAPCS64 variadic HFA outgoing calls and aggregate
  `va_arg` metadata, especially the 286 stdarg proof subset.

## Current Targets

- `append_aarch64_variadic_hfa_carrier_arg_lanes` and its call sites in
  `src/backend/bir/lir_to_bir/calling.cpp`
- `bir::CallArgAbiInfo` lane metadata attached to expanded call arguments
- Local aggregate alias facts, leaf-slot facts, and scalar slot type facts used
  by the current inline expansion
- Focused backend tests around outgoing AAPCS64 variadic HFA calls and
  aggregate `va_arg` payload metadata

## Non-Goals

- Do not rewrite AArch64 call ABI classification wholesale.
- Do not rework prepared/prealloc publication plans.
- Do not introduce a parallel ABI metadata shape that must later be merged with
  `CallArgAbiInfo`.
- Do not remove the current inline behavior until the helper contract has
  behavior-preserving proof.
- Do not use rendered call-argument text or type suffix text as the helper's
  primary source of truth.

## Working Model

The helper should receive structured facts, decide whether an outgoing AAPCS64
variadic HFA carrier array expands into lanes, and return either no expansion
or an ordered expansion plan. The expansion plan owns both the lane values and
their corresponding `bir::CallArgAbiInfo` records, including HFA lane count and
lane index metadata.

Recognized-but-incomplete HFA carrier facts must remain fail-closed for missing
layout, missing aggregate alias, leaf-slot count mismatch, missing local slot
type, and lane type mismatch.

## Execution Rules

- Keep code changes scoped to `src/backend/bir/lir_to_bir/calling.cpp` and a
  target ABI helper module under `src/backend/bir/lir_to_bir/` or an adjacent
  target ABI policy location.
- Prefer a small structured helper input/output type over threading many
  positional parameters through call lowering.
- Preserve existing aggregate `va_arg` payload metadata behavior unless an
  equivalent structured payload-plan contract deliberately replaces it.
- Add or adjust focused tests only to prove the helper boundary and fail-closed
  behavior; do not weaken existing supported-path expectations.
- Every code-changing step needs fresh build proof plus the supervisor-chosen
  focused test subset.

## Step 1: Inventory Current HFA Expansion Inputs and Fail-Closed Edges

Goal: establish the exact structured facts and rejection paths the helper must
preserve before moving code.

Primary target: `src/backend/bir/lir_to_bir/calling.cpp`

Actions:

- Inspect `append_aarch64_variadic_hfa_carrier_arg_lanes` and each call path
  that appends outgoing call arguments.
- List the current inputs used for target profile, variadic-call recognition,
  source aggregate operand, `LirTypeRef` / layout identity, local aggregate
  alias facts, leaf-slot facts, and local scalar slot types.
- List each current fail-closed condition and the observable behavior expected
  when that condition is hit.
- Identify focused tests that already cover the completed 286 stdarg path and
  gaps for mismatched leaf-slot count/type or missing aggregate alias.

Completion check:

- `todo.md` records the helper input/output contract candidates, current
  fail-closed edges, and the narrow tests or fixture gaps for later steps.

## Step 2: Define the Target ABI Helper Contract

Goal: introduce the structured helper input/output boundary without changing
  outgoing call behavior.

Primary target: a new or existing target ABI helper module under
`src/backend/bir/lir_to_bir/` or an adjacent target ABI policy location.

Actions:

- Define a helper request type that carries the structured inputs identified in
  Step 1.
- Define a helper result type that represents no expansion, fail-closed
  rejection, or ordered lane expansion.
- Ensure expanded lanes and matching `bir::CallArgAbiInfo` records are returned
  together.
- Keep the contract private to the semantic BIR lowering layer unless existing
  local patterns strongly justify a header boundary.

Completion check:

- The helper contract exists, compiles, and can be called from
  `calling.cpp` without changing observable output.

## Step 3: Move HFA Carrier Expansion Policy Behind the Helper

Goal: make semantic call lowering delegate the outgoing AAPCS64 variadic HFA
carrier-array decision and lane emission to the helper.

Primary target: `src/backend/bir/lir_to_bir/calling.cpp`

Actions:

- Replace the inline carrier-array expansion decision with a helper call.
- Preserve the existing aggregate `va_arg` payload metadata behavior.
- Preserve the exact fail-closed behavior for incomplete layout, missing
  aggregate alias, leaf-slot count mismatch, missing local slot type, and lane
  type mismatch.
- Keep generic semantic call lowering responsible for appending ordinary
  arguments, not for recomputing HFA lane shape after helper classification.

Completion check:

- The completed 286 stdarg proof subset remains green:
  `backend_lir_to_bir_notes`,
  `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

## Step 4: Prove Fail-Closed Carrier Edge Coverage

Goal: add focused proof that helper extraction did not turn incomplete HFA
carrier facts into silent byval pointer lowering or partial expansion.

Primary targets:

- Focused backend tests around AAPCS64 variadic HFA outgoing calls
- Focused backend tests around aggregate `va_arg` metadata

Actions:

- Add or adjust focused tests for mismatched leaf-slot count/type when fixture
  support exists.
- Add or adjust focused tests for missing aggregate alias when fixture support
  exists.
- If fixture support is missing, record the exact blocker in `todo.md` rather
  than adding testcase-shaped shortcuts.
- Keep existing supported-path expectations at least as strong as before.

Completion check:

- Focused tests prove the available fail-closed edges and the completed 286
  stdarg subset remains green.

## Step 5: Broader Validation and Closure Readiness

Goal: decide whether the helper extraction is complete under the source idea
and whether any adjacent work belongs in a new idea.

Actions:

- Run the supervisor-selected broader validation appropriate for the final
  helper diff.
- Verify semantic BIR and prepared/prealloc do not recompute the HFA lane shape
  after helper classification.
- Verify the helper does not consume rendered call-argument or type suffix text
  as primary source of truth.
- Record any remaining adjacent ABI cleanup or fixture-support work in
  `todo.md` for supervisor lifecycle routing.

Completion check:

- The source idea acceptance criteria are satisfied, validation is recorded in
  `todo.md`, and the supervisor has enough evidence to request close or split
  follow-up work.
