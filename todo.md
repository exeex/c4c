Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed plan Steps 1-3 for the one-field `LirSelectOp.true_val` packet in
`collect_inst_refs`: `op.true_val` now uses `collect_operand_ref(op.true_val,
refs)`, with focused semantic LinkNameId and legacy raw compatibility coverage.

## Suggested Next

Supervisor should select the next packet. A coherent adjacent candidate is
`LirSelectOp.false_val` in `collect_inst_refs`, but no follow-on field was
implemented or lifecycle-repaired in this packet.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirSelectOp.false_val`, aggregate/vector ops, inline asm, and residual
raw/global text on their current scanner paths. Do not reconstruct references
from rendered names or text. `LirSelectOp.true_val` legacy raw compatibility is
covered by the nearby frontend HIR test.

## Proof

Latest proof for this packet passed:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
