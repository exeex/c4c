# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Establish and consume source-semantic named results

## Just Finished

- The Step 2.2 checkpoint review found no testcase overfit, expectation
  downgrade, or target-policy drift, and confirmed that `plan.md` still
  matches idea 706.
- The review rejected the Route 2 retirement claim because
  `find_select_chain_view_result` still scans and classifies BIR instructions
  in common MIR, including the type-less lookup path, instead of consuming a
  complete BIR-owned named answer.
- The review also identified a truthy/unavailable state in the shared
  `SameBlockSelectProducer` carrier. The prior select-producer packet did pass
  its 331/331 broad backend proof; after the monotonic guard passed, that
  canonical result was rolled forward from `test_after.log` to
  `test_before.log` under the supervisor workflow.

## Suggested Next

- Reopen Step 2.2 as one bounded Route 2 corrective packet before any Route 1
  or Route 3 migration:
  1. Extend the BIR-owned select/dependency result with the root producer
     kind/materialization identity and type-less lookup outcome needed by the
     adapter, or consume an existing separately named BIR producer result
     directly.
  2. Make `find_select_chain_view_result` a narrow status/identity adapter:
     remove common-MIR instruction scanning, producer-kind classification,
     candidate-root rediscovery, and locally derived materialization
     completeness.
  3. Normalize `SameBlockSelectProducer` status semantics by either populating
     a complete `Available` carrier in prepared target compatibility paths
     before making truth require availability, or separating the target-only
     compatibility carrier from the common-query result.
  4. Add focused positive and fail-closed proof for preserved root identity,
     producer kind/materialization identity, type-less lookup behavior, and
     unavailable/mismatched carrier states.
- After focused proof is green, rerun the supervisor-selected exact backend
  build/test command and retain its output as canonical `test_after.log`.
  Route 2 may reach zero only after both the ownership audit and retained broad
  proof are green.

## Watchouts

- Route 2 is not retired while common MIR scans or classifies BIR instructions,
  rediscovers type-less candidate roots, or derives producer completeness.
- Preserve Step 2.1 select-arm short-circuit and operand-order semantics; this
  packet transfers ownership and must not broaden dependency discovery.
- Keep the carrier correction bounded. Do not migrate target materializers or
  introduce target policy while repairing explicit status semantics.
- Remaining Route 3 helpers in common MIR serve publication identity; do not
  mechanically migrate them.
- Preserve missing, incomplete, ambiguous, unsupported, and mismatched
  fail-closed behavior at the common-MIR boundary.
- Eighteen Route 1 spellings remain in other bounded memory/publication/
  edge-join adapters; they are not same-block producer authority and should
  migrate with their owning families rather than being mechanically renamed.
- Route 5 remains the only public-header breach and the largest family; leave
  its indexed edge/join migration for a later bounded packet.
- Keep target materializer migration in ideas 708-710 and stack-destination
  authority work in idea 707.

## Proof

- Required focused proof: the directly affected BIR select/dependency and
  common-MIR select-chain/producer contracts must cover positive identity,
  type-less lookup, and missing/incomplete/ambiguous/mismatched/unavailable
  negatives without common-layer instruction reconstruction.
- Required acceptance proof:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`
- The prior select-producer packet's rolled-forward 331/331 proof remains
  valid, but it does not accept this reopened ownership correction. Fresh
  `test_after.log` proof for this packet remains pending and must include the
  common-query route guard and all backend families.
