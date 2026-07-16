Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Accepted commit `142ffce66` completed plan Steps 1-3 for the one-field
`LirSelectOp.cond` packet in `collect_inst_refs`.

## Suggested Next

Next packet: migrate only `LirSelectOp.true_val` in `collect_inst_refs` from
raw `S(op.true_val)` scanning to `collect_operand_ref(op.true_val, refs)`.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirSelectOp.false_val`, aggregate/vector ops, inline asm, and residual
raw/global text on their current scanner paths. Do not reconstruct references
from rendered names or text. Preserve legacy raw compatibility for
`LirSelectOp.true_val` if the semantic carrier is absent or non-authoritative.

## Proof

Required proof for the next packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Latest proof for this packet passed:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
