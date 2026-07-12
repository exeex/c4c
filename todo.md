# Current Packet

Status: Active
Source Idea Path: ideas/open/717_prepared_mir_join_source_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Preserve exact authority in the prepared-MIR direct-edge view

## Just Finished

- Plan Step 1 revalidated the focused prepared-MIR join-source boundary: the
  named, immediate, stack, unsupported, missing, stale, duplicate, and mismatch
  contract is green with the idea-719 prerequisite restored.
- The Step 2 route-quality review found that the public prepared-MIR and BIR
  views retain `Available` after dropping exact typed authority, and that the
  focused test proves only status/count. The blocking evidence is recorded in
  `review/idea717_step2_route_quality_review.md`.

## Suggested Next

- Execute plan Step 2.1 as the first bounded correction packet: preserve exact
  destination/source, producer, publication, move, and selected-freshness
  authority in the prepared-MIR direct-edge view, then prove that boundary.

## Watchouts

- `Available` is invalid when any required typed authority was summarized or
  discarded; classify the row explicitly unavailable if exact authority
  cannot be carried or independently resolved.
- Do not start broader acceptance until Steps 2.1 through 2.4 complete and an
  independent re-review clears the blocking report.
- Reject row-order, display-name, route, prepared-call, target, and expectation
  shortcuts.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_prepared_lookup_helper$'`.
- The supervisor-selected focused proof was sufficient for Step 1; complete
  combined output is preserved in `test_after.log`.
- Step 2.1 requires a fresh supervisor-delegated build and narrow
  prepared-MIR authority-preservation proof before advancing to Step 2.2.
