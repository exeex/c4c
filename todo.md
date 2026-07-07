Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Prepared Terminator Object Lowering

# Current Packet

## Just Finished

Step 1 coverage slice added focused RV64 object diagnostic hooks for the current
prepared fragment blockers:

- `src/20000314-3.c`, `src/930614-1.c`, and `src/pr35456.c` map to the
  prepared terminator object-fragment blocker,
  `unsupported_terminator_fragment`; covered by
  `reports_prepared_register_condition_terminator_fragment_diagnostic`.
- `src/980604-1.c` maps to the prepared move-bundle/select-publication blocker,
  `unsupported_move_bundle_target_shape`; covered by
  `reports_prepared_select_publication_move_bundle_fragment_diagnostic`.
- `src/pr39501.c` maps to the plain BIR `SelectInst` object-fragment blocker,
  `unsupported_instruction_fragment`; covered by the extended
  `rejects_reused_nested_i32_ordinary_select_without_intermediate_home_object`
  diagnostic assertion.

## Suggested Next

Delegate Step 2 from `plan.md`: inspect the prepared terminator facts for the
terminator representatives and implement the semantic RV64 object terminator
fragment rule, keeping malformed prepared facts fail-closed.

## Watchouts

- Keep BIR scalar-control-flow producer work in
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Keep function-signature, scalar-binop, and scalar/local-memory work in their
  own open ideas.
- Do not use row reclassification, expectation rewrites, unsupported markers,
  allowlist edits, or named torture-case shortcuts as progress.
- If focused BIR evidence shows prepared facts are malformed, stop and report
  the owner-boundary mismatch instead of expanding this RV64 object runbook.
- Step 1 only locked down current RV64-local object diagnostics; it did not
  repair lowering or change representative classifications.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
