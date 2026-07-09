# Aggregate Global-Object Materialization Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
Related:
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/open/633_aggregate_stack_home_local_memory_policy.md`
- `ideas/open/634_large_selected_pointer_offset_local_memory_policy.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: aggregate global-object materialization and byte-storage policy
Queue Order: 41
Prerequisites: aggregate global object identity, lane, byte range, offset,
extent, selected destination, and memory-use authority must be explicit before
RV64 materializes the access
Proof Surface: rows whose local-memory residual is aggregate or byte-storage
global-object materialization rather than direct scalar `addr @symbol`

## Goal

Define the producer and consumer policy for aggregate global-object
materialization into local or selected memory destinations.

## Why This Exists

Idea 631 Step 5 kept rows such as `src/complex-7.c`, `src/pr49073.c`,
`src/pr60017.c`, `src/pr60822.c`, and `src/pr88739.c` out of direct
global-symbol local-memory support. Their refreshed diagnostics point at
aggregate global-object materialization, byte-storage lanes, local aggregate
traffic, or very large aggregate offsets.

## In Scope

- Refresh aggregate/global-object materialization rows and capture global
  object identity, aggregate lane, byte range, selected offset, width, extent,
  destination, and memory-use authority.
- Separate aggregate global-object materialization from aggregate stack-home
  policy, large selected pointer-offset materialization, direct global-symbol
  local memory, and ordinary scalar global memory.
- Add producer or RV64 consumer support only for a shared aggregate/byte-lane
  authority shape proven by current diagnostics.
- Preserve fail-closed diagnostics for missing lane identity, incomplete byte
  ranges, unsupported offsets, ambiguous destination authority, or mismatched
  aggregate extent.

## Out Of Scope

- Direct global-symbol local-memory support closed by idea `631`.
- Aggregate/sret/byval stack-home local-memory policy owned by idea `633`.
- Large selected pointer-offset policy owned by idea `634`.
- Prepared global value-location consumption owned by idea `621`.
- ABI, runtime/library policy, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe identifies a shared aggregate global-object
  materialization family or splits the rows into precise existing owners.
- At least one complete-authority aggregate materialization row moves past its
  current owner, or the route records the exact missing producer or target
  policy that blocks it.
- Negative proof keeps scalar direct global-symbol rows, stack-home aggregate
  rows, large-offset rows owned by idea `634`, and runtime-only failures
  outside this policy.

## Reviewer Reject Signals

- Reject treating aggregate global-object materialization as scalar direct
  `addr @symbol` local-memory support.
- Reject testcase-shaped handling for the named Step 5 rows or specific
  aggregate offsets.
- Reject RV64 inference of aggregate lanes, byte ranges, or destination
  authority from final assembly layout or source object spelling.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave aggregate
  materialization authority missing.
