# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.33
Current Step Title: Receive selected inline-assembly output-only authority

## Just Finished

- Step 7.33 completed: the selected scalar-integer output-only row now receives
  only its structured `LirValueId`, Output/index-0 binding, and `LirTypeRef`;
  its Raw-BIR source registration bypasses compatibility result/operand and
  asm/constraint-text recovery. Nearby interface coverage proves i32/i64
  receipt to Store plus missing, invalid, duplicate, role/index/type mismatch,
  unknown/foreign use, and missing/duplicate-use rollback.

## Suggested Next

- Supervisor: select the next bounded active-plan packet.

## Watchouts

- The selected receiver intentionally accepts absent/misleading compatibility
  result and original asm/constraint text; nonselected inline-asm forms retain
  their existing fail-closed validation route.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_to_bir_interface$'`.
- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log`; proof log:
  `test_after.log`.
