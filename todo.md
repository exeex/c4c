Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert LIR Verifier Signature Checks

# Current Packet

## Just Finished

Completed `plan.md` Step 4, `Convert LIR Verifier Signature Checks`.

Converted verifier signature checks:

- Function signature return and parameter mirror validation now checks
  `signature_return_type_ref`, `signature_param_type_refs`,
  `signature_params`, `signature_is_variadic`, and
  `signature_has_void_param_list` directly for semantic validity.
- Retained `signature_text` parsing in `verify.cpp` only to confirm the final
  render payload contains a `define` or `declare` header; parsed header return
  and parameter text no longer drives semantic mirror decisions.
- Structured mirror checks now reject mismatched declared `StructNameId`
  payloads without consulting `signature_text`, while still allowing stale
  mirror text when the structured ID remains the semantic authority.
- Added verifier drift coverage where corrupted rendered signature text is
  accepted when structured signature metadata remains correct.

## Suggested Next

Delegate Step 5 to add or consolidate drift-resistance tests for the remaining
signature-text boundary, especially any target-specific consumers identified
during the Step 1 inventory.

## Watchouts

Remaining target-specific consumers from the Step 1 inventory still need a
follow-up audit, notably aarch64 fast-path signature predicates. BIR still has
a text parser for legacy hand-built LIR fixtures with missing structured
signature fields; generated LIR should not take that path once Step 2 metadata
is present.

## Proof

Supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^verify_tests_|^frontend_lir_function_signature_type_ref$)' > test_after.log 2>&1`
passed. Proof log: `test_after.log`.
