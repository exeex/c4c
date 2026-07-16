Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed the Step 1 `LirShuffleVectorOp.vec2` packet: migrated only the
`collect_inst_refs` shuffle `vec2` field from raw `S(op.vec2)` scanning to
`collect_operand_ref(op.vec2, refs)`. `LirShuffleVectorOp.vec1` remains on
`collect_operand_ref`.

## Suggested Next

Select the next single carrier-backed collector seam under Step 1, preserving
the existing raw scanner compatibility paths for unselected fields.

## Watchouts

Do not perform a broad collector sweep. Leave `LirShuffleVectorOp.mask`, inline
asm, and residual raw/global text on their current scanner paths after the
packet. Do not reconstruct references from rendered names or text. Do not
change producers, verifier semantics, or BIR lowering as part of the next
collector seam packet.

## Proof

Passed:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Proof log: `test_after.log`.
