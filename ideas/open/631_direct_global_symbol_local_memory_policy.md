# Direct Global-Symbol Local-Memory Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
Related:
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/open/621_rv64_prepared_global_value_location_consumer.md`
- `ideas/closed/608_prepared_global_data_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: RV64 direct global-symbol local-memory policy
Queue Order: 31
Prerequisites: direct global-symbol base-plus-offset authority must identify
global identity, offset, width, extent, and supported addressing before RV64
consumes the local-memory access
Proof Surface: local-memory rows whose selected base is a direct
`global_symbol` rather than a prepared global value-location sequence

## Goal

Define the RV64 consumer policy for direct global-symbol base-plus-offset
local-memory accesses, keeping it distinct from prepared global value-location
handling in idea `621`.

## Why This Exists

Idea 614's close-readiness classification found `global_symbol`
local-memory residuals. Existing idea `621` covers prepared global
value-location consumption and explicitly excludes direct global-symbol
base-plus-offset authority. These rows need a separate policy route unless a
fresh diagnostic refresh proves they now match idea `621`.

## In Scope

- Refresh direct global-symbol local-memory rows and decide whether they truly
  differ from idea `621` prepared value-location rows.
- Consume explicit direct global-symbol identity, base-plus-offset, extent,
  width, relocation/addressing mode, and memory-use authority when complete.
- Preserve fail-closed diagnostics for missing global identity, unsupported
  relocation/addressing, incomplete extent/width, or ambiguous object-data
  facts.
- Add focused tests separating direct global-symbol local-memory access from
  prepared global value-location access.

## Out Of Scope

- Prepared global value-location handling owned by idea `621`.
- Prepared/global producer authority owned by closed idea `608` unless fresh
  evidence requires a new producer split.
- String-constant local-memory policy covered by idea `630`.
- Local frame-slot access already closed by idea `614`.
- ABI, runtime/library policy, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe proves whether the residual rows belong here or can be
  folded into idea `621` without broadening its source intent.
- At least one direct global-symbol local-memory row moves past the current
  owner or is reclassified to a precise prepared/global producer owner.
- Negative proof keeps prepared value-location rows, string constants,
  unsupported widths, aggregate homes, and unrelated local-memory rows outside
  this idea.

## Reviewer Reject Signals

- Reject silently widening idea `621` semantics without a refreshed diagnostic
  basis.
- Reject testcase-shaped handling for specific globals, source files, or final
  symbol names.
- Reject RV64 inference of global identity, offset, extent, or addressing mode
  from source filenames or final assembly layout instead of explicit facts.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave direct
  global-symbol local-memory authority unconsumed.
