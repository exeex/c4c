# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated the AArch64 fused-comparison operand producer consumers
  in `comparison.cpp` from BIR/Route 7 producer discovery to the existing
  prepared named producer facts. Emitted-cast, folded-constant, stack-home, and
  missing-publication paths now consume prepared producer kind, instruction,
  value-name, and constant authority; missing or internally inconsistent named
  producer authority fails closed before operand publication.

## Suggested Next

- Continue Plan Step 2 with one narrow remaining AArch64 authority-family
  packet selected by the supervisor; keep it limited to one semantic
  route-index consumer family and its focused proof.

## Watchouts

- Legacy Route 7 agreement helpers remain solely for the existing BIR
  compatibility test; production fused-comparison lowering no longer calls
  them or `bir::find_comparison_operand_producer`.
- Other AArch64 route-index consumers remain outside this completed packet and
  still belong to Step 2.
- The delegated subset retains the known baseline failure in test 354 (`bl
  printf` missing); all other 35 tests pass, including the branch-control,
  call-boundary
  scalability and prepared-memory records coverage.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_branch_control_lowering|backend_aarch64_prepared_memory_operand_records|backend_(codegen_route|cli)_aarch64_)';
  } 2>&1 | tee test_after.log`. Build succeeded; 35/36 tests passed, including
  the focused AArch64 branch-control and prepared-memory contracts. The sole
  failure is the known baseline test 354 (`bl printf` missing). The delegated
  proof is sufficient relative to that baseline. Proof log: `test_after.log`.
