# Current Packet

Status: Active
Source Idea Path: ideas/open/727_common_prepared_return_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish complete attributed and fresh authority

## Just Finished

- Plan Step 2 added `publish_prepared_move_bundle` as the common producer seam.
  It owns the prepared function identity, assigns deterministic nonzero IDs
  distinct within the function, normalizes the exact published moves, and is
  now used by normal regalloc bundle creation.
- The common one-link/two-link contract builders and representative AArch64
  add/sub return-chain builder now use that seam instead of fixture-only proof
  field injection. The terminal builder also publishes the complete matching
  `FunctionReturnAbi` binding, allowing common traversal to derive DirectHome
  freshness from each attributed bundle's exact source move and home.

## Suggested Next

- Execute Plan Step 3 production-to-consumer readiness review and broader
  checkpoint using the now-attributed representative inputs; keep the AArch64
  consumer implementation unchanged unless separately authorized.

## Watchouts

- `backend_aarch64_instruction_dispatch` remains the recorded baseline failure
  with the same selected-global-load diagnostic. All nine return-chain and
  scalar tests in the delegated subset pass, including the formerly failing
  return-lowering and external add/sub-chain smoke surfaces.
- Zero attribution and the existing absent, ambiguous, inconsistent,
  unsupported, non-adjacent, wrong-operand, missing-home, incomplete-terminal,
  and cycle cases remain fail closed in the unchanged common classifier.
- The hook-produced `test_baseline.new.log` candidate was rejected because it
  added `backend_codegen_route_aarch64_prepared_call_boundary_scalability`
  beyond the accepted 52-failure baseline. The candidate remains for diagnosis,
  and the baseline reminder was cleared.

## Proof

- Ran the exact delegated command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_aarch64_return_(zero|add|add_sub_chain)_writes_elf_obj|backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|instruction_dispatch|return_lowering)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke)$' | tee test_after.log`.
  Build succeeded; 9/10 tests passed. The sole failure is the recorded baseline
  `backend_aarch64_instruction_dispatch`; `test_after.log` is canonical proof.
