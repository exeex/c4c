# Route 5 Edge/Join Oracle Acceptance Review

Active source idea: `ideas/open/212_route5_edge_join_oracle_printer_row.md`

Chosen base commit: `0403490f8` (`[plan] Activate Route 5 edge join oracle row plan`)

Why this base: it is the commit that created the current active `plan.md` and
`todo.md` for idea 212. Later commits are row selection, implementation,
fail-closed proof, and string-proof todo updates for the same active idea.

Commit count since base: 4

## Findings

No blocking findings.

Low: `test_after.log` is not present in the worktree during this review, even
though `todo.md` records a seven-test Step 4 proof. This is a close-gate
validation hygiene issue, not an implementation-alignment problem. Before
regression-guard handling or lifecycle closure, the supervisor should recreate
the recorded seven-test proof so canonical `test_after.log` exists.

## Alignment Review

The implementation matches source idea 212. The selected row is one
helper-oracle row: `PreparedCurrentBlockJoinParallelCopySourceFact` for the
named current-block join source success case. The production change adds Route
5 evidence metadata to that one fact shape in
`src/backend/prealloc/publication_plans.hpp:410` and accepts it only through
the current-block join source helper in
`src/backend/prealloc/publication_plans.cpp:1079`.

The agreement gate is semantic rather than testcase-shaped. It requires an
available prepared fact and available Route 5 record, matching predecessor and
successor labels, destination name/type, prepared destination home identity,
source value kind/name/type, and source producer identity before setting
`route5_join_source_agrees` in
`src/backend/prealloc/publication_plans.cpp:1079`. The attach path is also
bounded to the selected current-block join source row and excludes immediate
sources, source-value rows, stack-home rows, and non-matching prepared states in
`src/backend/prealloc/publication_plans.cpp:1127`.

Prepared fallback is preserved. Failed Route 5 agreement leaves the prepared
fact intact and only records route status metadata; duplicate agreeing records
fail closed with `NoMatch` in
`src/backend/prealloc/publication_plans.cpp:1167`. The original prepared
parallel-copy, branch, move-bundle, edge-publication, printer, and wrapper
ownership paths are not migrated by this diff.

The tests cover the route-positive row and reject signals without weakening
expectations. Positive Route 5 evidence is asserted at
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3105`. Adjacent
prepared-owned facts, including the unsupported move row, are checked at
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3135`. Prepared-only
fallback is checked at
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3164`. Missing,
no-source, source mismatch, destination mismatch, memory-source, duplicate,
and wrong-predecessor cases are covered from
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3203` through
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp:3354`.

I found no expected-string rewrites, unsupported downgrades, helper renames,
baseline refreshes, broad edge/join family migration, or Route 5 takeover of
branch/parallel-copy policy.

## Judgments

Idea alignment: matches source idea

Runbook transcription: plan matches idea

Route alignment: on track

Technical debt: acceptable

Validation sufficiency: needs broader proof only in the sense that the
supervisor should recreate the already-recorded seven-test acceptance proof into
`test_after.log` before close/regression-guard handling.

Reviewer recommendation: continue current route
