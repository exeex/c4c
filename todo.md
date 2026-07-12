# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate sibling AArch64 materializers

## Just Finished

- Plan Step 2 migrated the remaining AArch64 scalar ALU return-chain consumers
  in `alu.cpp` from `Route8ReturnChainRecord` and
  `Route1SourceValueIdentity` to existing prepared named handoffs. Return-chain
  register selection now follows indexed `BeforeInstruction` value moves and
  prepared same-block binary producer facts to the prepared before-return ABI
  move; missing, ambiguous, or inconsistent handoff authority fails closed.

## Suggested Next

- Continue Plan Step 2 with the next remaining AArch64 route-index consumer
  family selected by the supervisor, keeping the packet to one materializer
  and its focused proof.

## Watchouts

- The ALU return-chain traversal requires agreement between the prepared move
  bundle destination, the named producer result, and the consuming binary's
  unique chain operand; it intentionally rejects gaps and ambiguity.
- Other AArch64 route-index consumers remain outside this packet and still
  belong to Step 2.
- The delegated subset retains the known baseline failure in test 354 (`bl
  printf` missing); all other 35 tests pass, including the branch-control,
  call-boundary
  scalability and prepared-memory records coverage.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_aarch64_(return_lowering|prepared_scalar_alu_records|scalar_alu_records)|backend_(codegen_route|cli)_aarch64_)';
  } 2>&1 | tee test_after.log`. Build succeeded; 36/37 tests passed, including
  return lowering and both scalar-ALU record suites. The sole failure is the
  known baseline test 354 (`bl printf` missing). The delegated proof is
  sufficient relative to that baseline. Proof log: `test_after.log`.
