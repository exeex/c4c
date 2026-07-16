Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed plan Step 1 one-field packet for `LirExtractElementOp.vec` in
`collect_inst_refs`: raw `S(op.vec)` scanning was replaced with
`collect_operand_ref(op.vec, refs)` only.

## Suggested Next

Select the next one-field carrier-backed aggregate/vector collector seam and
migrate only that field after confirming the collector has a raw scanner
consumer and the producer records semantic carrier data.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. `LirInsertValueOp`,
unselected vector operands, inline asm, and residual raw/global text remain on
their current scanner paths. Do not reconstruct references from rendered names
or text.

## Proof

Required implementation proof for this packet passed:

```
{ cmake --build build && ctest --test-dir build -R '^backend_lir_native_vector_authority$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Both commands passed. Proof log: `test_after.log`.
