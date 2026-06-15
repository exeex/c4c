# 266 Phase F4 edge_publications unsupported publication fail-closed proof map

## Goal

Map the remaining unsupported publication fail-closed proof obligations for
`PreparedFunctionLookups::edge_publications` after the Phase F3 x86 Route 5
prepared edge-publication agreement bridge.

This is analysis/proof work only. It must not demote, delete, privatize, or
wrap the edge-publication lookup table.

## Why This Exists

The post-F3 gate found `edge_publications` thinner than after idea 247 because
x86 now has selected dynamic `LoadLocal` Route 5/prepared edge-publication
agreement with Route 3 agreement gating. The row is still blocked because
unsupported publication surfaces need explicit fail-closed proof.

## In Scope

- Map duplicate Route 5 records on one natural edge.
- Map prepared-only publication rows.
- Map Route 5-only publication rows.
- Map wrong-edge publication rows.
- Identify the Route 5/BIR publication authority and any retained public
  prepared compatibility mirror.
- Name expected fail-closed behavior and the x86/riscv evidence or explicit
  non-applicability required for each row.

## Out Of Scope

- Edge-publication lookup demotion, deletion, privatization, accessor wrapping,
  or adapter migration.
- Broad `PreparedFunctionLookups` or prepared aggregate retirement.
- Moving target publication/output policy, edge layout policy, branch/layout
  policy, formatting, wrapper behavior, instruction spelling, or exact target
  output into BIR.
- Weakening unsupported behavior, helper/oracle statuses, fallback behavior, or
  route-debug/printer/wrapper output.

## Completion Criteria

- A proof map exists for duplicate same-edge Route 5 records, prepared-only
  rows, Route 5-only rows, and wrong-edge rows.
- Each row names the semantic authority, consumer boundary, compatibility
  surface, and expected fail-closed result.
- The map distinguishes positive selected-path evidence from unsupported-row
  proof gaps.
- No implementation demotion is proposed unless a later idea can point to this
  map as closed evidence.

## Closure Disposition

Closed after the active runbook completed the bounded proof map for all four
publication families:

- Duplicate same-edge Route 5 rows are mapped as fail-closed/diagnostic
  obligations. Current helper and diagnostic evidence is not same-consumer
  target proof, so these rows remain blocked from demotion.
- Prepared-only rows are mapped as retained compatibility state plus missing
  Route 5 authority. Current helper/oracle, selected x86, and riscv diagnostic
  proof does not prove every x86/riscv target consumer rejects absent Route 5
  authority while preserving public lookup/status/output behavior.
- Route 5-only rows are mapped as authority-present and mirror-missing. Same
  consumer proof is still absent for consuming Route 5 authority while the
  public prepared mirror remains observable and compatible.
- Wrong-edge rows are mapped as natural-edge identity mismatches. Existing
  route/helper/oracle, diagnostic, and selected-path evidence does not directly
  prove target consumers reject wrong-predecessor or wrong-successor rows
  without emitting from the wrong natural edge.

Final policy:

- `PreparedFunctionLookups::edge_publications` remains public retained
  compatibility.
- No `edge_publications` demotion, deletion, privatization, private accessor,
  wrapper, adapter migration, target-policy migration into BIR, status-output
  rewrite, expectation rewrite, helper/oracle relabeling, fallback rename, or
  route-debug/printer/wrapper-output rewrite is safe from this map.
- No implementation follow-up should be created from this proof map. A later
  proof-only initiative may be opened separately if the project chooses to
  pursue same-consumer x86/riscv target proof for one blocked family.

Close-time regression handling used the existing canonical full-suite logs:
`test_before.log` and `test_after.log` both reported 3428/3428 passing tests
for the same full-suite CTest scope. The regression guard passed with
`--allow-non-decreasing-passed`, which is appropriate for this lifecycle-only
closure with no implementation changes.

## Reviewer Reject Signals

- The slice weakens unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- The slice claims progress by rewriting expected strings, renaming helpers,
  relabeling statuses, changing route-debug/printer output, or reclassifying
  rows without proving fail-closed behavior.
- The slice hides old public `edge_publications` authority behind a renamed BIR
  field, route field, private accessor, adapter, wrapper, or compatibility
  helper without proving it is only a mirror.
- The slice migrates target-owned edge layout, branch/layout, publication,
  output, formatting, emission, instruction spelling, wrapper, ABI, register,
  stack, or storage policy into BIR.
- The slice adds testcase-shaped shortcuts for one duplicate or wrong-edge
  fixture while leaving adjacent unsupported publication rows unexamined.
- The slice authorizes broad prepared retirement, `PreparedFunctionLookups`
  retirement, or edge-publication lookup demotion.
