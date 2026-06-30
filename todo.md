Status: Active
Source Idea Path: ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Diagnostic Coordinate Gap

# Current Packet

## Just Finished

Closed idea 460 as complete residual-owner audit documentation. The remaining
blocker is observability: current RV64 object diagnostics do not identify the
first failing move-bundle event coordinate, so no semantic lowering owner can
be selected safely.

## Suggested Next

Execute Step 1 for idea 461. Re-read:

- `ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md`
- `ideas/closed/460_rv64_move_bundle_residual_owner_audit.md`
- `build/agent_state/460_step2_residual_disposition/disposition.md`
- `build/agent_state/460_step1_move_bundle_residual_audit/audit.md`
- `build/agent_state/460_step1_move_bundle_residual_audit/20010329-1.prepared.out`
- `build/agent_state/460_step1_move_bundle_residual_audit/20010329-1.object.err`

Create `build/agent_state/461_step1_diagnostic_coordinate_gap/` and record a
coordinate-gap table: missing diagnostic fields, candidate event families,
current object-route evidence, and possible diagnostic/probe surfaces. Do not
make semantic lowering edits in Step 1.

## Watchouts

- This is diagnostics/probe support, not semantic lowering.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, or 459 without coordinate-bearing evidence.
- Do not infer ownership from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump alone.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Lifecycle activation validation:

```sh
git diff --check
```

Result: passed.
