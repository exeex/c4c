# Current Packet

Status: Active
Source Idea Path: ideas/open/792_lir_next_local_operation_receiver_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select one native-authority local-operation row

## Just Finished

- 734 Step 7.29 accepted the selected direct static-local-array GEP receipt in
  `4ab2deb7e`; this successor must not repeat it.

## Suggested Next

- Execute Step 1: inspect post-GEP local candidates and select exactly one
  only when its native authority is sufficient.

## Watchouts

- Do not use local spelling, `%t`, rendered operands, printer output, LLVM
  text, or testcase identity as semantic authority. Do not edit Raw-BIR.

## Proof

- Before implementation, establish the focused producer proof for the one
  selected row; the supervisor will select broader acceptance proof.
