Status: Active
Source Idea Path: ideas/open/460_rv64_move_bundle_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Remaining Move-Bundle Failure

# Current Packet

## Just Finished

Completed Step 1 for idea 460: audited the remaining `20010329-1` RV64
move-bundle residual after idea 459 and saved fresh prepared/object probes plus
the residual-owner table under
`build/agent_state/460_step1_move_bundle_residual_audit/`.

| Route command | Exit | Diagnostic / evidence | Candidate event coordinates | Ownership classification |
| --- | ---: | --- | --- | --- |
| `./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/20010329-1.c` | 0 | Prepared dump reproduced the select-result, dependency-authority, and move-bundle rows. | Prepared rows include block-entry copies at `3/2`, `7/6`, `11/10`; before-instruction stack publication at `4:1`; suppression/register setup at `4:2`, `8:1`, `12:1`. | Evidence source for candidate classification. |
| `./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj -o build/agent_state/460_step1_move_bundle_residual_audit/20010329-1.o tests/c/external/gcc_torture/src/20010329-1.c` | 2 | `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves`. | Object stderr does not identify the first rejecting event. Inherited `gdb_probe.out` reached `fragment_for_prepared_move_bundle` repeatedly but could not print optimized locals. | Exact coordinate remains unobservable from current diagnostics; do not select a lowering packet solely from raw prepared row order. |
| Prepared before-instruction stack publication | n/a | `%t17` has stack-slot home; dependency authority includes `rematerialize_cast_from_source status=available` for `%t16 -> %t17`; stale stack-load row remains `missing_stack_freshness`. | `block_index=4 instruction_index=1`, move `from_value_id=8 to_value_id=9 destination_storage=stack_slot reason=consumer_register_to_stack`. | Strongest candidate owner if coordinate-bearing evidence confirms it: cast-dependency stack-publication/suppression, not idea 459 and not generic stack move support. |
| Prepared suppression/register setup target | n/a | Idea 459 should suppress matching explicit `predecessor_edge_consumed_suppression` register-destination bundles. | `block_index=4 instruction_index=2`, moves `7 -> 10` and `9 -> 10`; later possible register setups at `8:1` and `12:1`. | Closed for idea 459 unless a future coordinate-bearing probe proves the matcher failed on an exact suppression record. |
| Prepared block-entry select copies | n/a | Available `block_entry_publication` rows for `%t22`, `%t36`, and `%t50`. | `block_index=3/2`, `7/6`, `11/10`. | Candidate select-result publication/rematerialization owner only if coordinate evidence shows a parallel-copy event is first; current stderr is the generic move-bundle diagnostic, not the distinct select-publication admission diagnostic. |

## Suggested Next

Define the Step 2 residual-owner contract before selecting implementation:
either add/require coordinate-bearing move-bundle rejection evidence for the
first failing event, or record the exact diagnostic blocker. If the coordinate
proves `block_index=4 instruction_index=1`, keep the next implementation
packet bounded to authorized cast-dependency stack publication and continue to
reject stale stack-load authority.

## Watchouts

- The audit narrowed the candidate set but did not prove the first rejecting
  coordinate because current object diagnostics omit event identity.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, or 459 without coordinate-bearing evidence
  that their exact route failed.
- Do not infer ownership from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump without
  corroborating route evidence.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
