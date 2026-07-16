Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed plan Step 1 one-field packet for `LirInsertElementOp.elem` in
`collect_inst_refs`: raw `S(op.elem)` scanning was replaced with
`collect_operand_ref(op.elem, refs)` only.

## Suggested Next

Select the next one-field carrier-backed aggregate/vector collector seam; a
natural follow-up is `LirInsertElementOp.vec`, but migrate only that field after
supervisor selection confirms the producer records semantic carrier data.

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

Proof log path: `test_after.log`.

Both commands passed.
