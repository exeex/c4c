# RV64 Pointer Local-Memory Consumption

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/597_pointer_address_semantic_model_research.md`
- `ideas/closed/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/RV64 authority and RV64 consumer boundary
Queue Order: 13
Prerequisites: BIR local-memory GEP/address semantics must exist for producer-owned rows; selected pointer base+offset and pointer-value authority from ideas `599` and `600` must be present
Estimated Evidence Breadth: `27` local memory frame-slot or pointer base+offset rows
Proof Surface: rows with selected pointer/local-memory authority that still fail at RV64-side consumption

## Goal

Wire selected pointer base+offset and pointer-value memory-use authority into
the RV64 local-memory consumer for rows whose upstream authority already
exists.

## Why This Exists

The current scan has `27` rows close to the recent pointer/address architecture
work. They should be repaired as target consumption of selected authority, not
as fresh pointer semantics.

## Closure Summary

Closed after Step 4 residual split/close-readiness classification. The route
landed bounded RV64 consumer support for explicit prepared frame-slot
local-memory accesses with supported scalar/floating widths, while keeping
missing frame-slot ids, missing base-plus-offset authority, non-default address
spaces, and unsupported 16-byte widths fail-closed.

The final residual refresh found no additional selected pointer/local-memory
consumer family with complete authority and meaningful breadth. The remaining
`unsupported_local_memory_access` rows need policy decisions or producer
authority before RV64 object emission can consume them semantically.

Durable residual work was split instead of folded into this closed consumer
route:

- `ideas/open/630_string_constant_local_memory_policy.md` covers local-memory
  rows whose selected base is a string constant.
- `ideas/open/631_direct_global_symbol_local_memory_policy.md` covers direct
  global-symbol base-plus-offset local-memory policy distinct from prepared
  global value-location consumption in idea `621`.
- `ideas/open/632_f128_and_16_byte_local_memory_width_policy.md` covers
  unsupported 16-byte/F128 local-memory width policy.
- `ideas/open/633_aggregate_stack_home_local_memory_policy.md` covers
  sret/byval or aggregate pointer stack-home local-memory policy.
- `ideas/open/634_large_selected_pointer_offset_local_memory_policy.md`
  covers large selected pointer-offset materialization policy.

Do not reopen this idea merely because residual rows still carry
`unsupported_local_memory_access`. Reopen only if refreshed diagnostics show a
new broad row family already has complete selected local-memory authority and
is rejected solely at the same RV64 consumer boundary.

## In Scope

- RV64 local-memory consumption of selected pointer base+offset authority.
- Guardrails that keep BIR address production and direct pointer arithmetic
  research out of this route.
- Same-diagnostic proof across multiple pointer/local-memory rows.

## Out Of Scope

- BIR GEP/address producer repair.
- Direct `unsupported_pointer_arithmetic` policy.
- Branch stack-source, select publication, ABI, runtime, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple pointer/local-memory consumption rows progress when selected
  authority exists.
- Rows missing selected authority remain rejected before RV64 consumption.
- The proof cites or exercises the authority boundary from ideas `599` and
  `600`.

## Reviewer Reject Signals

- Reject RV64 guessing of pointer base+offset facts.
- Reject merging BIR pointer/address production into this consumer idea.
- Reject named-case-only frame-slot fixes.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes.
- Reject retaining the same unsupported local-memory frame-slot diagnostic
  behind renamed code.
