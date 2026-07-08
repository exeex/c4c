Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish And Consume Move-Bundle Source Freshness

# Current Packet

## Just Finished

Step 4 corrected the reviewed move-bundle source-freshness drift. The shared
move-bundle consumer no longer manufactures selected freshness from the
consumed `PreparedMoveBundle`/`PreparedMoveResolution` alone. Default
classification builds source-freshness candidates only from complete
source-side value-home facts supplied through `PreparedValueHomeLookups`, and
explicit candidate classification rejects `MoveBundleSource` candidates whose
only proof is the consumed move bundle.

Focused shared and RV64 coverage now proves that a destination-valid move
bundle with `from_value_id` but no independent source freshness fails closed,
while the same shape is accepted when a complete direct source home is visible.
Injected missing, invalid, ambiguous, and wrong-kind candidate coverage remains
as helper/query coverage.

## Suggested Next

Proceed to Step 5: wire the next MVP prepared object or publication consumer
through the shared freshness query, preserving fail-closed behavior for
unknown, incomplete, ambiguous, or stale authority.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Do not reintroduce source freshness derived only from raw `from_value_id`,
  destination bundle validity, or the consumed move-bundle record.
- The representative RV64 object route already passes
  `lookups.value_homes` into the shared move-bundle classifier; incomplete
  source homes can now fail with shared
  `MissingMoveBundleSourceFreshness` before RV64-local shape diagnostics.
- Producer rematerialization and explicit publication rank above older
  PriorPreservation through the shared query; target consumers should follow
  the selected authority rather than reimplementing rank ordering.
- Equal-rank matching authorities intentionally fail closed as
  `ambiguous_candidate`.

## Proof

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: passed, `100% tests passed, 0 tests failed out of 346`.
Proof log: `test_after.log`.
