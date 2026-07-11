# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Enrich the BIR-owned memory-access result

## Just Finished

- Step 2.3 enriched `BirMemoryAccessResult` with the BIR instruction pointer,
  address space, volatility, alignment, stable slot/link ids, distinct
  local/global/string base names, and stable pointer/result/stored value names.
- The named producer now returns a closed failure state for missing instruction,
  kind, base, role value, stable id/name pairs, conflicting base evidence, and
  incompatible local/global node/base evidence.
- Focused contracts prove complete local/global/string identities and reject
  incomplete and mismatched identities without changing common MIR or targets.

## Suggested Next

- Execute Step 2.4: adapt the common MIR memory query exclusively from the now
  complete named BIR result, preserving the producer's fail-closed status.

## Watchouts

- Preserve the accepted distinction between `CompleteStopped` and
  `CompleteNoDependency`; common MIR relies only on the BIR result's complete
  contract and must not reconstruct operand traversal.
- Route 2 is now zero. Do not reintroduce route vocabulary or hidden recursive
  select dependency interpretation in later common-query packets.
- Step 2.4 must consume the named fields directly; do not reconstruct identity
  from `instruction_index`, `base_name`, generic value pointers, or raw BIR.
- Preserve missing, incomplete, ambiguous, unsupported, and mismatched
  fail-closed behavior at the common-MIR boundary.
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
  enriched `backend_bir_memory_publication_view_contract` and unchanged
  `backend_aarch64_instruction_dispatch`; `test_after.log` is the canonical
  proof log.
