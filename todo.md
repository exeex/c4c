Status: Active
Source Idea Path: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select The First Follow-Up Implementation Seam

# Current Packet

## Just Finished

Completed Step 4: selected exactly one safe follow-up seam from the Step 3
probe specifications.

Selected follow-up seam: authority-rejection focused probe only.

First probe filename: `tests/backend/case/riscv64_stack_destination_authority_rejection.c`.

Negative proof expectation: prepared/prealloc output for a two-register
fan-in into one stack destination must record the visible source homes and
destination, but the bundle and each move must remain `authority=none`,
`parallel_copy=no`, with fragment status
`producer_authority_missing_for_register_fan_in_stack_destination` when no
matching destination authority producer exists. Unrelated source freshness,
call-preservation, branch-load, select/join, or carrier facts must not satisfy
the destination-authority contract when their consumer point, value,
destination, or semantics do not match.

Why outside idea 637: this follow-up is a fail-closed non-637 boundary probe.
It does not implement or depend on
`SelectMaterializationPreservedStackFallback`, does not materialize a select
result through a preserved stack fallback, and does not authorize a destination
from any `%*.sel*` carrier. The positive fact is intentionally absent; the
probe proves that idea 637-style or unrelated facts cannot authorize
stack-destination register fan-in.

Residual seams intentionally left blocked:

- Ordered final-state authority is blocked because no legal non-637
  `ordered_final_state` producer fact is currently proven.
- Mutual-exclusion authority is blocked because the existing
  `src/20021204-1.c` evidence remains rejection-only and does not contain a
  destination producer fact proving candidate exclusivity.
- Explicit merge authority is blocked because no producer currently publishes
  an explicit merge carrier for the destination stack value at the consumer
  point.

## Suggested Next

Execute Step 5 by preparing a test/probe-only handoff for the selected
authority-rejection seam. The next packet should add
`tests/backend/case/riscv64_stack_destination_authority_rejection.c` as a
negative focused probe without producer implementation, expectation
downgrades, unsupported markers, or allowlist changes.

## Watchouts

- Do not reopen idea 637 through `SelectMaterializationPreservedStackFallback`.
- Do not claim the rejected `src/20021204-1.c` route from unrelated `%t25`
  select facts.
- Do not treat the diagnostic phrase `mutually-exclusive authority
  event_kind=before_instruction_copies` as positive mutual-exclusion evidence;
  the same excerpts report `authority=none` and missing producer authority.
- Keep source freshness separate from destination authority; branch stack-load,
  call-preservation, or source-publication facts do not by themselves authorize
  multi-register fan-in into a stack destination.
- Existing code has only the closed idea 637 semantics enum
  `SelectMaterializationPreservedStackFallback`; do not treat that as the
  selected implementation seam.
- Ordered final-state, mutual-exclusion, and explicit merge remain proposed
  producer contract shapes, not proof that implementation support already
  exists.
- The selected authority-rejection follow-up is negative by design and must
  not be converted into an expectation downgrade or an unsupported marker
  change.
- No positive producer implementation work is selected by this Step 4 packet.

## Proof

No build or ctest proof required by the delegated packet because this was a
selection-only `todo.md` update. Used the current Step 3 probe specifications
in `todo.md` and the active `plan.md`. Did not create or overwrite
`test_after.log`.
