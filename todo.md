# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate prepared publication production

## Just Finished

- Completed `plan.md` Step 2 publication producer packet: current-block join
  publication selection now consumes only stable named BIR evidence
  (`FunctionNameId`, `BlockLabelId`, `ValueNameId`, instruction index) while
  existing prepared publications, homes, moves, and freshness remain the sole
  executable authority.
- The production prepared MIR view now obtains that evidence from the existing
  idea-704 `find_bir_current_block_join_source_identity` query and translates
  only its stable identities into the prepared boundary contract.
- Added focused positive and explicit missing, incomplete, ambiguous, and
  mismatched evidence proof; negative evidence changes the established
  executable fact status from `Available` to `MissingSourceProducer`. Legacy
  Route 5 fields remain diagnostic compatibility payload only and do not
  select the prepared publication fact.

## Suggested Next

- Continue `plan.md` Step 2 with the next coherent publication producer seam,
  reducing diagnostic Route 5 compatibility payload only when all remaining
  consumers have named evidence.

## Watchouts

- The current-block join query still accepts Route 5 input solely to populate
  legacy diagnostic fields used by printers/tests; executable selection must
  continue to consult `join_source_evidence` only.
- Do not widen into call plans, prepared lookups, target materializers, or the
  common MIR migration owned by idea 706.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_'`.
- Canonical proof log: `test_after.log`.
