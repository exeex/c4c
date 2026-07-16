Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Carrier-Backed Collector Seam

# Current Packet

## Just Finished

Lifecycle repair after accepted `LirSelectOp.true_val` migration. Closure of
845 is rejected because durable in-scope collector migration remains.

## Suggested Next

Execute plan Steps 1-3 for the one-field `LirSelectOp.false_val` packet in
`collect_inst_refs`: replace raw `S(op.false_val)` scanning with
`collect_operand_ref(op.false_val, refs)` only if inspection confirms the
existing semantic carrier remains ready.

## Watchouts

Do not close 845 yet. Do not perform a broad collector sweep. Leave
aggregate/vector ops, inline asm, and residual raw/global text on their current
scanner paths. Do not reconstruct references from rendered names or text.

## Proof

Required implementation proof:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
