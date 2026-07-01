Status: Active
Source Idea Path: ideas/open/504_select_publication_move_bundle_evidence_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Select-Publication Evidence Surfaces

# Current Packet

## Just Finished

Step 1 inspected the prepared/BIR select-publication evidence and RV64
authority surfaces for the three ambiguous move-bundle rows from idea 495:
`src/builtin-constant.c`, `src/pr37924.c`, and `src/pr58726.c`.

The rows all currently collapse to the same case-log diagnostic,
`select publication move bundle requires unsupported RV64 moves`, without
function/block/event/phase/authority/storage/source tokens. Inspection found
the needed lower facts already exist in `PreparedEdgePublication`,
`PreparedEdgePublicationSourceProducer`, `PreparedMoveBundle`,
`PreparedParallelCopyBundle`, and RV64 `EdgePublicationMoveIntent`; the gap is
the early select-publication rejection's diagnostic/evidence publication, not a
lower producer prerequisite.

Durable evidence was written under
`build/agent_state/504_step1_select_publication_evidence_surfaces/`.

## Suggested Next

Execute Step 2 by publishing a narrow select-publication move-bundle evidence
surface for the early RV64 rejection path. The evidence should be keyed by the
prepared function/block/edge/destination identity and include the exact
prepared move-bundle, parallel-copy, `PreparedEdgePublication`, and
`EdgePublicationMoveIntent` fields needed to explain why the bundle is not yet
RV64-object-admitted.

## Watchouts

- Do not re-open before-instruction, out-of-SSA, or before-return
  materialization under idea 504.
- Do not lower select-publication move bundles in RV64 before explicit
  evidence/authority is published and accepted by a later consumer packet.
- Do not infer authority from source names, absent case-log tokens, raw BIR
  shape, object output, final registers, or target behavior.
- The current missing fields are diagnostic/evidence fields: function/block
  coordinate, event kind, phase, authority, move count, predecessor/successor,
  execution site, destination/source value ids, source producer kind/site,
  intent status, home details, and exact unsupported reason.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 1 proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
