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

## Closure Note

Closed after implementation in:

- `4da5d3cb3` (`Introduce AArch64 f128 carrier operand owner`)
- `28ca89deb` (`Route f128 result carrier operand rendering`)

The route introduced the local `F128CarrierCallOperandOwner` in
`src/backend/mir/aarch64/codegen/calls.cpp`. The owner consumes existing
prepared f128 carrier facts and owns only calls-side f128 carrier completion,
module fallback lookup, and q-register operand rendering for call-boundary
uses. It does not select or create prepared f128 carriers, own shared transport
authority, construct ordinary call-boundary records, or absorb scalar FP helper
authority.

Routed in-scope f128 calls-side uses through the owner, including:

- full-width f128 carrier completion
- f128 constant carrier completion
- module fallback prepared-carrier lookup
- source q-register operand rendering
- after-call result destination q-register operand rendering

Intentionally outside this route:

- prepared carrier selection and creation
- ABI result source operand binding
- f128 stack argument transport records and transport instruction construction
- constant argument destination register rendering and record construction
- shared scalar FP helpers and ordinary scalar FP moves
- aggregate byval transport, immediate scalar publication, and generic
  call-boundary record ownership

Separate open follow-up remains:

- `ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`

That follow-up was not absorbed into this route.

Proof:

- Focused before/after scope passed 5/5 before and 5/5 after.
- Close-time regression guard passed with non-decreasing pass-count mode:

```bash
cmake --build --preset default
ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_prepared_printer|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

The supervisor also reported broader validation on June 2, 2026:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

That broader backend scope passed 169/169.
