# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated AArch64 scalar-select publication in `alu.cpp` off its
  target-local Route 2/BIR select-chain reconstruction. The consumer now uses
  only the owned prepared lookup and named scalar-select-chain materialization
  query; missing, stale, non-select, or incomplete authority fails closed.

## Suggested Next

- Continue Plan Step 2 with the next remaining AArch64 route-index consumer
  family selected by the supervisor, keeping the packet to one materializer
  and its focused proof.

## Watchouts

- The focused scalar-ALU fixture now attaches its prepared lookup through the
  same owned lifetime contract required by production consumers.
- Other AArch64 route-index consumers remain outside this packet and still
  belong to Step 2.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_(prepared_scalar_alu_records|scalar_alu_records)|backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy)$';
  } 2>&1 | tee test_after.log`. Build succeeded and all 3 delegated tests
  passed. The delegated proof is sufficient. Proof log: `test_after.log`.
