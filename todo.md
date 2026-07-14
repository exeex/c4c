# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.22
Current Step Title: Receive typed conditional-branch authority

## Just Finished

- Step 7.22 receives typed `LirCondBr.condition` and ordered
  `true_successor`/`false_successor` IDs into `CondJumpTerm`, validating local
  I1 condition plus distinct same-function targets before the existing atomic
  Raw-BIR publication boundary; builder/verifier invariants and focused
  malformed-authority rollback coverage are included.

## Suggested Next

- Supervisor: review Step 7.22's typed conditional-branch receiver and select
  the next active-plan packet.

## Watchouts

- The importer reads only `condition`, `true_successor`, and
  `false_successor`; display shadows remain outside semantic lowering.
- `LirSwitch`, `LirIndirectBrOp`, PHI, and all other unreceived families remain
  unsupported and fail closed.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log` (5/5). Proof log:
  `test_after.log`.
