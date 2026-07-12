# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 1 repaired the prepared-memory acceptance regression after the
  AArch64 dispatch-value scalar select-chain authority migration. The
  store-global stack-publication fixture now attaches the same owned prepared
  lookup contract used by production traversal, so selected stack-home
  materialization succeeds through common named/prepared authority while the
  missing-authority case remains closed.

## Suggested Next

- Begin Plan Step 2 with one narrow call-boundary packet: migrate the Route 4
  publication-availability index in `calls.cpp` to existing common
  named/prepared authority, then remove the unused
  `Route6CallUseSourceIndex*` select-materialization API parameter while that
  dead call-use surface is adjacent. Keep comparison and ALU authority out of
  this packet.

## Watchouts

- Step 1 is complete at its dispatch-centered boundary; the remaining
  call-boundary, comparison, and ALU route families belong to Step 2.
- The first Step 2 packet is limited to call-boundary Route 4 publication
  availability plus dead Route 6 select API cleanup; do not absorb comparison
  or ALU route-index construction.
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
