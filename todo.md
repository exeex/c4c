# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair typed join-source identity propagation

## Just Finished

- Step 2 repaired `attach_named_current_block_join_source_evidence` so it derives
  one exact named producer instruction from `inputs.block`, validates that
  producer through the BIR producer view, and selects evidence with the resulting
  `(function, block, value, instruction)` key. Optional publication producer
  coordinates now confirm the derived key when both are present; absence alone
  no longer yields `Incomplete`.
- The helper keeps typed fail-closed outcomes: invalid or absent BIR identity and
  partial publication metadata are `Incomplete`, duplicate matching BIR
  producers are `Ambiguous`, disagreeing publication coordinates are
  `Mismatched`, and missing/negative/duplicate evidence retains the selector's
  existing typed status. No cache or caller-pointer shape controls status:
  nonempty negative evidence always promotes the applicable row to
  `MissingSourceProducer`; empty evidence does so when no complete agreeing
  publication producer coordinates independently confirm the exact key.

## Suggested Next

- Localize the now-first failing, distinct CFG load-local identity assertion at
  `backend_prepared_lookup_helper_test.cpp:6414`; the current-block join-source
  evidence and reconstructed JoinTransfer fixture section now completes.

## Watchouts

- The original prepared-MIR/BIR aggregate assertion, copied-block positive
  evidence assertion, and all four typed negative evidence checks pass without
  expectation changes. The test now stops later at `complete prepared
  JoinTransfer publication should support a non-PHI edge`; temporary assertion
  splitting localized the first failure to missing PreparedJoinTransfer semantic
  origin, not the corrected instruction-0 evidence. The authorized evidence-only
  fixture edit cannot repair that separate authority setup.
- AST tracing identifies the exact first mismatch in
  `prepared_join_transfer_destination_consistent`: the publication's
  `destination_value` is named I32 `%current.destination`, but its owning
  `PreparedJoinTransfer::result` is the default-constructed `bir::Value`
  `(Immediate, Void, 0, empty name)` because the fixture's JoinTransfer aggregate
  omits `.result`. Therefore destination consistency is false,
  `has_unique_complete_prepared_join_transfer_authority` returns before its
  unique-edge scan, `prepared_join_transfer_authority_complete` stays false, and
  the origin stays `Unknown` after PHIs are erased.
- `route5_join_block` masks this incomplete fixture authority because
  `block_has_matching_phi_publication` establishes `BirPhi` origin first. The
  copied `prepared_only_block` removes those three PHIs, so it correctly exposes
  the missing JoinTransfer result. Neither exact producer evidence nor the block
  mutation changes the stored JoinTransfer/result fields.
- Ownership classification: fixture construction, not common producer-evidence
  preparation. Repairing `prepared_join_transfer_destination_consistent` to
  accept a missing/mismatched result would weaken a shared fail-closed authority
  contract and is not a valid Step 2 implementation change.
- After authorization, the named JoinTransfer result was initialized to exact
  named I32 `%current.destination`; the named prepared-only positive and stale
  evidence checks then passed. The next first failure is the immediate row:
  its publication destination is named I32 `%current.immediate_destination`, but
  it shares the same owning transfer whose result is now
  `%current.destination`. Destination consistency therefore correctly rejects
  that row's prepared origin. The fixture currently models three distinct PHI
  results as three edge transfers under one single-result JoinTransfer.
- The fixture is now reconstructed as three complete, single-result
  JoinTransfers for named, immediate, and stack destinations. The incomplete and
  mismatched named-authority probes mutate only the named transfer result; the
  duplicate named transfer now correctly yields typed `AmbiguousPublication` at
  lookup time. Later no-Route5/no-evidence probes retain prepared identities but
  now expect the common fail-closed `MissingSourceProducer` row status.

## Proof

- Ran the delegated command exactly:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$' 2>&1 | tee
  test_after.log`.
- Build succeeded. All current-block join-source evidence, typed negative,
  prepared-only authority, malformed-authority, and no-Route5 fail-closed probes
  now pass. The focused test advances to the distinct later failure `BIR CFG edge
  source identity should match prepared load-local semantic oracle` at line 6421.
  Proof log: `test_after.log`.
