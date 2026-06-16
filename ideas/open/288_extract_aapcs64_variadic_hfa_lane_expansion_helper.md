# Extract AAPCS64 Variadic HFA Lane Expansion Helper

## Goal

Move AArch64 variadic HFA carrier-array expansion policy out of generic
semantic call lowering and behind a target ABI helper with structured
input/output facts.

## Why This Exists

The post-286 cleanup decided that the current inline expansion in
`src/backend/bir/lir_to_bir/calling.cpp` is an acceptable compatibility owner
for the completed 286 route, but not the durable ownership boundary.

`append_aarch64_variadic_hfa_carrier_arg_lanes` currently verifies an AArch64
variadic call, classifies aggregate layout, resolves aggregate alias and local
leaf-slot facts, then appends one BIR argument per HFA lane with
`CallArgAbiInfo` metadata. Those are target ABI decisions embedded in generic
semantic call lowering.

## In Scope

- Define a target ABI helper contract for outgoing AAPCS64 variadic HFA
  carrier arrays.
- Feed the helper structured inputs: target profile, variadic-call marker,
  source aggregate operand, `LirTypeRef` / structured layout identity, local
  aggregate alias facts, leaf-slot facts, and local scalar slot types.
- Return either no expansion or ordered lane values plus matching
  `bir::CallArgAbiInfo` records with HFA lane count/index metadata.
- Preserve the existing aggregate `va_arg` payload metadata behavior unless an
  equivalent structured payload-plan contract deliberately replaces it.
- Preserve fail-closed behavior for incomplete layout, missing aggregate alias,
  leaf-slot count mismatch, missing local slot type, and lane type mismatch.

## Out of Scope

- Rewriting AArch64 call ABI classification wholesale.
- Reworking prepared/prealloc publication plans.
- Introducing a parallel ABI metadata shape that prepared/prealloc must merge
  with `CallArgAbiInfo`.
- Removing the current inline implementation before a behavior-preserving
  helper contract and proof exist.

## Owned Files

- `src/backend/bir/lir_to_bir/calling.cpp`
- a new or existing target ABI helper module under
  `src/backend/bir/lir_to_bir/` or an adjacent target ABI policy location
- focused backend tests around AAPCS64 variadic HFA outgoing calls and
  aggregate `va_arg` metadata

## Acceptance Criteria

- Semantic call lowering delegates HFA carrier-array policy to a target ABI
  helper with structured inputs and outputs.
- The helper emits lane values and the corresponding `CallArgAbiInfo` records
  together.
- The completed 286 stdarg proof subset remains green:
  `backend_lir_to_bir_notes`,
  `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
- Focused tests prove mismatched leaf-slot count/type and missing aggregate
  alias still fail closed.

## Reviewer Reject Signals

- The patch only moves the current lambda/body into a new file without defining
  structured helper input and output contracts.
- The helper consumes rendered call-argument or type suffix text as its primary
  source of truth.
- The helper emits HFA lane values without matching `CallArgAbiInfo` records.
- Semantic BIR or prepared/prealloc recomputes HFA lane shape after the helper
  has already classified it.
- A recognized but incomplete HFA carrier silently degrades to byval pointer
  lowering instead of failing closed.
- The route broadens AArch64 ABI classification or prepared call-plan behavior
  beyond the HFA carrier-array boundary.
