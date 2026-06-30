Status: Active
Source Idea Path: ideas/open/460_rv64_move_bundle_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Remaining Move-Bundle Failure

# Current Packet

## Just Finished

Closed idea 459 as complete for explicit RV64
`predecessor_edge_consumed_suppression` consumption. Fresh `20010329-1` object
route still fails with broad `unsupported_move_bundle_target_shape`, but Step 4
classified that residual as outside an exact 459 packet.

## Suggested Next

Execute Step 1 for idea 460. Re-read:

- `ideas/open/460_rv64_move_bundle_residual_owner_audit.md`
- `ideas/closed/459_rv64_select_edge_suppression_placement_consumer.md`
- `build/agent_state/459_step4_residual_disposition/disposition.md`
- `build/agent_state/459_step4_residual_disposition/probe_status.tsv`
- `build/agent_state/459_step4_residual_disposition/20010329-1.prepared.out`
- `build/agent_state/459_step4_residual_disposition/20010329-1.object.err`
- `build/agent_state/459_step4_residual_disposition/gdb_probe.out`

Create `build/agent_state/460_step1_move_bundle_residual_audit/` and record a
residual-owner table for the first remaining unsupported move-bundle event:
route command, exit status, object diagnostic, candidate event coordinates,
prepared move-bundle rows, and ownership classification. Do not make
implementation edits in Step 1.

## Watchouts

- This is an audit, not an implementation packet.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, or 459.
- Do not infer ownership from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump without
  corroborating route evidence.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Lifecycle activation validation:

```sh
git diff --check
```

Result: passed.
