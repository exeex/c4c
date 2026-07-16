Status: Active
Source Idea Path: ideas/open/845_lir_typed_reference_carriers_collector_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Parity And Fail-Closed Behavior

# Current Packet

## Just Finished

Completed the fifth 845 migration packet for `LirPhiIncoming.value`.
`collect_inst_refs` now routes PHI incoming values through `collect_operand_ref`,
so `LirOperand::global` semantic `LinkNameId` identity preempts stale rendered
text. Raw PHI incoming values still scan text for compatibility.

## Suggested Next

Ask plan-owner whether to close 845, repair with another one-field migration,
or route a successor. Remaining scanner fields include GEP indices, inline asm,
and residual raw/global text; this packet did not claim them.

## Watchouts

Do not perform a broad collector sweep. Do not reconstruct references from
rendered names or text. Do not delete any scanner path before the exact source
field has semantic carrier proof and nearby parity coverage. For this packet,
loads, GEPs, PHIs, inline asm, and residual raw/global text remain outside
scope.

## Proof

Passed:

```
{ cmake --build build && ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```
