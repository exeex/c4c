Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Supervisor accepted and committed `8c57acace`, completing the previous
one-field packet that migrated only `LirInsertValueOp.agg` in
`collect_inst_refs` from raw scanner collection to
`collect_operand_ref(op.agg, refs)`.

## Suggested Next

Repair-current-route: execute the next one-field packet by migrating only
`LirInsertValueOp.elem` in `collect_inst_refs` from `S(op.elem)` to
`collect_operand_ref(op.elem, refs)`, then prove parity with the focused
collector test.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirShuffleVectorOp.vec1`, `LirShuffleVectorOp.vec2`, inline asm, and residual
raw/global text on their current scanner paths. Do not reconstruct references
from rendered names or text. Do not change producers, verifier semantics, or
BIR lowering unless the selected field is disproven during executor inspection.
`LirInsertValueOp.agg` is already accepted; do not revisit it.

## Proof

Required proof for the next packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
