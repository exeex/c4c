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
