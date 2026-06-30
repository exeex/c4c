Status: Active
Source Idea Path: ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Parallel-Copy Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 462 by routing the first parallel-copy packet as
blocked rather than keeping an unsafe plain-copy consumer. Supporting artifact:
`build/agent_state/462_step3_preterminator_parallel_copy_consumer/blocker.md`.

Fresh object probe still fails at the coordinate-bearing event:

| Field | Value |
| --- | --- |
| Event | `pre_terminator_copies`, `main`, `block_index=10`, `block_label=logic.rhs.end.40`, `instruction_index=0` |
| Bundle | `phase=block_entry`, `authority=out_of_ssa_parallel_copy`, `logic.rhs.end.40 -> logic.end.41` |
| Move | `from_value_id=20` / `%t46` to `to_value_id=21` / `%t50`, `destination_storage=register`, `reason=phi_join_register_to_register` |
| Failure | `fragment_status=generic_move_bundle_materialization_failed` |

Blocking fact: `%t46` is not proven predecessor-available. The fresh prepared
dump shows `%t46 = bir.ule ptr %t42, %t45` is in successor block
`logic.end.41`, after `logic.rhs.end.40` branches to that block. Treating the
prepared `%t46` GPR home as a predecessor-edge source would infer availability
from value home alone and would be an overbroad/raw-copy route.

Classification: this is a select-edge source-producer rematerialization
blocker, not a sound plain preterminator parallel-copy consumer. No
implementation or test changes were kept.

## Suggested Next

Execute Step 4 residual disposition for idea 462. The recommended lifecycle
route is to close/split away from plain preterminator parallel-copy
materialization and open or activate a source-producer rematerialization audit
for `%t46 -> %t50`, especially whether duplicate select carrier facts
`%t50.phi.sel0` / `%t50.phi.sel1` block the existing source-producer consumer.

## Watchouts

- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not implement a plain GPR copy for `%t46 -> %t50`; `%t46` is defined in
  successor block `logic.end.41`, not in predecessor `logic.rhs.end.40`.
- Do not infer predecessor availability from prepared value homes alone.
- The next owner should explain the select-edge source-producer rematerializer
  rejection for `%t46 = bir.ule ptr %t42, %t45`.
- Do not reopen ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence that their exact route owns the first failure.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
