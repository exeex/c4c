# LIR CFG Terminator Block Identity Completion

Status: Open
Type: bounded LIR CFG carrier repair
Predecessor: `ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md`

## Goal

Remove semantic dependence on raw label strings for active LIR terminator
successors. Direct branch, conditional branch, switch default/case targets,
and computed-goto target lists must carry current-function `LirBlockId`
authority, with label text retained only as display.

## Why This Exists

The direct-branch route already began moving from label strings to block IDs,
but nearby terminators still keep successors as raw strings. The user-label
goto regression showed that display labels can look correct while the
structured target relation is wrong or missing. Leaving the family half
converted makes every later LIR receiver vulnerable to accidental string
recovery.

## In Scope

- Complete current-function `LirBlockId` successor authority for active
  `LirCondBr` and `LirSwitch` terminators.
- Convert active `LirIndirectBrOp.targets` or provide an equivalent structured
  target carrier for computed-goto blocks.
- Preserve label strings as printer/debug mirrors only.
- Make the verifier reject invalid, missing, duplicate/ambiguous, or
  cross-function block authority before printing or downstream consumption.
- Add focused coverage for forward labels, conditional edges, switch labels,
  and computed-goto target lists.

## Out Of Scope

- PHI incoming predecessor/value authority; idea 751 owns that.
- Raw-BIR import, canonical BIR, target lowering, MIR, or emission changes.
- Recovering block identity from label spelling, rendered LLVM, or testcase
  names.

## Acceptance Criteria

- Every active terminator successor that controls CFG flow has current-function
  block identity authority.
- Existing display labels may be misleading without changing verifier outcome.
- Focused positive and negative tests prove direct, conditional, switch, and
  indirect target behavior without weakening existing verifier contracts.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.
