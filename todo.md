# Current Packet

Status: Active
Source Idea Path: ideas/open/792_lir_next_local_operation_receiver_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority

## Just Finished

- Plan Step 2 published the one selected VLA `LirStackSaveOp` receipt with an
  explicit native stack-save admission. The producer marks one saved-stack
  result; the verifier requires its valid current-function result,
  unique owner, pointer/object/pointee coherence, and liveness, while
  compatibility stack saves and stack restore remain unselected.

## Suggested Next

- Execute Step 3: record the bounded 734 handoff and perform its designated
  producer-slice proof; do not add receiver/Raw-BIR support.

## Watchouts

- The selected save result is exactly one receipt, not a save/restore pair.
  Its marker cannot be removed while retaining local authority, and a function
  cannot publish a second selected stack-save row. Do not use local spelling,
  `%t`, rendered operands, printer output, LLVM text, or testcase identity as
  semantic authority. Do not edit Raw-BIR.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`
  (passed for Step 2; `test_after.log` is the proof log).
