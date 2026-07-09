Status: Active
Source Idea Path: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Blocked Failure-Family Baseline

# Current Packet

## Just Finished

Completed Step 1: Established the blocked failure-family baseline from
`build/agent_state/647_step2_family_revision/summary.md` and its existing
per-row excerpts. The baseline is idea 647 Step 2's refreshed
stack-destination register fan-in residual family, still fail-closed because
the observed consumer bundles have no legal non-637 producer authority fact at
the consumer point.

Rejected routes preserved:

- `src/20021204-1.c` mutual-exclusion route: rejected. The failing consumer is
  `main:tern.end.12` before instruction `1`, with `%t20`/value `19` register
  and `%t21`/value `20` register both targeting `%t22`/value `18` stack slot
  `16` offset `120`. Current authority is `none`, `parallel_copy=no`, and
  fragment status is
  `producer_authority_missing_for_register_fan_in_stack_destination`. Nearby
  `%t17`/`%t24 -> %t25` select facts are unrelated to the `%t20`/`%t21 ->
  %t22` bundle and remain illegal as authority.
- `src/20011109-2.c` idea 637 route: rejected for this decomposition. The
  failing consumer is `main:block_1` before instruction `9`, destination
  `%t12.sel1`/value `17` stack slot `15` offset `32`, sources `%t10`/value
  `10` register, `%t12.elt1`/value `13` stack slot `12` offset `29`, and
  `%t12.sel2`/value `16` register. Current authority is `none`; fragment
  status is `missing_stack_destination_fan_in_authority_fact`. The only
  positive producer shape is `select_materialization` preserved-stack fallback,
  which belongs to closed idea 637 and is out of scope here.

Remaining residual row shapes:

| Target | Consumer point | Destination | Source homes | Current authority | Fragment status |
| --- | --- | --- | --- | --- | --- |
| `src/920429-1.c` | `main:entry` before instruction `8` | `%t7`/value `21`, stack slot `6` offset `24` | `%t5`/value `22` register `s2`; `%t6`/value `20` register `t0` | `none`, `parallel_copy=no` | `producer_authority_missing_for_register_fan_in_stack_destination` |
| `src/930429-1.c` | `main:entry` before instruction `6` | `%t6`/value `14`, stack slot `7` offset `24` | `%t2`/value `12` register `t0`; `%t5`/value `15` register `s2` | `none`, `parallel_copy=no` | `producer_authority_missing_for_register_fan_in_stack_destination` |
| `src/pr34415.c` | `main:entry` before instruction `6` | `%t7`/value `47`, stack slot `10` offset `24` | `%t2`/value `45` register `t0`; `%t5`/value `48` register `s2` | `none`, `parallel_copy=no` | `producer_authority_missing_for_register_fan_in_stack_destination` |
| `src/pr70005.c` | `fn1:logic.end.73` before instruction `2` | `%t85`/value `55`, stack slot `7` offset `28` | `%t83`/value `53` register `s1`; `%t84`/value `54` register `s2` | `none`, `parallel_copy=no` | `producer_authority_missing_for_register_fan_in_stack_destination` |
| `src/ptr-arith-1.c` | `main:entry` before instruction `8` | `%t7`/value `13`, stack slot `3` offset `24` | `%t5`/value `14` register `s2`; `%t6`/value `12` register `t0` | `none`, `parallel_copy=no` | `producer_authority_missing_for_register_fan_in_stack_destination` |

No Step 3 implementation packet is currently selected.

## Suggested Next

Execute Step 2: split the residual non-637 rows into authority seams
(`ordered final-state`, `mutual-exclusion`, `explicit merge`,
`rejection-only`, or `unknown`) and name the producer fact plus fail-closed
negative state required for each seam.

## Watchouts

- Do not reopen idea 637 through `SelectMaterializationPreservedStackFallback`.
- Do not claim the rejected `src/20021204-1.c` route from unrelated `%t25`
  select facts.
- Keep source freshness separate from destination authority; branch stack-load,
  call-preservation, or source-publication facts do not by themselves authorize
  multi-register fan-in into a stack destination.
- Keep the next packet classification-focused. No Step 3 implementation work
  is selected until a legal non-637 seam has a producer fact shape and
  fail-closed negative proof.

## Proof

No build or ctest proof required by the delegated packet. Used existing
evidence only: `build/agent_state/647_step2_family_revision/summary.md`,
per-row `diagnostic_excerpt.txt`, `authority_extract.txt`, and targeted
prepared dump excerpts under that directory. Did not rerun diagnostics and did
not create or overwrite `test_after.log`.
