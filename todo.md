# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair typed join-source identity propagation

## Just Finished

- Step 2 repaired the prepared-MIR and BIR join adapters: structural identity
  is copied before status gating, immediate rows no longer require named
  freshness, move immediates are retained, and BIR row statuses preserve each
  prepared negative category including `UnsupportedMove`. Aggregate BIR
  availability remains fail closed.

## Suggested Next

- Investigate the newly exposed focused-fixture failure in the prepared
  join-source producer: the copied Route 5 block does not retain the expected
  agreeing named BIR evidence/home identity.

## Watchouts

- The delegated adapter assertions now pass far enough to expose the next
  failure, `current-block join helper row should carry agreeing named BIR
  evidence`, inside producer preparation outside this packet's owned files.
- Do not weaken the exact typed row mapping or aggregate fail-closed behavior
  while repairing that producer-side copied-block identity mismatch.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$' 2>&1 | tee
  test_after.log`.
- Build passed. The focused test remains blocked by
  `current-block join helper row should carry agreeing named BIR evidence`;
  output is preserved in `test_after.log`.
