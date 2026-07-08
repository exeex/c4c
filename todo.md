Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Producer/Publication Operand Coverage

# Current Packet

## Just Finished

Step 5 added producer/publication operand freshness coverage for the existing
store-source publication route. `PreparedStoreSourcePublicationPlan` now
publishes `ProducerPublicationOperand` freshness candidates from already
validated same-block source-producer facts and selects them through
`find_prepared_value_freshness_authority`.

Prepared dumps now expose the selected freshness status and authority on
store-source publication rows. Focused printer coverage proves selected
`producer_rematerialization` authority for the binary store-local publication
route and for the select-materialization direct-global publication route.

## Suggested Next

Proceed to Step 6: inventory AArch64, x86, RV64, and nearby shared-prealloc
consumers that remain unwired, wiring only a narrow path if the MVP authority
already proves freshness without new semantic producer analysis.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Store-source publication freshness is currently wired only for validated
  same-block source producers on available store-source publication plans.
- Nearby unwired operand families for closure inventory: edge-publication move
  consumers, dependency operand authorities, select-carrier alias authorities,
  branch stack-load authorities, typed stack-source publications, recovered
  narrow store-source publication helpers, and pending store-global publication
  runs.
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
