Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Inventory And Regression Check

# Current Packet

## Just Finished

Completed `plan.md` Step 6 final inventory and broad backend regression check.

Acceptance coverage recorded from Steps 2-5:
- Step 2 repaired the main declaration/signature aggregate ABI route by
  threading structured layout authority into BIR declaration lowering while
  keeping legacy `type_decls` as fallback/parity evidence.
- Step 3 coverage added or preserved focused aggregate storage/addressing
  proofs for padded local dynamic member arrays, nested pointer member arrays,
  packed structs, global aggregate storage paths, and local aggregate member
  offsets.
- Step 4 recorded aggregate call ABI and variadic coverage status. The covered
  call paths include sret/byval aggregate ABI lowering and variadic aggregate
  argument behavior through the same aggregate ABI route; remaining legacy-only
  call-family helper surfaces are listed below instead of being treated as
  removed.
- Step 5 added `backend_cli_dump_bir_layout_sensitive_aggregate`, a focused
  `--dump-bir` guard for layout-sensitive BIR facts: callee
  `sret(size=8, align=4)` and `byval(size=8, align=4)` ABI annotations, caller
  aggregate ABI call text, and `+4` aggregate copy offsets for byval input and
  sret output.

Remaining fallback-only legacy layout users for later printer or legacy-removal
ideas:
- Named `%struct` lookup remains parity-gated. `BackendStructuredLayoutTable`
  facts are accepted for named structs only when legacy and structured layouts
  are both present, parity is checked, and parity matches; otherwise lookup
  falls back to legacy layout.
- No-table helper overloads remain legacy-only:
  `lower_byval_aggregate_layout(text, type_decls)`,
  `parse_global_address_initializer(text, type_decls)`,
  `lower_aggregate_initializer(... type_decls ...)`, local
  scalar-offset/array-slot helpers without structured layouts, and
  repeated-extent/addressing helper overloads without structured layouts.
- Declaration/signature paths that were fallback-only in the Step 1 inventory
  were not all left in that state: the main declaration ABI lowering route was
  repaired in Step 2. Any generic declaration/signature helper path that still
  passes an empty or legacy-only `TypeDeclMap` and no structured table remains
  fallback-only evidence for later cleanup.
- Legacy `type_decls` intentionally remains in the model as fallback and
  parity evidence; this runbook did not remove legacy parsing helpers or make
  the BIR printer parse/query layout authority.

## Suggested Next

Hand back to the supervisor for the lifecycle decision on the active runbook.
No further executor implementation packet is suggested from this Step 6
inventory-only slice.

## Watchouts

- Keep the remaining fallback-only helper inventory visible for later printer
  or legacy-removal ideas.
- Do not interpret the Step 2 declaration ABI repair as removal of every
  legacy-only generic helper overload.
- `type_decls` remains required as fallback/parity evidence until a later
  lifecycle explicitly replaces or removes those surfaces.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. Build completed and CTest reported 106/106 enabled
`^backend_` tests passed; 12 disabled MIR-focused backend tests were not run.
Proof log: `test_after.log`.
