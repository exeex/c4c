Status: Active
Source Idea Path: ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Coordinate-Bearing Parallel-Copy Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 462: audited the coordinate-bearing
`pre_terminator_copies` / `out_of_ssa_parallel_copy` evidence and prepared
value-home facts for the exact `logic.rhs.end.40 -> logic.end.41` predecessor
edge. Supporting artifact:
`build/agent_state/462_step1_preterminator_parallel_copy_audit/audit.md`.

Audit table:

| Field | Value |
| --- | --- |
| Object-route event | `event_kind=pre_terminator_copies`, `function=main`, `block_index=10`, `block_label=logic.rhs.end.40`, `instruction_index=0` |
| Prepared move bundle | `phase=block_entry`, `authority=out_of_ssa_parallel_copy`, `block_index=10`, `instruction_index=0`, `source_parallel_copy=logic.rhs.end.40 -> logic.end.41` |
| Edge identity | predecessor `logic.rhs.end.40`, successor `logic.end.41`, `execution_site=predecessor_terminator`, `execution_block=logic.rhs.end.40` |
| Join/select publication | `join_transfer logic.end.41 result=%t50 kind=phi_edge carrier=select_materialization ownership=authoritative_branch_pair`; incoming `%t46 -> %t50` |
| Move | `from_value_id=20` / `%t46` to `to_value_id=21` / `%t50`, `destination_storage=register`, `op_kind=move`, `reason=phi_join_register_to_register`, `uses_cycle_temp_source=no` |
| Source availability | `%t46 value_id=20 kind=register reg=t0`; storage `encoding=register bank=gpr reg=t0 width=1` |
| Destination availability | `%t50 value_id=21 kind=register reg=t0`; storage `encoding=register bank=gpr reg=t0 width=1` |
| Edge publication | `block_entry_publication successor=logic.end.41 status=available to_value_id=21 ... reg=t0 block_index=10 instruction_index=0` |
| Excluded authority paths | `select_edge_suppression_authorized=no`, `cast_dependency_stack_publication_authorized=no` |
| Current RV64 failure | `fragment_status=generic_move_bundle_materialization_failed` after bundle admission; generic `fragment_for_prepared_move_bundle` lacks this register-source route |

Classification: Step 2 can define a bounded RV64 consumer contract. The
representative has enough prepared authority for a narrow contract: explicit
coordinate-bearing event identity, matching parallel-copy and join-transfer
facts, available destination publication, and GPR-compatible source/destination
homes. No producer/prepared metadata blocker is visible for this event.

## Suggested Next

Execute Step 2: define the preterminator parallel-copy materialization
contract. The first candidate contract should accept only coordinate-backed
`pre_terminator_copies` / `out_of_ssa_parallel_copy` predecessor-terminator
bundles with single acyclic register-destination moves, available edge
publication, and GPR-compatible source/destination facts; it should explicitly
reject raw-only, mismatched-edge, missing-home, non-register destination,
stack/stale-load, cycle-temp, and generic move-bundle shapes.

## Watchouts

- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not treat value ids, block labels, or the testcase name as authority; Step
  2 should consume coordinate/authority-backed prepared facts.
- The representative has source and destination both in `t0`; a future
  materialization contract may admit no-op emission for matching GPR homes, but
  must not assume all register-register copies are no-ops.
- Do not reopen ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence that their exact route owns the first failure.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
