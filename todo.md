# Current Packet

Status: Active
Source Idea Path: ideas/open/728_prepared_return_chain_shape_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split the public shapes into focused probes

## Just Finished

- Plan Step 2 added two focused source-to-normal-preparation probes without
  production changes or external expectation changes.
- The one-link probe independently proves that normal preparation publishes an
  attributed `BeforeReturn` `FunctionReturnAbi` move but no matching
  `PreparedAbiBinding`; its first instruction's attached return-chain status is
  exactly `StructurallyIncomplete`.
- The multi-link probe independently proves that normal preparation publishes
  the attributed successor value move at exact block 0 / instruction 1
  adjacency before checking the terminal seam; its first instruction's attached
  return-chain status is also exactly `StructurallyIncomplete`. This separates
  healthy successor attribution/position/adjacency from the shared missing
  terminal ABI-binding authority.

## Suggested Next

- Repair the shared normal-preparation terminal `FunctionReturnAbi` binding
  publication seam, then update these probes to require `Available` while
  retaining the independent multi-link successor-authority assertions.

## Watchouts

- Both focused public shapes now identify the same missing terminal binding;
  the multi-link successor relation is already attributed and adjacent. Do not
  widen the repair into successor reconstruction or AArch64 consumer logic.
- The probes intentionally assert the current precise diagnostic status and
  missing fact so this observation-only slice remains green. The repair packet
  must invert those terminal expectations to require the binding and
  `Available`, rather than leaving diagnostic expectations permanent.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^(backend_prepared_object_consumer_contract|backend_cli_aarch64_return_(zero|add|add_sub_chain)_writes_elf_obj|backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|return_lowering)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke)$'
  | tee test_after.log` (10/10 tests). The subset now directly observes both
  public preparation shapes and preserves all existing external integration
  expectations. Proof log: `test_after.log`.
