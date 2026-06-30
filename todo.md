Status: Active
Source Idea Path: ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 462: recorded residual disposition and close
readiness after Step 3 blocked the plain preterminator parallel-copy consumer.
Supporting artifact:
`build/agent_state/462_step4_residual_disposition/disposition.md`.

Disposition: idea 462 is close-ready as a negative route-classification slice
and should split/route the remaining semantic owner. The coordinate-bearing
event remains:

| Field | Value |
| --- | --- |
| Event | `pre_terminator_copies`, `main`, `block_index=10`, `block_label=logic.rhs.end.40`, `instruction_index=0` |
| Bundle | `phase=block_entry`, `authority=out_of_ssa_parallel_copy`, `logic.rhs.end.40 -> logic.end.41` |
| Move | `from_value_id=20` / `%t46` to `to_value_id=21` / `%t50`, `destination_storage=register`, `reason=phi_join_register_to_register` |
| Failure | `fragment_status=generic_move_bundle_materialization_failed` |

No-plain-copy rationale: `%t46` is not proven predecessor-available. It is
defined in successor block `logic.end.41` as `%t46 = bir.ule ptr %t42, %t45`,
after `logic.rhs.end.40` branches there. Treating the prepared `%t46` GPR home
as a predecessor-edge source would infer availability from value-home metadata
alone and would be overbroad.

Follow-up owner boundary: a separate select-edge source-producer
rematerialization audit/implementation should own `%t46 -> %t50`, including
why the existing source-producer consumer does not rematerialize
`%t46 = bir.ule ptr %t42, %t45`, whether duplicate carrier facts
`%t50.phi.sel0` / `%t50.phi.sel1` block the route, and whether `%t42`/`%t45`
are target-consumable at the predecessor edge.

No implementation or test changes were made in this Step 4 packet.

## Suggested Next

Plan-owner close/split review for idea 462. Recommended route: close/retire
idea 462 as a completed negative classification, then split or activate a
focused source-producer rematerialization idea for `%t46 -> %t50`.

## Watchouts

- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not implement a plain GPR copy for `%t46 -> %t50`; `%t46` is defined in
  successor block `logic.end.41`, not in predecessor `logic.rhs.end.40`.
- Do not infer predecessor availability from prepared value homes alone.
- The next owner should explain the select-edge source-producer rematerializer
  rejection for `%t46 = bir.ule ptr %t42, %t45`.
- Do not continue idea 462 with plain preterminator copy materialization unless
  a later source-producer packet proves the source value is actually available
  at the predecessor edge.
- Do not reopen ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence that their exact route owns the first failure.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 4 proof:

```sh
git diff --check
```

Result: passed.
