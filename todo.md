# Current Packet

Status: Active
Source Idea Path: ideas/open/792_lir_next_local_operation_receiver_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select one native-authority local-operation row

## Just Finished

- Plan Step 1 selected exactly one post-791 row: the VLA `LirStackSaveOp`
  saved-stack-pointer receipt. Its native current-function result,
  pointer-definition/object/owner/type/liveness fields and verifier binding are
  recorded in `docs/lir_local_operation_authority/handoff_to_734.md`; stack
  restore and all other local forms remain unselected.

## Suggested Next

- Execute Step 2: add only the selected VLA stack-save row's minimum
  producer/schema and verifier admission; do not add receiver/Raw-BIR support.

## Watchouts

- The selected save result is one receipt, not a save/restore pair. Do not use
  local spelling, `%t`, rendered operands, printer output, LLVM text, or
  testcase identity as semantic authority. Do not edit Raw-BIR.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`
  (required Step 1 audit proof; `test_after.log` is the proof log).
