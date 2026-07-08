Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish And Consume Move-Bundle Source Freshness

# Current Packet

## Just Finished

Step 4 review reset: `review/step4_move_bundle_source_freshness_review.md`
rejected the current move-bundle source-freshness route as drifting. The
uncommitted Step 4 implementation self-publishes `MoveBundleSource` freshness
from every destination-valid move with `from_value_id`, so the normal shared
classifier path can accept source freshness from the consumed move bundle
itself instead of from independent source-side authority.

## Suggested Next

Redo Step 4 before starting Step 5. The next executor packet must make
move-bundle source freshness depend on visible source-side authority beyond raw
move `from_value_id` and beyond destination bundle validity.

Acceptable correction routes include one of:

- publish move-source freshness only when an existing source home,
  publication, producer, preservation, or equivalent source-side fact proves
  availability for the move use;
- make any auto-publication helper distinguish source identity from source
  freshness and refuse to manufacture selected freshness from the consumed
  move bundle alone;
- keep explicit injected-candidate plumbing, but require production callers to
  pass candidates assembled from source-authority facts instead of
  self-publishing from the move being classified.

Required proof additions:

- add a production-path test where a destination-valid move bundle with
  `from_value_id` but no independent source freshness fails closed;
- add or preserve a nearby production-path acceptance test where the same
  destination-valid shape succeeds only when real source freshness is visible;
- keep the injected missing, invalid, ambiguous, and wrong-kind candidate
  failure coverage as helper/query coverage, not as the sole proof of normal
  production behavior.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Do not advance to Step 5 until Step 4 has been corrected and re-proved.
- Do not treat raw `from_value_id`, a well-formed destination bundle, or the
  consumed `PreparedMoveBundle`/`PreparedMoveResolution` record by itself as
  source freshness proof.
- The RV64 object move-bundle call site is in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, outside this packet's
  prior owned file list; the representative route currently reaches the shared
  classifier, so the production-path fail-closed behavior must be proven there
  or in equivalent shared classifier coverage.
- Producer rematerialization and explicit publication rank above older
  PriorPreservation through the shared query; target consumers should follow
  the selected authority rather than reimplementing rank ordering.
- Equal-rank matching authorities intentionally fail closed as
  `ambiguous_candidate`.
- The reviewer found the current explicit tests prove injected-candidate
  failure modes, not the default production failure where no independent source
  authority exists.

## Proof

No accepted proof for Step 4 after review. Prior `test_after.log` is not
acceptance proof because the reviewed implementation was route-drifting.
