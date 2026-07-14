# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.21
Current Step Title: Receive legacy indirect-branch authority

## Just Finished

- Step 7.21 received legacy `LirIndirectBr.addr` and ordered current-function
  target IDs into verified Raw-BIR `IndirectJumpTerm` authority, with
  transactional malformed-address/target rollback coverage.

## Suggested Next

- Supervisor: review and commit the completed Step 7.21 slice, then obtain
  plan-owner direction for the next bounded receiver packet.

## Watchouts

- `LirCondBr`, `LirSwitch`, and `LirIndirectBrOp` remain unsupported and
  fail closed; this slice consumes no labels as indirect-branch semantics.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log` (5/5); proof log:
  `test_after.log`.
