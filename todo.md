Status: Active
Source Idea Path: ideas/open/463_select_edge_ule_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Edge Source-Producer Rejection

# Current Packet

## Just Finished

Completed Step 1 audit for idea 463. The existing select-edge source-producer
consumer sees the right failure family for `%t46 -> %t50` on
`logic.rhs.end.40 -> logic.end.41`, but it remains fail-closed because the
representative source producer is consumed through duplicate carrier selects
that are not explicitly authorized as carrier-only aliases of the final join
result.

Rejection/audit table:

| Field | Value |
| --- | --- |
| Event | `pre_terminator_copies`, `main`, `block_index=10`, `block_label=logic.rhs.end.40`, `instruction_index=0` |
| Edge | `logic.rhs.end.40 -> logic.end.41` |
| Move | `%t46` / `from_value_id=20` to `%t50` / `to_value_id=21`, `destination_storage=register`, destination home `t0` |
| Source producer | `%t46 = bir.ule ptr %t42, %t45` in `logic.end.41`; publication class is the selected binary source-producer route |
| `%t50.phi.sel0` / `%t50.phi.sel1` carrier facts | Both consume `%t46` before final `%t50`; they are duplicate carrier values, not the join result name itself |
| `%t42` operand | Home `%t42 value_id=18 kind=register reg=s1`; loaded before the predecessor edge, so the current register/immediate operand helper can accept it |
| `%t45` operand | Home `%t45 value_id=19 kind=register reg=s2`; current operand helper can accept the register home, but no separate edge-dependency authority is printed |
| Existing consumer behavior | `fragment_for_prepared_select_edge_source_producer` requires the binary result to have only approved carrier uses; `prepared_select_edge_binary_source_has_only_carrier_uses` currently requires carrier select results to match `join_transfer.result` |
| First missing fact | Durable prepared carrier-alias authority that proves `%t50.phi.sel0` / `%t50.phi.sel1` are carrier-only aliases for final `%t50`, allowing rematerialization of `%t46` directly into `%t50` without raw shape inference |
| Classification | Not a plain-copy packet and not a missing GPR-home packet. This is a duplicate-carrier source-producer policy/metadata gap. |

Artifact:
`build/agent_state/463_step1_select_edge_ule_source_producer_audit/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the duplicate-carrier select-edge
source-producer contract. The contract should either require explicit prepared
carrier-alias authority for `%t50.phi.sel0` / `%t50.phi.sel1`, or route Step 3
to a producer/prepared metadata packet if that relationship cannot be expressed
without raw-shape inference.

Suggested artifact directory:
`build/agent_state/463_step2_select_edge_ule_source_producer_contract/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
- Do not infer predecessor availability from prepared value homes alone.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, 459, 460, 461, or 462 without new
  coordinate-bearing evidence that their exact route owns the first failure.
- Do not match on testcase name, function name, block label, value ids alone,
  raw BIR shape, or prepared dump order.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
