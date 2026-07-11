# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate prepared publication production

## Just Finished

- Step 2 — migrated binary store-source producer publication to require the
  applicable named `BirProducerResult` plus existing prepared producer, home,
  access, and ordering authority. Missing, ambiguous, or mismatched named
  producer evidence now fails closed without changing unrelated load/cast/
  select store-source families.

## Suggested Next

- Continue Step 2 with the next smallest formal or remaining store-source
  publication family selected by the supervisor.

## Watchouts

- The named producer evidence requirement is deliberately classified as
  applicable only to the binary store-source family in this packet; load,
  cast, and select families retain their existing prepared-authority behavior
  until migrated independently.
- Production store-source population and pending global publication derive the
  named producer evidence directly from the BIR block and publication index.
- Do not widen into call plans, prepared lookups, target materializers, or the
  common MIR migration owned by idea 706.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: passed, 309/309 backend tests; canonical proof log:
  `test_after.log`.
