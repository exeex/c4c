Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Representatives And Route Remaining Failures

# Current Packet

## Just Finished

Step 5 proved the five targeted RV64 object-lowering representatives with the
supervisor-selected torture subset. All requested rows passed:
`src/20000314-3.c`, `src/930614-1.c`, `src/pr35456.c`,
`src/980604-1.c`, and `src/pr39501.c`.

## Suggested Next

Proceed to Step 6 broader validation and closure decision. Keep the next
packet validation-focused unless the broader proof exposes a fresh RV64 object
fragment failure.

## Watchouts

- Keep BIR scalar-control-flow producer work in
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Keep function-signature, scalar-binop, and scalar/local-memory work in their
  own open ideas.
- Do not use row reclassification, expectation rewrites, unsupported markers,
  allowlist edits, or named torture-case shortcuts as progress.
- No remaining downstream diagnostic is current for the five Step 5
  representatives under the delegated CTest proof.
- If Step 6 finds another `unsupported_terminator_fragment`,
  `unsupported_move_bundle_target_shape`, or `unsupported_instruction_fragment`
  while emitting prepared RV64 object fragments, that stays inside this source
  idea. Fresh function-signature, call ABI, scalar-binop, scalar/local-memory,
  or BIR producer boundaries should be split or routed to their existing source
  ideas later.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'llvm_gcc_c_torture_src_(20000314_3|930614_1|pr35456|980604_1|pr39501)_c' >> test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

Representative outcomes:
- `llvm_gcc_c_torture_src_20000314_3_c`: passed.
- `llvm_gcc_c_torture_src_930614_1_c`: passed.
- `llvm_gcc_c_torture_src_pr35456_c`: passed.
- `llvm_gcc_c_torture_src_980604_1_c`: passed.
- `llvm_gcc_c_torture_src_pr39501_c`: passed.
