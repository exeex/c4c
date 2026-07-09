# Direct Global-Symbol Local-Memory Policy

Status: Closed
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

## Closure Notes

Closed on 2026-07-09 after Step 5 reclassified the refreshed direct
global-symbol row set.

The accepted route added focused prepared-layer coverage for explicit scalar
direct `PreparedAddressBaseKind::GlobalSymbol` local-memory facts, then added
narrow RV64 scalar load/store admission for prepared direct-global local-memory
accesses. The RV64 consumer now requires explicit global-symbol authority,
direct policy, default address space, non-volatile access, scalar layout
authority, width/alignment agreement, symbol identity, base-plus-offset facts,
and signed-12-bit offset encodability.

Step 5 reran the focused allowlist from Step 1 with:

`cmake --build --preset default && ALLOWLIST=build/agent_state/631_step1_global_symbol.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/631_step5_global_symbol.log 2>&1`

The probe still reported `total=18 passed=0 failed=18`, but no remaining row
proved another direct `addr @symbol` local-memory consumer gap for this idea.
Residual owners were classified as prepared move-bundle authority,
pointer-loaded-from-global local-memory, aggregate/global-object
materialization, byval/struct-return aggregate copies, mixed local/global
publication, large aggregate offsets, or runtime mismatch.

Existing open ideas already cover several residual owners:

- `ideas/open/633_aggregate_stack_home_local_memory_policy.md`
- `ideas/open/634_large_selected_pointer_offset_local_memory_policy.md`
- `ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md`
- `ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md`

Follow-up split ideas created for residual owners not already represented by a
specific open idea:

- `ideas/open/639_pointer_loaded_from_global_local_memory_policy.md`
- `ideas/open/640_mixed_local_global_publication_authority.md`
- `ideas/open/641_aggregate_global_object_materialization_policy.md`
- `ideas/open/642_rv64_global_residual_runtime_mismatch_research.md`

The close gate used matching focused backend CTest logs:
`test_before.log` and `test_after.log` both ran
`ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`.
The c4c regression guard passed in non-decreasing mode with 1/1 tests passing
before and after and no new failures.

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
