Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Call-Argument Freshness Facts

# Current Packet

## Just Finished

Step 2 - Publish Call-Argument Freshness Facts: added per-argument prepared
freshness authority publication from existing call-plan facts. Call arguments
now publish DirectHome candidates when the existing source plan does not require
prior preservation, ExplicitPublication candidates from before-call argument
moves, ProducerRematerialization candidates from existing same-block producer
materialization facts, and PriorPreservation candidates only from the unique
complete indexed prior-preservation lookup. The prepared dump exposes the
published candidates, and focused printer coverage proves producer precedence
over prior preservation plus prior preservation as a valid unique source.

## Suggested Next

Start Step 3 from plan.md: wire a representative RV64 call-argument consumer
through the shared freshness query before trusting local home or
prior-preservation fallback ordering.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Call-argument freshness candidates are now published, but target backend
  consumers still use their existing local source-selection paths.
- Producer rematerialization and explicit publication rank above older
  PriorPreservation through the shared query; do not reimplement this ordering
  in target backend code.
- Equal-rank matching authorities intentionally fail closed as
  `ambiguous_candidate`.
- Track closure-inventory notes here as consumers are wired or deliberately
  deferred.

## Proof

Passed: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

`test_after.log` contains the passing proof for the delegated backend subset.
