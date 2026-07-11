# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate prepared publication production

## Just Finished

- Step 2 — migrated load-local store-source producer publication to require
  the applicable named `BirProducerResult` plus existing prepared producer,
  home/byval, access, and ordering authority. Missing, ambiguous, or
  mismatched named producer evidence now fails closed without changing cast
  or select store-source families.

## Suggested Next

- Continue Step 2 with the next smallest remaining cast or select
  store-source publication family selected by the supervisor.

## Watchouts

- Formal publication is not a remaining named-producer seam because BIR
  formals are parameters rather than same-block instruction producers.
- The named producer evidence requirement is now applicable to load-local and
  binary store-source families; cast and select families retain their existing
  prepared-authority behavior until migrated independently.
- Production store-source population and pending global publication derive the
  named producer evidence directly from the BIR block and publication index.
- Do not widen into call plans, prepared lookups, target materializers, or the
  common MIR migration owned by idea 706.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: passed, 309/309 backend tests; canonical proof log:
  `test_after.log`.
