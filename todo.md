Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select One Producer Authority Family Or Park

# Current Packet

## Just Finished

Completed `plan.md` Step 2 as a no-implementation decision packet. The route
is blocked/parked: no non-637 producer authority family is selected from the
refreshed Step 1 evidence.

Decision basis:

| Candidate family | Step 2 decision | Missing or negative producer evidence |
| --- | --- | --- |
| Ordered final-state authority | Not selected | No refreshed row exposes producer metadata designating one final authoritative stack-slot state at the failing consumer program point. |
| Mutual-exclusion authority | Not selected | `src/20021204-1.c` remains negative evidence: the available edge/select facts authorize `%t17/%t24 -> %t25`, not the failing `%t20/%t21 -> %t22` stack-destination bundle, which still has no predicate, edge, selected-active-candidate, guarded-copy, or destination-authority carrier fact at the consumer point. |
| Explicit merge authority | Not selected | No refreshed row exposes semantic-equivalence metadata or an explicit merge operation covering all candidate sources targeting the same stack destination. |
| Reopened idea-637 select-materialized semantic merge | Not selected under idea 647 | `src/20011109-2.c` has select-materialized chain evidence, but that matches closed idea 637 rather than a legal non-637 family for this plan. |
| Rejection-only or stale rows | Not an implementation family | `src/920429-1.c`, `src/ptr-arith-1.c`, and `src/pr70005.c` only prove fail-closed `authority=none`; `src/930429-1.c` and `src/pr34415.c` are stale for this route because their current first owners are not stack-destination fan-in. |

This satisfies Step 2 by recording the blocked outcome with the missing evidence
named. Step 3 implementation is not currently legal because the source
files/artifacts do not contain explicit positive non-637 producer metadata at
the failing consumer point.

## Suggested Next

Route to plan-owner/lifecycle review to decide whether this active runbook
should be closed, parked, rewritten around proof discovery, or replaced by a
different open idea. Do not delegate implementation for Step 3 unless new
prepared/prealloc evidence first exposes positive non-637 ordered final-state,
mutual-exclusion, or explicit-merge producer metadata at the failing consumer
program point.

## Watchouts

- Do not reopen idea 637's select-materialized semantic-merge contract under
  this idea.
- Do not implement RV64 materialization before prepared/prealloc producer facts
  prove a selected authority family.
- Idea 655's parked outcome still applies: ordered final-state,
  mutual-exclusion, and explicit merge positive producer seams remain blocked
  until a legal non-637 producer authority fact is proven.
- The implementation split requires producer/prealloc ownership first; RV64 is
  only a downstream consumer after an accepted destination-authority contract
  exists.
- Rejection-only diagnostics are useful negative proof, but they do not select
  a producer family.

## Proof

No new build was required for this todo-only decision packet. This decision
cites the existing Step 1 proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

The proof passed and is recorded in `test_after.log`; the Step 1 evidence
summary is
`build/agent_state/647_step1_20260710_residual_baseline/summary.md`.
