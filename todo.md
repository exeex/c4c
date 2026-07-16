Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Accepted commit `6d9ecb05f` completed plan Step 1-3 for the one-field
`LirSelectOp.false_val` packet in `collect_inst_refs`: raw
`S(op.false_val)` scanning was replaced with
`collect_operand_ref(op.false_val, refs)` only. Closure is rejected because
durable in-scope aggregate/vector collector migration remains.

## Suggested Next

Implement the next one-field packet: migrate `LirExtractValueOp.agg` in
`collect_inst_refs` from raw `S(op.agg)` scanning to
`collect_operand_ref(op.agg, refs)`.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirInsertValueOp`, vector ops, inline asm, and residual raw/global text on
their current scanner paths. Do not reconstruct references from rendered names
or text. This packet should touch only `LirExtractValueOp.agg`.

## Proof

Required implementation proof:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Both commands passed. Proof log: `test_after.log`.
