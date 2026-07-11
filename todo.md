# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Adapt common MIR to named source-semantic results

## Just Finished

- Step 2.1 corrected the BIR-owned select/dependency result with an explicit
  `CompleteStopped` status. Immediate or unresolved first arms now preserve a
  complete root identity while stopping traversal without exposing later
  dependencies; only a proven traversable dependency-free arm continues.
- Focused contracts now prove immediate-first/global-second stopping,
  global-first/immediate-second direct-global identity, and LoadLocal-first
  continuation to a later global dependency.

## Suggested Next

- Step 2.2: finish the narrow common-MIR adapter over the corrected named BIR
  result, preserving its status and stable identities without restoring
  common-MIR recursion.

## Watchouts

- `CompleteStopped` is complete for root identity/materialization consumers but
  is not continuable by pair traversal; `CompleteNoDependency` remains the
  only status that permits inspection of the later operand.
- Do not restore common-MIR recursion. The correction belongs in the BIR-owned
  producer/result and must preserve the distinction between a conclusively
  dependency-free traversable operand and a legacy stop/unavailable operand.
- Do not ratchet Route 2 vocabulary to zero or claim Step 2.2 complete until
  the corrected Step 2.1 focused proof and the broader backend proof are green.
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
- The supervisor-selected exact backend proof passed 331/331; `test_after.log`
  is the canonical proof log.
