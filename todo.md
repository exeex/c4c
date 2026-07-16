Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed Step 1 packet for `LirShuffleVectorOp.vec1`: migrated only the
`collect_inst_refs` shuffle `vec1` field from raw `S(op.vec1)` scanning to
`collect_operand_ref(op.vec1, refs)`. `LirShuffleVectorOp.vec2` remains on the
raw scanner path.

## Suggested Next

Execute the next bounded collector packet: confirm `LirShuffleVectorOp.vec2`
has an acceptable semantic carrier, then migrate only that field from raw
`S(op.vec2)` scanning if selected by the supervisor.

## Watchouts

Do not perform a broad collector sweep. Leave `LirShuffleVectorOp.vec2`,
`LirShuffleVectorOp.mask`, inline asm, and residual raw/global text on their
current scanner paths. Do not reconstruct references from rendered names or
text. Do not change producers, verifier semantics, or BIR lowering unless
`LirShuffleVectorOp.vec1` is disproven during executor inspection.

## Proof

Passed:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Proof log: `test_after.log`.
