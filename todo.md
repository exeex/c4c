Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Validation And Closure Decision

# Current Packet

## Just Finished

Step 6 broader backend validation passed after the Step 3 through Step 5 RV64
object-lowering repairs. The focused Step 5 representatives had already passed:
`src/20000314-3.c`, `src/930614-1.c`, `src/pr35456.c`,
`src/980604-1.c`, and `src/pr39501.c`.

## Suggested Next

Ask the plan owner to decide whether the active RV64 object-lowering source
idea is complete and should close, or whether any remaining work should be
split into a follow-up idea/runbook.

## Watchouts

- Keep BIR scalar-control-flow producer work in
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Keep function-signature, scalar-binop, and scalar/local-memory work in their
  own open ideas.
- Do not use row reclassification, expectation rewrites, unsupported markers,
  allowlist edits, or named torture-case shortcuts as progress.
- No remaining downstream diagnostic is current for the five Step 5
  representatives under the delegated CTest proof.
- The broader backend proof did not expose a fresh RV64 object-fragment
  blocker.
- Fresh function-signature, call ABI, scalar-binop, scalar/local-memory, or BIR
  producer boundaries should be split or routed to their existing source ideas
  later rather than expanding this RV64 object-lowering runbook.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

Prior Step 5 representative outcomes:
- `llvm_gcc_c_torture_src_20000314_3_c`: passed.
- `llvm_gcc_c_torture_src_930614_1_c`: passed.
- `llvm_gcc_c_torture_src_pr35456_c`: passed.
- `llvm_gcc_c_torture_src_980604_1_c`: passed.
- `llvm_gcc_c_torture_src_pr39501_c`: passed.
