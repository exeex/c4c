Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove and Hand Off One Collector Migration

# Current Packet

## Just Finished

Completed the seventh one-field collector migration for plan Steps 1-3:
`LirBinOp.lhs` in `collect_inst_refs` now uses
`collect_operand_ref(op.lhs, refs)`.

Added focused dead-internal reachability coverage proving lhs `LinkNameId`
identity keeps the semantic helper before stale rendered text, while legacy raw
lhs text still scans for compatibility.

## Suggested Next

Supervisor should choose the next one-field collector migration still in scope
for 845. `LirBinOp.rhs` remains intentionally unchanged by this packet.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
`LirBinOp.rhs`, compare, select, aggregate/vector ops, inline asm, and residual
raw/global text on their current scanner paths. Do not reconstruct references
from rendered names or text.

## Proof

Completed proof for this packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Both commands passed. Proof log: `test_after.log`.
