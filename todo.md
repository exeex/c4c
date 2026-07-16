Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Plan-owner rejected closure for 845 because durable source scope remains:
`collect_inst_refs` still scans remaining carrier-capable fields after six
accepted one-field migrations. The active route is repaired for the seventh
packet.

## Suggested Next

Execute Step 1 for the next exact one-field migration:
`LirBinOp.lhs` in `collect_inst_refs`.

Confirm the selected field, consumer, proof target, and non-goals before code
edits. If readiness holds, Step 2 should replace only `S(op.lhs)` in the
`LirBinOp` arm with `collect_operand_ref(op.lhs, refs)`.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirBinOp.rhs`, compare, select, aggregate/vector ops, inline asm, and residual
raw/global text on their current scanner paths. Do not reconstruct references
from rendered names or text.

## Proof

Last accepted proof before this repair:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Next implementation packet must run fresh proof after code changes.
