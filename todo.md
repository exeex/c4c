Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Lookup-Reader Contract

# Current Packet

## Just Finished

Completed Step 1: Inventory Lookup-Reader Contract for the idea 260 `module`
lookup-reader packet.

Definitions and declarations:

- `src/backend/prealloc/prepared_lookups.hpp` publicly declares
  `PreparedFunctionLookups` and `make_prepared_function_lookups(...)`; the
  selected helper row remains file-local today.
- `src/backend/prealloc/prepared_lookups.cpp` defines file-local
  `prepared_bir_function(...)`, `prepared_bir_block(...)`, and
  `prepared_bir_block_label_id(...)`, then builds `PreparedFunctionLookups`
  from prepared call plans, addressing, memory accesses, move bundles, value
  homes, edge publications, and edge-publication source producers.
- `src/backend/prealloc/select_chain_lookups.cpp` has a second file-local
  `prepared_bir_function(...)` plus `prepared_bir_block_label_id(...)` pair
  used by `make_prepared_edge_publication_source_producer_lookups(...)`.

Direct callers and construction surface:

- `publish_prepared_after_call_result_lane_bindings(...)` calls
  `prepared_bir_function(...)` and `prepared_bir_block(...)`; null function,
  null block, or null value-locations currently skips after-call lane binding
  publication.
- `collect_block_entry_republication_effects(...)` calls
  `prepared_bir_function(...)` and `prepared_bir_block(...)`; null function or
  block currently skips preservation-republication effect discovery for that
  block.
- `make_prepared_edge_publication_source_producer_lookups(...)` in
  `select_chain_lookups.cpp` calls its local `prepared_bir_function(...)` and
  `prepared_bir_block_label_id(...)`; null function currently returns empty
  source-producer lookups, and invalid/unresolved block labels are published as
  `kInvalidBlockLabel` rather than recovered from raw strings.
- `make_prepared_move_bundle_lookups(prepared, function)`,
  `make_prepared_call_plan_lookups(...)`,
  `make_prepared_edge_publication_lookups(prepared, ...)`, and
  `make_prepared_function_lookups(...)` are the prepared lookup construction
  path that exposes these results to backends and helper tests.

Current behavior to preserve:

- `prepared_bir_function(...)` returns null for
  `kInvalidFunctionName`, empty prepared function spelling, or no BIR function
  with matching display name.
- `prepared_bir_block(...)` returns null for `kInvalidBlockLabel`, empty
  prepared block spelling, or no matching BIR block. It currently accepts a
  BIR block when either structured `label_id` spells the same prepared label or
  the raw BIR `label` string matches.
- `prepared_bir_block_label_id(...)` prefers a non-invalid BIR
  `label_id` only when that id spells a non-empty name that exists in the
  prepared block-label table; otherwise it falls back to finding the raw BIR
  `block.label` in the prepared names.
- Stale-label and prepared/BIR drift behavior is compatibility-oriented today:
  stale structured ids may still resolve through raw label fallback, while
  missing/invalid names or unresolved ids keep null, empty lookup, or
  `kInvalidBlockLabel` behavior.
- Duplicate/conflicting prepared value-home and memory-access rows are already
  fail-closed in the lookup maps; the module packet should not weaken those
  policies while changing the function/block discovery layer.

Candidate implementation and test files:

- Implementation candidates:
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/select_chain_lookups.cpp`, and possibly
  `src/backend/prealloc/prepared_lookups.hpp` only if Step 2 chooses a shared
  helper declaration instead of duplicate file-local helpers.
- Primary focused tests:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Nearby stability tests for Step 3/4 proof:
  `tests/backend/bir/backend_prepared_printer_test.cpp`,
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, and the
  x86 route/debug handoff bucket if the selected helper affects prepared
  function lookups consumed by x86.

## Suggested Next

Execute Step 2 from `plan.md`: design the exact agreement boundary for the
`module` lookup-reader packet. Recommended design packet should decide whether
to share one helper between `prepared_lookups.cpp` and
`select_chain_lookups.cpp` or update each file-local helper, then record the
accepted rows and fail-closed rows before code changes.

Recommended narrow proof for Step 2/3:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

## Watchouts

- Keep this runbook limited to the selected `module` lookup-reader candidate.
- Do not treat the activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve existing null, fallback, `kInvalidBlockLabel`, stale-label, and
  prepared/BIR drift behavior.
- Do not rewrite printer/debug, route-debug, target output, baselines, or
  unsupported expectations to claim progress.
- The current block helper has raw-label fallback after structured label-id
  miss; Step 2 must explicitly decide which rows may use the new BIR structure
  fact and which rows must retain null or `kInvalidBlockLabel` results.
- The selected row has duplicate helper logic in `prepared_lookups.cpp` and
  `select_chain_lookups.cpp`; avoid fixing one path while leaving the other
  with a contradictory agreement policy.

## Proof

Ran:

```bash
git diff --check -- todo.md
```

Result: passed. This packet is inventory-only; no build/test proof was needed
because no implementation or tests changed.
