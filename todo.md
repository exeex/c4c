Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Plan-owner repaired the exhausted one-field runbook after accepted commit
`a067e8cc5` completed the prior `LirInsertValueOp.elem` target.

## Suggested Next

Execute Step 1 for the next bounded packet: confirm `LirShuffleVectorOp.vec1`
in `collect_inst_refs` is the selected carrier-backed field, then migrate only
that field from raw `S(op.vec1)` scanning to
`collect_operand_ref(op.vec1, refs)`.

## Watchouts

Do not perform a broad collector sweep. Leave `LirShuffleVectorOp.vec2`,
`LirShuffleVectorOp.mask`, inline asm, and residual raw/global text on their
current scanner paths. Do not reconstruct references from rendered names or
text. Do not change producers, verifier semantics, or BIR lowering unless
`LirShuffleVectorOp.vec1` is disproven during executor inspection.

## Proof

Required after implementation:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
