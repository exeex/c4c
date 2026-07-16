Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Completed plan Step 1-3 for the one-field `LirSelectOp.false_val` packet in
`collect_inst_refs`: replaced raw `S(op.false_val)` scanning with
`collect_operand_ref(op.false_val, refs)` only. Added nearby frontend HIR
coverage proving false_val semantic `LinkNameId` identity preempts stale
rendered text, while legacy raw false_val compatibility remains.

## Suggested Next

Supervisor should select the next packet or lifecycle action. Executor did not
repair lifecycle, choose a follow-on field, or widen this slice.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
aggregate/vector ops, inline asm, and residual raw/global text on their current
scanner paths. Do not reconstruct references from rendered names or text. This
packet touched only `LirSelectOp.false_val`.

## Proof

Required implementation proof:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

Both commands passed. Proof log: `test_after.log`.
