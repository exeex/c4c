# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated the AArch64 scalar-ALU unpublished load-local consumer
  in `alu.cpp` from Route 3 validation and BIR instruction-index reconstruction
  to the existing prepared same-block load-local producer. Missing prepared
  producer authority now rejects the special source-home operand, while stale
  BIR address identity cannot override the prepared producer. The focused
  prepared scalar-ALU contract positively proves both behaviors.

## Suggested Next

- Continue Plan Step 2 with one narrow remaining AArch64 authority-family
  packet selected by the supervisor; keep it limited to one semantic route-index
  consumer family and its focused proof.

## Watchouts

- Other AArch64 route-index consumers remain outside this completed packet and
  still belong to Step 2; this slice intentionally changed only unpublished
  load-local scalar-ALU operands.
- The delegated subset retains the known baseline failure in test 354 (`bl
  printf` missing); all other 35 tests pass, including the branch-control,
  call-boundary
  scalability and prepared-memory records coverage.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_branch_control_lowering|backend_aarch64_prepared_memory_operand_records|backend_(codegen_route|cli)_aarch64_)';
  } 2>&1 | tee test_after.log`. Build succeeded; 35/36 tests passed, with only
  the known baseline test 354 failure (`bl printf` missing); the added focused
  scalar-ALU route tests passed, for 35/36 total. The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
- Additionally ran `ctest --test-dir build --output-on-failure -R
  '^backend_aarch64_prepared_scalar_alu_records$'`; the focused prepared
  authority contract passed 1/1.
