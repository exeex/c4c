Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Fail-Closed Proof Rows

# Current Packet

## Just Finished

Completed Step 4: Add Fail-Closed Proof Rows for the idea 260 `module`
lookup-reader packet.

Added focused proof rows in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for:

- invalid prepared function ids returning no BIR agreement and no source
  producer rows
- absent BIR function/module structure returning no BIR agreement and no source
  producer rows
- invalid prepared block ids returning no block agreement
- public source-producer and prepared aggregate raw-label fallback regression
  coverage for legacy BIR blocks with no structured label id
- missing prepared-name lookup rows for structured BIR labels failing closed
  instead of falling back to raw label spelling

Existing Step 3 rows still cover the positive structured agreement path,
duplicate same-name BIR functions, stale structured BIR labels, duplicate
structured blocks, helper-level raw-label fallback, and prepared/BIR label
drift.

## Suggested Next

Execute Step 5 from `plan.md`: run broader validation and decide closure
readiness for the selected `module` lookup-reader candidate.

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
- Empty prepared function/block spelling is represented by invalid ids in this
  fixture. Forcing a separate empty-spelling row would require mutating
  name-table internals and stays out of scope for this packet.

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
