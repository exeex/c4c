Status: Active
Source Idea Path: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Focused Probe Files

# Current Packet

## Just Finished

Completed Step 3: defined focused backend probe file specifications for the
four authority seams before any shared producer implementation. These are
probe specifications only; no test files, implementation files, expectations,
unsupported markers, or build outputs were changed.

Focused probe specifications:

| Seam | Proposed file under `tests/backend/case/` | Primary authority contract | Expected producer fact shape | Why outside idea 637 |
| --- | --- | --- | --- | --- |
| Ordered final-state authority | `riscv64_stack_destination_ordered_final_state_authority.c` | A stack-destination register fan-in bundle is authorized when a producer explicitly names the final destination value at the consumer point after an ordered sequence of contributing register homes. Source freshness remains separate from destination authority. | Positive: prepared/prealloc metadata for the consumer bundle carries `PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` with owner `prepared_stack_destination_register_fan_in`, semantics `ordered_final_state`, destination stack slot/value, ordered contributing source homes, and the same authority on every move in the bundle. Negative paired shape: same two-register-to-stack destination with source homes present but no `ordered_final_state` producer fact remains `authority=none` with `producer_authority_missing_for_register_fan_in_stack_destination`. | It does not materialize a select result through a preserved stack fallback, does not depend on `SelectMaterializationPreservedStackFallback`, and authorizes the stack destination from an ordered final-state producer at the consumer point rather than a select-carrier fallback. |
| Mutual-exclusion authority | `riscv64_stack_destination_mutual_exclusion_authority.c` | A stack-destination register fan-in bundle is authorized only when a producer proves that exactly one mutually-exclusive source candidate can define the destination at the consumer point. | Positive: prepared/prealloc metadata for the bundle carries `PreparedMoveAuthorityKind::StackDestinationRegisterFanIn`, owner `prepared_stack_destination_register_fan_in`, semantics `mutually_exclusive_destination_candidate`, destination stack slot/value, candidate source homes, and a predicate/edge/candidate identifier proving exclusivity at the consumer point. Negative paired shape: a diagnostic or nearby select fact that merely says `mutually-exclusive` without the destination producer fact keeps the bundle rejected as `authority=none`. | It is not the idea 637 selected-stack fallback contract because the authority is a candidate-exclusivity fact for the destination bundle itself, not a select materialization using a preserved fallback slot or `%*.sel*` carrier. |
| Explicit merge authority | `riscv64_stack_destination_explicit_merge_authority.c` | A stack-destination register fan-in bundle is authorized when a producer publishes an explicit merge carrier for the destination stack value at the consumer point. | Positive: prepared/prealloc metadata for the bundle carries `PreparedMoveAuthorityKind::StackDestinationRegisterFanIn`, owner `prepared_stack_destination_register_fan_in`, semantics `explicit_destination_merge`, destination stack slot/value, merge carrier id, participating source homes, and per-move authority. Negative paired shape: merge/select facts for a different value, function, block, or destination do not authorize the bundle and must leave the target destination with `authority=none`. | It uses an explicit merge for the destination stack value, not select materialization fallback. Nearby idea-637-style select facts, carrier aliases, or preserved stack fallback evidence are insufficient unless the merge producer names this destination. |
| Authority rejection | `riscv64_stack_destination_authority_rejection.c` | A two-register-to-stack-destination fan-in must fail closed when no matching destination authority producer exists, even if source homes, source freshness, or unrelated select/join facts are visible. | Negative: prepared/prealloc output records source homes and the stack destination, but the bundle and moves remain `authority=none`, `parallel_copy=no`, with fragment status `producer_authority_missing_for_register_fan_in_stack_destination`; unrelated producer facts are rejected by consumer point, value, destination, or semantics mismatch. Positive fact is intentionally absent in this probe. | It guards idea 655's non-637 boundary by proving that idea 637 select materialization facts, source publication, call-preservation, branch-load freshness, or unrelated joins cannot authorize stack-destination fan-in. |

The probe specs intentionally avoid selecting an implementation packet. They
only name the file targets and the fact shapes that a later Step 4 packet can
use to choose one legal non-637 seam.

## Suggested Next

Execute Step 4: select at most one follow-up implementation seam after
supervisor review of these probe specifications. The cleanest next packet
would add focused probe files only, still without producer implementation, if
the supervisor wants executable observability before selecting producer code.

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
  `SelectMaterializationPreservedStackFallback`; the Step 3 probes must name
  proposed non-637 contract shapes without pretending they already exist.
- Ordered final-state, mutual-exclusion, and explicit merge are proposed
  producer contract shapes, not proof that implementation support already
  exists.
- The authority rejection probe is negative by design and must not be converted
  into an expectation downgrade or an unsupported marker change.
- No producer implementation work is selected by this Step 3 packet.

## Proof

No build or ctest proof required by the delegated packet because this was a
probe-specification-only update. Used the Step 2 seam inventory already in
`todo.md` and the active `plan.md`. Did not create or overwrite
`test_after.log`.
