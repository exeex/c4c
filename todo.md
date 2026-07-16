Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Close rejected after accepted commit `a16666851`: durable in-scope collector
migration work remains.

Repaired the current route to target `LirCmpOp.lhs` in `collect_inst_refs`.

## Suggested Next

Execute the next one-field collector migration for plan Steps 1-3:
`LirCmpOp.lhs` in `collect_inst_refs`.

Step 1 should confirm `LirCmpOp.lhs` has the existing semantic carrier needed
to replace raw `S(op.lhs)` scanning with `collect_operand_ref(op.lhs, refs)`.
If confirmed, keep the implementation packet limited to that exact field and
leave every other raw scanner path unchanged.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirCmpOp.rhs`, select, aggregate/vector ops, inline asm, and residual
raw/global text on their current scanner paths. Do not reconstruct references
from rendered names or text. Preserve legacy raw compatibility for
`LirCmpOp.lhs` if the semantic carrier is absent or non-authoritative.

## Proof

Required proof for the next packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
