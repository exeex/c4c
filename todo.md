Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Agreement Boundary and Fail-Closed Matrix

# Current Packet

## Just Finished

Step 2, Agreement Boundary and Fail-Closed Matrix, is complete.

Accepted boundary: add a narrow x86-local agreement facade at the local-slot
statement/compare-load memory consumer path in
`src/backend/mir/x86/module/module.cpp`, around
`render_prepared_local_slot_statement_memory_operand(...)` and
`find_prepared_local_slot_compare_load(...)`. No existing x86 helper already
forms the required Route 3/prepared join: the current path checks prepared
`result_value_name` / `stored_value_name` before rendering a frame-slot operand
but does not call `mir::find_bir_memory_access_identity(...)`,
`mir::find_bir_same_block_load_local_source_identity(...)`, or compare Route 3
identity with `PreparedEdgePublication::source_memory_access`. The facade
should be x86-local and should return agreement evidence to the caller before
the prepared memory row is treated as a semantic mirror; it must not rename
`PreparedFunctionLookups::memory_accesses` into route authority.

Agreement predicates for the selected row:
- same function: the facade is called with the active BIR function and prepared
  function/addressing rows for the same `FunctionNameId`;
- same block: the selected BIR block and prepared `BlockLabelId` match the
  block label used for the prepared memory access and Route 3 request;
- selected `LoadLocal`: the selected BIR instruction is a `LoadLocalInst` at
  the candidate instruction index, and the Route 3/MIR identity reports
  `node_kind = LoadLocal`;
- source-memory identity: Route 3 reports the selected result value name/type,
  local-slot base identity, address space, volatility, byte offset, size, and
  alignment for that load;
- prepared row completeness: `PreparedEdgePublication::source_memory_access`
  is `Available`, points at a non-null `PreparedMemoryAccess`, has a complete
  frame-slot base, nonzero size/alignment, and a result value matching the
  selected `LoadLocal`;
- prepared/Route 3 row match: prepared and Route 3 agree on result value,
  local-slot/frame-slot identity, address space, volatility, byte offset, size,
  and alignment before x86 may reuse the prepared frame-slot operand as the
  selected source-memory mirror.

Fail-closed matrix:
- Missing: no BIR block, block label, Route 3 record, prepared addressing row,
  or prepared publication row means no agreement; x86 keeps current fallback or
  rejection behavior.
- Incomplete: null `source_memory_access`, unavailable/missing/incomplete
  source-memory status, missing frame slot, incomplete base, zero size, zero
  alignment, negative unsupported offset, or unusable base-plus-offset means no
  agreement.
- Duplicate/conflict: duplicate prepared result-name rows, ambiguous prepared
  memory lookup answers, or multiple/conflicting Route 3 candidates for the
  selected `LoadLocal` must reject rather than choose one arbitrarily.
- Prepared-only: prepared `memory_accesses` or `source_memory_access` without a
  matching Route 3 `LoadLocal` record remains compatibility evidence only, not
  semantic agreement.
- Route-only: a Route 3 `LoadLocal` record without a complete prepared
  `source_memory_access` row does not authorize x86 prepared operand reuse.
- Unsupported: non-local-slot bases, unsupported type/size combinations,
  unsupported address materialization, and target-policy-sensitive addressing
  rows stay outside this bridge and follow existing x86 fallback/rejection.
- Fallback: existing fallback names, helper/oracle names, status strings,
  prepared printer rows, and public prepared lookup/status surfaces remain
  observable and unchanged.
- Mismatch: disagreement on function, block, instruction index, result value,
  base kind, slot identity, address space, volatility, offset, size, or
  alignment rejects; no output baseline rewrite or status weakening counts as
  progress.
- Policy-sensitive: x86 frame placement, operand spelling, register
  materialization, emitted-output formatting, ABI behavior, and final storage
  policy remain target-owned and are not imported into Route 3/BIR.

Proof row split:
`backend_prepared_lookup_helper` and `backend_prepared_printer` cover default
prepared lookup/status and printer observability rows, including Route 3
`LoadLocal` identity, prepared `source_memory_access` status, and stable public
prepared `memory_accesses` surfaces. The x86 agreement facade, accepted local
slot path, direct x86 missing/incomplete/mismatch/duplicate/conflict rows, and
byte-stability for x86 output require x86-enabled validation because
`backend_x86_handoff_boundary` and `backend_x86_route_debug` are registered
only when `C4C_ENABLE_X86_BACKEND_TESTS` is on.

## Suggested Next

Execute Step 3: implement, or prove already implemented, the narrow x86
agreement facade around the local-slot prepared memory consumer path. The
implementation should gate only the selected Route 3 `LoadLocal`
source-memory mirror and leave public prepared lookup/status surfaces
unchanged.

## Watchouts

- Do not implement generic memory parity; this bridge is only for the selected
  x86 Route 3 `LoadLocal` source-memory fact.
- Keep `PreparedFunctionLookups::memory_accesses`,
  `find_prepared_memory_access(...)`, prepared source-memory status rows,
  prepared printer output, route-debug strings, and fallback/helper names
  observable and unchanged.
- Step 3 should reject prepared-only and route-only rows instead of using either
  side as unilateral authority.
- Use x86-enabled validation for direct x86 rows; default prepared
  lookup/printer tests are not closure-grade proof for the x86 facade.

## Proof

Ran `git diff --check -- todo.md`: pass. This delegated analysis-only proof
does not create `test_after.log`.
