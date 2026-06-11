# 192 Residual route-view consumer migration map

## Goal

Produce a route-by-route migration map for residual prepared consumers after
the Phase D selected-consumer follow-ups.

The map should identify the remaining production, printer/debug,
target-wrapper, and oracle consumers for Routes 1-7 and define the next
selected-consumer migrations that are prerequisites for true Phase E
`PreparedBirModule` retirement analysis.

## Why This Exists

Ideas 182-188 proved one bounded reader per route family. The pre-Phase-E
readiness audit shows that those migrations are useful evidence, but they do
not prove route-wide consumer exhaustion or public prepared API contraction.

Draft 155 needs a field-by-field retirement map. That map cannot be credible
until each route family has a durable account of which prepared readers remain,
which readers can move next, and which consumers are intentionally retained as
fallbacks or oracles.

## In Scope

- Inventory residual prepared consumers for Route 1 through Route 7.
- Classify each residual consumer as production lowering, printer/debug,
  target-wrapper, oracle/test, or retained target/prepared policy.
- Identify candidate next selected-consumer migrations for each route family.
- Name route-view or facade facts needed by those candidate migrations.
- Record proof expectations for each candidate migration, including nearby
  same-feature tests that must remain covered.
- Preserve selected-consumer evidence from ideas 182-188 as bounded prior art,
  not as route-wide completion.

## Out Of Scope

- Implementing the migrations named by the map.
- Deleting, privatizing, or renaming prepared APIs.
- Combining unrelated route families into one implementation slice.
- Opening or executing draft 155.
- Rewriting existing route-view contracts unless the map proves a specific
  contract gap for a future idea to own.

## Acceptance Criteria

- A durable route-family table names residual prepared consumers for Routes
  1-7 and classifies their consumer type.
- The table distinguishes migrated selected readers from unmigrated residual
  readers.
- Each route family has either a concrete next selected-consumer migration
  proposal or an explicit reason no migration should precede Phase E.
- Prepared printer/debug and oracle consumers are represented instead of being
  hidden behind production-only readiness.
- The output is usable by a future plan owner to create implementation ideas
  without re-deriving the residual consumer inventory from scratch.

## Reviewer Reject Signals

- Claiming route-wide completion because one selected reader in ideas 182-188
  moved to a route view.
- Weakening prepared oracle, printer/debug, route-debug, or c_testsuite
  expectations to make the map appear closer to retirement.
- Mixing Routes 1-7 into one catch-all implementation idea instead of naming
  separable next migrations.
- Treating helper renames, expectation rewrites, or consumer classification as
  capability progress.
- Omitting nearby same-feature residual consumers while proving only the
  already-known selected testcase.
- Moving target policy, ABI policy, address-materialization policy, or final
  emission policy into route-view scope by implication.
