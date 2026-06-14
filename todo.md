Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow Lookup-Reader Bridge

# Current Packet

## Just Finished

Completed Step 3: Implement Narrow Lookup-Reader Bridge for the idea 260
`module` lookup-reader packet.

Files changed:

- Added `src/backend/prealloc/lookup_agreement.hpp` and
  `src/backend/prealloc/lookup_agreement.cpp`.
- Updated `src/backend/prealloc/prepared_lookups.cpp` to use the shared
  function/block agreement helper before the existing raw-label compatibility
  block fallback.
- Updated `src/backend/prealloc/select_chain_lookups.cpp` to use the shared
  function/block-label helper and skip source-producer publication when a
  structured block label is stale or conflicted.
- Added focused rows in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Implemented agreement boundary:

- Function agreement now requires exactly one matching BIR function spelling.
  Duplicate same-name BIR functions fail closed instead of selecting the first
  row.
- Block agreement now requires a unique structured BIR `label_id` that spells
  the prepared block label through the module or retained prepared-name
  compatibility table and maps back to the same prepared label id.
- Raw `block.label` fallback remains compatibility-only: it is only used when
  there is no structured id; stale or conflicting structured ids return
  `kInvalidBlockLabel` and do not publish select-chain producer rows.
- Public prepared aggregate compatibility remains intact through
  `make_prepared_function_lookups(...)`.

Focused rows proved in the helper test:

- positive structured function/block agreement with raw label spelling drift
  ignored in favor of matching ids
- use by both `make_prepared_function_lookups(...)` and
  `make_prepared_edge_publication_source_producer_lookups(...)`
- duplicate same-name BIR function rejection
- stale structured BIR label rejection
- duplicate structured block rejection
- raw-label-only fallback as compatibility-only behavior
- prepared/BIR label drift rejection for the requested prepared block

## Suggested Next

Execute Step 4 from `plan.md`: add any remaining fail-closed proof rows and
decide whether the focused Step 3 rows are sufficient for this selected
candidate.

Remaining Step 4 proof gaps to consider:

- direct rows for invalid prepared function ids and invalid prepared block ids
- empty prepared function/block spelling rows if a compact fixture can express
  them without mutating unrelated name-table internals
- absent BIR function/module structure and missing prepared-name lookup rows
- whether raw-label fallback needs an additional public-surface regression row
  beyond the helper-level compatibility check

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
- The shared helper still allows legacy fixtures whose BIR `label_id` resolves
  through the prepared name table instead of the module name table; unknown
  ids remain conflicts, not raw-label fallback.
- Step 4 should not broaden into another idea 260 candidate. Keep proof rows
  limited to the selected `module` lookup-reader helper path.
- Keep the public prepared aggregate and existing prepared lookup maps
  observable. This packet is not authorization to delete fields, privatize the
  aggregate, wrap the module, or migrate another `module`, `names`,
  `control_flow`, or `store_source_publications` candidate.

## Proof

Initial focused check before full delegated proof:

```bash
cmake --build --preset default --target backend_prepared_lookup_helper_test -j2 && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure
```

Result: passed.

Full delegated proof:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` shows 3/3 default prepared/RISC-V tests and
2/2 x86 route-debug/handoff tests passed.
