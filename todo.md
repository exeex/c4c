# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Enrich the BIR-owned memory-access result

## Just Finished

- Step 2.2 completed the narrow common-MIR select/dependency adapter and Route
  2 proof.
- The first Route 3 common-adapter attempt stopped before implementation:
  `BirMemoryAccessIdentity` requires instruction pointer, address space,
  volatility, alignment, slot/link identities, and distinct local, global,
  and string names, while the current named BIR result exposes only
  `instruction_index`, `base_name`, and generic value pointers.
- The runbook now makes producer enrichment Step 2.3 and the dependent common
  adapter Step 2.4; no missing payload may be reconstructed in common MIR.

## Suggested Next

- Execute Step 2.3 only: enrich the named BIR memory-access producer/result
  with the complete identity payload and focused positive/negative producer
  proof. Do not begin the common adapter until that completion check is green.

## Watchouts

- Preserve the accepted distinction between `CompleteStopped` and
  `CompleteNoDependency`; common MIR relies only on the BIR result's complete
  contract and must not reconstruct operand traversal.
- Route 2 is now zero. Do not reintroduce route vocabulary or hidden recursive
  select dependency interpretation in later common-query packets.
- Do not reconstruct instruction pointer, address space, volatility,
  alignment, slot/link identities, or distinct local/global/string names from
  `instruction_index`, `base_name`, generic value pointers, or raw BIR in
  common MIR. Those fields belong to the Step 2.3 producer result.
- Step 2.4 is dependency-gated on a complete Step 2.3 result and proof;
  missing, incomplete, ambiguous, unsupported, or mismatched identity must
  fail closed at both boundaries.
- Twenty-three Route 1 spellings remain in other bounded memory/publication/
  edge-join adapters; they are not same-block producer authority and should
  migrate with their owning families rather than being mechanically renamed.
- Route 5 remains the only public-header breach and the largest family; leave
  its indexed edge/join migration for a later bounded packet.
- Keep target materializer migration in ideas 708-710 and stack-destination
  authority work in idea 707.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`
- The supervisor-selected exact backend proof passed 331/331, including the
  unchanged `backend_aarch64_instruction_dispatch`; `test_after.log` is the
  canonical proof log.
