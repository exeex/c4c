# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Adapt common MIR to named source-semantic results

## Just Finished

- Step 2.2 replaced the rejected common-MIR recursive select dependency walk
  with a narrow adapter over `find_bir_select_dependency`. Complete stopped,
  no-dependency, and direct-global results preserve root/dependency identities;
  unavailable, incomplete, ambiguous, and mismatched results fail closed.
- Focused MIR contracts cover direct-global, complete-no-dependency,
  complete-stopped, type-less-name compatibility, incomplete, ambiguous, and
  mismatched inputs. The Route 2 guard legitimately reaches zero.

## Suggested Next

- Continue Step 2 with one bounded Route 3 memory-access family: inventory the
  existing named BIR memory-access result and adapt the matching common query
  slice without changing target placement policy.

## Watchouts

- Preserve the accepted distinction between `CompleteStopped` and
  `CompleteNoDependency`; common MIR relies only on the BIR result's complete
  contract and must not reconstruct operand traversal.
- Route 2 is now zero. Do not reintroduce route vocabulary or hidden recursive
  select dependency interpretation in later common-query packets.
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
