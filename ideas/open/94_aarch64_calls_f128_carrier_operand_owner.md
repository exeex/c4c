# AArch64 Calls F128 Carrier Operand Owner

## Goal

Create a narrow AArch64 calls-side owner for f128 carrier completion and
q-register operand rendering, preserving existing prepared f128 carrier
authority, diagnostics, and call-boundary behavior.

## Why This Exists

The calls-owner audit identified f128 carrier handling as a stable local
subowner candidate. The relevant helpers complete full-width and constant
carriers, find prepared f128 carrier placements, map scalar FP views, and render
q-register operands for call-boundary moves. This is an AArch64 ABI operand
rendering boundary; it can be clarified without inventing new shared f128
transport authority or moving ordinary call-boundary record construction.

## In Scope

- Audit and group the calls-side f128 helpers:
  `complete_full_width_f128_carrier`, `complete_f128_constant_carrier`,
  `find_prepared_f128_carrier_in_module`,
  `make_f128_q_register_operand_from_carrier`,
  `scalar_fp_view_from_register_name`, and any directly coupled scalar FP view
  conversion helpers needed by the f128 path.
- Tighten or extract a local helper owner that consumes prepared carrier
  placements and renders AArch64 q-register operands for existing call-boundary
  records.
- Preserve diagnostics for missing, incompatible, or incomplete f128 carrier
  facts.
- Preserve ordinary scalar FP register view handling where it is shared with
  non-f128 call-boundary operands.

## Out Of Scope

- Creating new shared f128 transport authority or moving carrier selection out
  of prepared call/transport owners.
- Reworking aggregate byval lane transport, immediate scalar publication,
  ordinary register operand rendering, or call-boundary record schema beyond
  direct f128 coupling.
- Changing f128 assembly output, unsupported diagnostics, q-register selection,
  or call-boundary record classification.
- Deleting printer-side validation or weakening record validation because a new
  f128 helper exists.
- Folding unrelated scalar GP/FP operand helpers into this idea unless they are
  needed to isolate the f128 carrier boundary.

## Acceptance And Proof Expectations

- The resulting boundary makes f128 carrier completion and q-register rendering
  explicit while consuming existing prepared carrier facts.
- Existing f128 argument/return call-boundary paths keep equivalent records,
  diagnostics, and assembly.
- Focused proof covers full-width f128 carriers, constant carriers, prepared
  carrier lookup, q-register operand rendering, ordinary non-f128 call-boundary
  moves adjacent to the helper boundary, and at least one f128 rejection or
  missing-carrier diagnostic path when available.
- Build proof covers the AArch64 backend targets affected by `calls.cpp` and
  any new local helper files.
- If the implementation changes helper APIs or record fields used by f128
  moves, use matching before/after logs for the focused test subset.

## Reviewer Reject Signals

- The implementation invents or relocates shared f128 transport authority
  instead of consuming prepared carrier placements.
- A helper rename or file move is claimed as capability progress while the f128
  carrier shape remains equally implicit and mixed into unrelated call
  lowering code.
- Ordinary call-boundary moves, aggregate byval lanes, immediate publication,
  or printer validation are changed without a direct f128 carrier reason.
- Unsupported expectations, diagnostics, selection status, or assembly output
  are weakened without explicit approval.
- Proof covers only one f128 happy path while constant carriers,
  full-width carriers, missing-carrier diagnostics, q-register rendering, and
  nearby ordinary call-boundary moves remain unexamined.
- The route keeps the exact old failure mode behind a new abstraction name,
  especially missing or incompatible carrier facts surfacing at the same late
  point with no clearer owner boundary.
