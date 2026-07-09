Status: Active
Source Idea Path: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split Residuals Into Authority Seams

# Current Packet

## Just Finished

Completed Step 2: split the residual non-637 stack-destination fan-in rows
from `build/agent_state/647_step2_family_revision/` into authority seams.
Existing evidence does not prove a legal non-637 producer fact for any
implementation packet, so no Step 3 implementation packet is selected.

Seam inventory:

| Target | Consumer point | Classification | Producer fact that would prove authority | Fail-closed negative state |
| --- | --- | --- | --- | --- |
| `src/920429-1.c` | `main:entry` before instruction `8`; `%t5`/value `22` register `s2` and `%t6`/value `20` register `t0` to `%t7`/value `21` stack slot `6` offset `24` | rejection-only; no ordered final-state, mutual-exclusion, or explicit-merge fact appears at this consumer point | none currently proven. A future non-637 positive would need `PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` on the bundle and all moves, owner `prepared_stack_destination_register_fan_in`, and a new non-637 semantics that designates either the final stack-slot state, the active predicate/edge candidate, or an explicit merge for value `21` at `main:entry` before instruction `8` | remains rejected when `authority=none`, `parallel_copy=no`, and fragment status is `producer_authority_missing_for_register_fan_in_stack_destination`; nearby `f:tern.end.11` select facts for `%t12` with `select_carrier_alias_authority status=unsupported_publication` must not authorize the `main` bundle |
| `src/930429-1.c` | `main:entry` before instruction `6`; `%t2`/value `12` register `t0` and `%t5`/value `15` register `s2` to `%t6`/value `14` stack slot `7` offset `24` | rejection-only; evidence has only the ambiguous two-register-source stack destination at the consumer point | none currently proven. A future non-637 positive would need a producer authority fact for destination value `14` at `main:entry` before instruction `6`, with both participating moves carrying `StackDestinationRegisterFanIn` authority and source homes/freshness published separately | remains rejected when `authority=none`, `parallel_copy=no`, and no matching select-chain, ordered-final-state, predicate, edge, guarded-copy, or merge carrier is present for value `14` |
| `src/pr34415.c` | `main:entry` before instruction `6`; `%t2`/value `45` register `t0` and `%t5`/value `48` register `s2` to `%t7`/value `47` stack slot `10` offset `24` | rejection-only for the failing `main` consumer; unrelated `foo` join/select facts are not authority for this row | none currently proven. A future non-637 positive would need `StackDestinationRegisterFanIn` authority on the `main:entry` bundle and all moves, with a producer fact for destination value `47` naming final-state, mutually-exclusive candidate, or explicit merge semantics at the same consumer point | remains rejected when the only visible select authorities belong to `foo` joins, including `missing_final_carrier`, `missing_carrier_aliases`, or `unsupported_publication`, while the failing `main` bundle still has `authority=none` |
| `src/pr70005.c` | `fn1:logic.end.73` before instruction `2`; `%t83`/value `53` register `s1` and `%t84`/value `54` register `s2` to `%t85`/value `55` stack slot `7` offset `28` | rejection-only for the failing stack destination; nearby select-chain evidence proves other destinations, not this bundle | none currently proven. A future non-637 positive would need producer metadata at `fn1:logic.end.73` before instruction `2` for destination value `55`, with bundle/move `StackDestinationRegisterFanIn` authority and a named non-637 semantics for ordered final state, mutual exclusion, or explicit merge | remains rejected when destination value `55` has `authority=none`, `parallel_copy=no`, and fragment status `producer_authority_missing_for_register_fan_in_stack_destination`; nearby select facts for `%t88`/value `57`, `%t83`/value `53`, or earlier join results, including `missing_carrier_aliases` and `unsupported_publication`, are not sufficient |
| `src/ptr-arith-1.c` | `main:entry` before instruction `8`; `%t5`/value `14` register `s2` and `%t6`/value `12` register `t0` to `%t7`/value `13` stack slot `3` offset `24` | rejection-only; no consumer-point producer authority surface appears in the excerpt | none currently proven. A future non-637 positive would need a `StackDestinationRegisterFanIn` producer fact for destination value `13` at `main:entry` before instruction `8`, with all participating source homes recorded and source freshness proven separately | remains rejected when `authority=none`, `parallel_copy=no`, and no ordered-final-state designation, predicate/edge selected candidate, guarded copy, or explicit merge carrier exists |

Out-of-scope row:

| Target | Reason |
| --- | --- |
| `src/20011109-2.c` | Only fits idea 637's selected `SelectMaterializationPreservedStackFallback` contract: `select_materialization` at `main:block_1` instruction `9`, preserved stack fallback into `%t12.sel1`/value `17`, and required semantics `select_materialization_preserved_stack_fallback`. This is real evidence but remains out of scope for idea 655 and must not be selected as a non-637 Step 3 packet. |

Rejected mutual-exclusion route retained:

| Target | Reason |
| --- | --- |
| `src/20021204-1.c` | The failing `%t20`/value `19` and `%t21`/value `20` to `%t22`/value `18` stack destination at `main:tern.end.12` before instruction `1` still has `authority=none`, `parallel_copy=no`, and no predicate, selected-active-candidate, guarded-copy, edge, or carrier fact. Nearby `%t17`/`%t24 -> %t25` select facts are unrelated and remain illegal as authority. |

Step 2 conclusion: every legal non-637 positive seam is still evidence-missing.
The only current non-unknown seam classification supported by the existing
evidence is rejection-only, with the fail-closed states above. Step 3 should
define focused probes for ordered final-state, mutual-exclusion, explicit
merge, and rejection authority before any producer implementation is selected.

## Suggested Next

Execute Step 3: define focused probe files under `tests/backend/case/` for
ordered final-state authority, mutual-exclusion authority, explicit merge
authority, and authority rejection. Keep them as probe specifications or tests
only; do not implement producer authority until one probe names a legal
non-637 positive producer fact shape and a fail-closed negative case.

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
- No Step 3 implementation work is selected until a legal non-637 seam has a
  producer fact shape and fail-closed negative proof.

## Proof

No build or ctest proof required by the delegated packet. Used existing
evidence only: `build/agent_state/647_step2_family_revision/summary.md`,
per-row `diagnostic_excerpt.txt`, `authority_extract.txt`, and targeted
prepared dump excerpts under that directory. Did not rerun diagnostics and did
not create or overwrite `test_after.log`.
