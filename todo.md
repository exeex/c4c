Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed plan step 1 packet migrating only `LirInsertValueOp.agg` in
`collect_inst_refs` from raw scanner collection to
`collect_operand_ref(op.agg, refs)`. `LirInsertValueOp.elem` remains on the
raw scanner path.

## Suggested Next

After proof, hand back to supervisor for the next one-field carrier-backed
collector packet selection.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirInsertValueOp.elem`, `LirShuffleVectorOp.vec1`,
`LirShuffleVectorOp.vec2`, inline asm, and residual raw/global text on their
current scanner paths. Do not reconstruct references from rendered names or
text. Do not change producers, verifier semantics, tests, or BIR lowering
unless the selected field is disproven during executor inspection.

## Proof

Proof completed for this packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Result: passed. Proof log path: `test_after.log`.
