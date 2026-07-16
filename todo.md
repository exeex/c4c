Status: Active
Source Idea Path: ideas/open/843_lir_direct_hir_family_construction_array_composition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected runtime-text construction escape hatches

# Current Packet

## Just Finished

Completed plan.md Step 4 as a bounded no-code retirement check for the selected
indexed local-array GEP producer.

No additional selected runtime-text escape hatch can be safely deleted in this
route. The supported static local-array element path now bypasses
`hir_rendered_indexed_gep_element_type_text` via recursive `LirTypeRef::array`
facts, and Step 3 proved stale compatibility text is not reparsed for that
selected path. The remaining `hir_rendered_indexed_gep_element_type_text`
fallback is still required for unsupported indexed GEP element shapes and
unmigrated named consumers.

## Suggested Next

Route active idea 843 to plan-owner for lifecycle disposition. The bounded
indexed local-array GEP producer slice has delivered direct recursive array
family construction and proof; broader `emit_rval_*`, `coerce`, global/extern,
collector, Raw-BIR, and terminal deletion work remains outside this selected
route.

## Watchouts

Keep this route to producer construction and recursive array composition.
Global/extern mirrors, collector scans, Raw-BIR receiver packets, and terminal
compatibility deletion belong to later ordered successors unless an exact
selected producer proves named-consumer parity.

The code intentionally duplicates a small recursive array-ref helper in the
local object authority and indexed GEP producer surfaces. A later cleanup can
centralize it only if that does not widen this route into global/extern,
collector, or Raw-BIR ownership.

Do not delete the runtime-text fallback for unsupported indexed GEP shapes
inside this idea. Keep global array GEP identity and local-array backend
publication rows out of any closure claim unless plan-owner records them as
separate successor scope.

## Proof

Step 4 proof command: `git diff --check`

Result: passed.

No new `test_after.log` was written because this was a no-code retirement
check. The existing `test_after.log` remains from Step 3 and shows
`frontend_lir_call_type_ref` plus `frontend_lir_extern_decl_type_ref`, 2/2
passing.
