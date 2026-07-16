Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed Step 2/Step 3 for the selected one-field packet: migrated only
`LirInsertValueOp.elem` in `collect_inst_refs` from raw `S(op.elem)` scanning
to `collect_operand_ref(op.elem, refs)`, preserving all unselected scanner
paths.

## Suggested Next

After this packet, supervisor should select the next exact carrier-backed
collector field or route lifecycle repair if no ready field remains.

## Watchouts

Do not perform a broad collector sweep. Leave `LirInsertValueOp.agg` on
`collect_operand_ref`, and leave `LirShuffleVectorOp.vec1`,
`LirShuffleVectorOp.vec2`, inline asm, and residual raw/global text on their
current scanner paths. Do not reconstruct references from rendered names or
text. Do not change producers, verifier semantics, or BIR lowering unless
`LirInsertValueOp.elem` is disproven during executor inspection.

## Proof

Passed:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Proof log: `test_after.log`.
