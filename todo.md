Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Revise Producer Authority Family Selection

# Current Packet

## Just Finished

Route checkpoint after the Step 3 discovery packet for the selected row
`src/20021204-1.c` at
`main:tern.end.12` before instruction 1. Fresh evidence lives under
`build/agent_state/647_step3_mutual_exclusion_probe/src_20021204-1.c/`.

Result: no acceptable prepared/prealloc mutual-exclusion proof exists for the
two register-source stack-destination producers at this consumer point. The
fresh object-route diagnostic still reports `authority=none`, `move_count=2`,
`parallel_copy=no`, `move[0].from_value_id=19`, `move[1].from_value_id=20`,
both targeting stack destination `to_value_id=18`, with
`fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

The fresh `--dump-prepared-bir` shows the failing bundle is the unpredicated
consumer bundle for `%t20` and `%t21` into `%t22`:

- `%t22 = bir.sub i64 %t20, %t21`
- `%t22` is `value_id=18`, home `stack_slot`
- `%t20` is `value_id=19`, home `register`
- `%t21` is `value_id=20`, home `register`
- `move_bundle phase=before_instruction authority=none block_index=5
  instruction_index=1`

The only explicit edge/control-flow authority nearby is for the different
select materialization `%t17/%t24 -> %t25`: `join_transfer tern.end.12
result=%t25`, edge transfers from `tern.then.end.9` and `tern.else.end.11`,
and predecessor `parallel_copy` records for `%t25`. Those facts do not
authorize `%t20/%t21 -> %t22`.

Producer-surface checks for `%t22` are negative:

- `block_entry_publication` for `to_value_id=18` reports
  `status=unsupported_destination_storage` on both incoming predecessor
  blocks.
- `current_block_join_parallel_copy_source` for `destination=%t22`,
  `destination_value_id=18`, `source=%t20`, `source_value_id=19` reports
  `status=missing_publication`, `source_freshness_status=no_candidate`, and
  no incoming expression/source identity on both predecessor edges.
- No matching predicate, selected-active-candidate, guarded-copy, or
  destination-authority carrier is present for `source_value_id=20`.

Conclusion: Step 3 should stop for the selected mutual-exclusion family. Any
implementation that publishes authority for this row from source availability,
same-block order, arithmetic operand shape, value-id shape, diagnostics,
testcase identity, final assembly, or the unrelated `%t25` select edge facts
would be overfit route drift.

Lifecycle decision: return the active route to Step 2. The attempted
mutual-exclusion family for `src/20021204-1.c` is not an executable Step 3
implementation target. The source idea remains valid because its acceptance
criteria allow recording precise producer evidence that no legal first packet
is available.

## Suggested Next

Execute Step 2 family revision before any new Step 3 implementation packet.

Recommended next packet:

- Re-open the residual classification and select a different first family only
  if evidence supports it.
- Treat ordered final-state authority as the likely next candidate to inspect,
  but require proof that the producer designates a final authoritative
  stack-slot state at the consumer point.
- Keep `src/20021204-1.c` `%t20/%t21 -> %t22` out of scope for
  mutual-exclusion publication unless new producer proof appears.
- If no residual row can publish ordered final-state, merge, mutual-exclusion,
  or another explicit prepared/prealloc destination-authority fact, stop Step 2
  and create or request a separate proof-discovery lifecycle instead of
  forcing implementation.

Step 2 completion should update this file with the revised selected family,
first target row, positive and negative examples, fact shape, owner label,
negative statuses, and intentionally excluded residuals.

## Watchouts

- The `%t25` select materialization has branch/edge authority, but the selected
  failing destination is `%t22`. Do not transfer authority across those
  surfaces.
- `%t20` appears in edge-preservation stack moves at block entry, but those
  publications are `unsupported_destination_storage`/`missing_publication` for
  `%t22` and do not prove mutual exclusion.
- There is no durable evidence for `%t21` as a predicated/edge-selected
  stack-destination producer at this consumer point.
- Keep rejecting source availability, same-block order, source freshness,
  value-id shape, diagnostic wording, testcase identity, and final assembly as
  Step 3 authority.
- Do not treat the phrase "likely ordered final-state" as selection. It is only
  the next candidate family to inspect unless fresh evidence proves the
  final-state authority contract.

## Proof

No build/test proof was required for this lifecycle-only route revision. Did
not create or overwrite `test_after.log`.

Commands/evidence:

- `build/c4cll -I . --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/20021204-1.c` wrote
  `build/agent_state/647_step3_mutual_exclusion_probe/src_20021204-1.c/fresh_dump_prepared_bir.txt`
  with exit code `0`.
- The focused object-route diagnostic wrote
  `build/agent_state/647_step3_mutual_exclusion_probe/src_20021204-1.c/fresh_case.log`
  with exit code `1`, still reporting
  `producer_authority_missing_for_register_fan_in_stack_destination`.
- Packet conclusion is summarized in
  `build/agent_state/647_step3_mutual_exclusion_probe/src_20021204-1.c/evidence_summary.md`.
