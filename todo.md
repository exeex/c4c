# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority
你該做code review了


## Just Finished

- Plan Step 1 repaired the prepared-memory acceptance regression after the
  AArch64 dispatch-value scalar select-chain authority migration. The
  store-global stack-publication fixture now attaches the same owned prepared
  lookup contract used by production traversal, so selected stack-home
  materialization succeeds through common named/prepared authority while the
  missing-authority case remains closed.

## Suggested Next

- Continue Plan Step 1 with the next supervisor-selected AArch64 dispatch
  authority family, keeping comparison, call-boundary, and ALU families out of
  this packet.

## Watchouts

- The separate comparison, call-boundary, and ALU route families were not
  changed by this packet.
- The common scalar select-chain producer/query contract was not defective;
  the regression was a test fixture that supplied a borrowed lookup without
  the matching owner required by dispatch authority validation.
- Missing prepared producer authority and mismatched producer kind continue to
  fail closed; no Route2 or raw select-chain identity authority was restored.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_prepared_memory_operand_records|backend_(codegen_route|cli)_aarch64_)';
  } 2>&1 | tee test_after.log`. Build succeeded; 34/35 tests passed, including
  repaired test 278, with only the known test 354 failure (`bl printf`
  missing). The delegated proof is sufficient relative to that baseline.
  Proof log: `test_after.log`.
