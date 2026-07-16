Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed the eighth one-field collector migration for plan Steps 1-3:
`LirBinOp.rhs` in `collect_inst_refs` now uses
`collect_operand_ref(op.rhs, refs)`.

Added focused dead-internal reachability coverage proving rhs `LinkNameId`
identity keeps the semantic helper before stale rendered text, while legacy raw
rhs text still scans for compatibility.

## Suggested Next

Supervisor should decide whether the repaired runbook has another bounded
one-field collector migration remaining or should enter the lifecycle
close/blocker loop.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave compare,
select, aggregate/vector ops, inline asm, and residual raw/global text on their
current scanner paths. Do not reconstruct references from rendered names or
text. Preserve legacy raw compatibility for `LirBinOp.rhs` if the semantic
carrier is absent or non-authoritative.

## Proof

Proof run for this packet:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Both passed. Log path: `test_after.log`.
