Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh The Non-637 Residual Baseline

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by refreshing the named idea-647 residual rows and
classifying the current evidence without selecting an implementation family.
Fresh diagnostics are under
`build/agent_state/647_step1_20260710_residual_baseline/`, with the summary in
`build/agent_state/647_step1_20260710_residual_baseline/summary.md`.

Current residual set by stable test name:

| Stable test name | Classification | Refreshed evidence |
| --- | --- | --- |
| `src/20011109-2.c` | reopened-637 evidence | current diagnostic is `authority=none`, `move_count=3`, `fragment_status=missing_stack_destination_fan_in_authority_fact`; prior Step 2 extract records select-materialized chains, which belongs to closed idea 637 rather than a non-637 producer family |
| `src/20021204-1.c` | mutual-exclusion negative evidence | current diagnostic is non-parallel two-register-source stack-destination fan-in with `authority=none`; prior focused probe shows edge/select facts authorize `%t25`, not failing `%t20/%t21 -> %t22` |
| `src/920429-1.c` | rejection-only | current diagnostic is non-parallel two-register-source stack-destination fan-in with `authority=none` and no ordered final-state, mutual-exclusion, or explicit-merge producer fact |
| `src/ptr-arith-1.c` | rejection-only | current diagnostic is non-parallel two-register-source stack-destination fan-in with `authority=none` and no ordered final-state, mutual-exclusion, or explicit-merge producer fact |
| `src/pr70005.c` | rejection-only | current diagnostic is `fn1:logic.end.73` non-parallel two-register-source stack-destination fan-in with `authority=none` and no ordered final-state, mutual-exclusion, or explicit-merge producer fact |

Stale named idea-647 rows:

| Stable test name | Classification | Refreshed evidence |
| --- | --- | --- |
| `src/930429-1.c` | stale | current first owner is `unsupported_terminator_fragment`, not stack-destination fan-in |
| `src/pr34415.c` | stale | current first owner is `unsupported_scalar_compare_publication`, not stack-destination fan-in |

No refreshed row exposes ordered final-state producer metadata, explicit merge
metadata, or positive non-637 mutual-exclusion metadata at the failing consumer
program point.

## Suggested Next

Keep Step 2 parked/unselected unless a later diagnostic packet finds explicit
non-637 producer metadata for ordered final-state, mutual-exclusion, or
explicit merge at the failing consumer program point.

## Watchouts

- Do not reopen idea 637's select-materialized semantic-merge contract under
  this idea.
- Do not implement RV64 materialization before prepared/prealloc producer facts
  prove a selected authority family.
- Idea 655 is parked unless new evidence identifies a positive producer seam
  outside idea 637.
- Fresh direct object-route diagnostics supersede older stale fan-in logs for
  `src/930429-1.c` and `src/pr34415.c` in this Step 1 baseline.
- `src/20021204-1.c` remains negative mutual-exclusion evidence only: current
  producer facts do not carry predicate, edge, selected-active-candidate,
  guarded-copy, or destination-authority metadata for `%t20/%t21 -> %t22`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Canonical proof log: `test_after.log`.
