# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate prepared publication production

## Just Finished

- Step 2 — migrated cast store-source publication production to require the
  applicable named `BirProducerResult` plus existing prepared producer, home,
  access, and ordering authority. Missing, ambiguous, or mismatched named
  producer evidence now fails closed without changing select store-source
  behavior or target-side compatibility planning.

## Suggested Next

- Continue Step 2 with the remaining select store-source publication family.

## Watchouts

- The named producer evidence requirement is now applicable to load-local,
  binary, and publication-produced cast store-source families; select retains
  its existing prepared-authority behavior until migrated independently.
- Cast evidence applicability is identified by the named producer block label
  already supplied by publication production. Target-side ad hoc compatibility
  planning remains unchanged and is outside this packet.
- Production store-source population and pending global publication derive the
  named producer evidence directly from the BIR block and publication index.
- Do not widen into call plans, prepared lookups, target materializers, or the
  common MIR migration owned by idea 706.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: passed, 309/309 backend tests; canonical proof log:
  `test_after.log`.
