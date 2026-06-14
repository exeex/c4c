Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Fail-Closed Proof Rows

# Current Packet

## Just Finished

Step 3: Add Fail-Closed Proof Rows completed.

Focused helper rows in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` now cover the
block-index label bridge through `populate_call_plans(...)` observations:

- positive prepared/BIR structured-label agreement
- absent prepared control-flow fallback to the BIR label
- prepared control-flow shorter than the queried BIR block index
- BIR rows absent at the public call-plan surface
- invalid BIR labels retaining prepared fallback
- prepared/BIR structured-label mismatch retaining prepared fallback
- invalid prepared-label behavior without manufacturing a non-invalid label

## Suggested Next

Execute Step 4 from `plan.md`: run the supervisor-selected broader backend
validation for the completed block-index label bridge candidate.

## Watchouts

- Do not reactivate completed idea 260 candidates.
- Keep scope limited to the block-index label bridge candidate.
- The private helper remains private in `call_plans.cpp`; the BIR-shorter row is
  proven at the public `populate_call_plans(...)` surface by omitting the BIR
  block, which means no call-plan selection can consume a prepared-only label.
- No expectations, output strings, helper statuses, or unsupported markers were
  rewritten to claim progress.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. The focused helper test rebuilt and
`backend_prepared_lookup_helper` passed 1/1.

Proof log: `test_after.log`
