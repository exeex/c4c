Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define Freshness Authority Model And Query Skeleton

# Current Packet

## Just Finished

Step 1 - Define Freshness Authority Model And Query Skeleton: added shared
prepared value freshness vocabulary, source references, ranked authority
candidates, a fail-closed query result/status surface, and a deterministic
selection helper. Added focused lookup-helper coverage for stable dump names,
producer/publication precedence over prior preservation, no-candidate,
invalid-candidate, ambiguous equal-rank, and unknown-use fail-closed statuses.

## Suggested Next

Start Step 2 from plan.md: publish call-argument freshness candidates from the
existing prepared call-plan facts without moving target backend consumers yet.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Keep producer rematerialization and explicit publication higher precedence
  than older PriorPreservation only when they are valid for the same value/use.
- The Step 1 query currently consumes explicit candidate facts only; Step 2
  should publish those candidates from call-plan/publication/preservation
  evidence before any consumer migration.
- Equal-rank matching authorities intentionally fail closed as
  `ambiguous_candidate`.
- Track closure-inventory notes here as consumers are wired or deliberately
  deferred.

## Proof

Passed on rerun: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

The first run of the same command failed before tests because `cc1plus` was
killed while compiling `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
outside the owned files. The rerun completed successfully and `test_after.log`
contains the passing proof.
