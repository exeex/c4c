Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Closure rejected after the accepted `LirInsertElementOp.vec` packet because
the source idea still has durable in-scope aggregate/vector collector
migration remaining. Accepted commits in this sequence migrated
`LirExtractValueOp.agg`, `LirExtractElementOp.vec`,
`LirInsertElementOp.elem`, and `LirInsertElementOp.vec`.

## Suggested Next

Execute the next one-field carrier-backed collector packet:
`LirInsertValueOp.agg` in `collect_inst_refs`. Replace only the raw
`S(op.agg)` in the `LirInsertValueOp` arm with
`collect_operand_ref(op.agg, refs)`.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirInsertValueOp.elem`, `LirShuffleVectorOp.vec1`,
`LirShuffleVectorOp.vec2`, inline asm, and residual raw/global text on their
current scanner paths. Do not reconstruct references from rendered names or
text. Do not change producers, verifier semantics, tests, or BIR lowering
unless the selected field is disproven during executor inspection.

## Proof

Required implementation proof for the next packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Proof log path: `test_after.log`.
