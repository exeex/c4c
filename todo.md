# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.23
Current Step Title: Receive typed switch authority

## Just Finished

- Completed plan Step 7.23: added transactional typed `LirSwitch` receipt as
  Raw-BIR `SwitchTerm`, retaining only its current-function integer selector,
  default successor, and ordered case successors. Builder/view/verifier and
  receiver coverage reject malformed selector/target authority without Raw or
  Canonical publication.

## Suggested Next

- Supervisor/plan-owner: assess the exhausted Step 7.23 runbook against the
  source idea and choose closure, repair, or a separately scoped successor.

## Watchouts

- The receiver reads only `selector`, `default_successor`, and
  `case_successors`; selector/label display shadows remain outside semantic
  lowering. `LirIndirectBrOp`, PHI, and all other unreceived families remain
  unsupported and fail closed.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log` (5/5 backend tests).
  The exact proof log is `test_after.log`; broader/full acceptance remains
  supervisor-owned.
