# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated the AArch64 fused-comparison operand-producer consumer
  family in `comparison.cpp` from executable Route 7 agreement/index queries
  (and the selected-operand Route 7/raw-scan fallback) to the existing common
  prepared fused-comparison producer facts. Missing prepared producer authority
  now fails closed instead of selecting a target-local fallback.

## Suggested Next

- Continue Plan Step 2 with one narrow remaining materialized-condition or ALU
  authority-family packet selected by the supervisor; keep it limited to one
  semantic route-index consumer family and its focused proof.

## Watchouts

- The legacy Route 7 conversion/agreement helpers remain for focused contract
  tests, but executable fused-comparison materialization no longer consults
  them.
- Materialized-condition comparison and ALU route-index consumers remain
  outside this completed packet and still belong to Step 2.
- The delegated subset retains the known baseline failure in test 354 (`bl
  printf` missing); all other 34 tests pass, including the call-boundary
  scalability and prepared-memory records coverage.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_prepared_memory_operand_records|backend_(codegen_route|cli)_aarch64_)';
  } 2>&1 | tee test_after.log`. Build succeeded; 34/35 tests passed, with only
  the known test 354 failure (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
