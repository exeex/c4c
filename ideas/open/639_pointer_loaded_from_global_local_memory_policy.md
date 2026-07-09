# Pointer-Loaded-From-Global Local-Memory Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
Related:
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/open/621_rv64_prepared_global_value_location_consumer.md`
- `ideas/open/633_aggregate_stack_home_local_memory_policy.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: RV64 pointer-loaded-from-global local-memory policy
Queue Order: 39
Prerequisites: pointer value loaded from a global object must carry explicit
value identity, freshness, extent, offset, address space, and selected
local-memory use authority before RV64 consumes it as a local-memory address
Proof Surface: rows where the local-memory address is a pointer SSA value loaded
from a global object, not direct `addr @symbol`

## Goal

Define the policy and authority boundary for RV64 local-memory accesses whose
address comes from a pointer loaded out of a global object.

## Why This Exists

Idea 631 closed the direct `addr @symbol` local-memory route. Step 5
reclassified rows such as `src/pr46309.c`, `src/pr58984.c`, and
`src/pr66556.c` because the refreshed dumps show forms like
`bir.load_global ptr @q` followed by `bir.load_local ... addr %t15`, not a
direct global-symbol local-memory address.

## In Scope

- Refresh pointer-loaded-from-global rows and capture the producer, pointer
  value, global source object, freshness, offset, width, extent, address space,
  and local-memory use authority.
- Separate pointer-loaded-from-global local-memory from prepared global
  value-location rows, direct global-symbol rows, aggregate stack homes, and
  ordinary frame slots.
- Add producer or RV64 consumer support only when explicit authority proves
  the loaded pointer value can be used for the selected local-memory access.
- Preserve fail-closed diagnostics for missing pointer freshness, missing
  global source identity, incomplete extent or width facts, unsupported address
  space, or ambiguous memory-use authority.

## Out Of Scope

- Direct global-symbol local-memory support closed by idea `631`.
- Prepared global value-location consumption owned by idea `621`.
- Aggregate, sret, byval, or stack-home policy owned by idea `633`.
- String-constant local-memory support closed by idea `630`.
- ABI, runtime/library policy, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe proves a shared pointer-loaded-from-global local-memory
  family rather than a direct `addr @symbol` row.
- At least one complete-authority row moves past its current first owner, or
  all rows are reclassified to a more precise producer, aggregate, value
  location, or runtime owner.
- Negative proof keeps direct global-symbol rows, prepared value-location rows,
  aggregate homes, string constants, and missing-freshness pointer values out
  of this policy.

## Reviewer Reject Signals

- Reject treating `bir.load_global ptr @x` followed by local memory as direct
  `addr @x` local-memory support.
- Reject testcase-shaped handling for `src/pr46309.c`, `src/pr58984.c`, or
  `src/pr66556.c`.
- Reject RV64 inference of pointer freshness, extent, or global source identity
  from source filenames, final symbol names, or final assembly layout.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave
  pointer-loaded-from-global authority unconsumed.
