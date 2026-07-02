# RV64 Object Select And Edge Publication Helper Cleanup

## Goal

Extract RV64 object-route select-source, publication move, and predecessor-edge helper fragments after scalar and local/global helper APIs are stable.

## Why This Exists

Select and edge publication logic is highly coupled to prepared publication plans, source producer classification, scalar materialization, and local/global memory helpers. It needs its own cleanup contract so select movement is not hidden inside a scalar or dispatch refactor.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.hpp`
  - `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- Extract selected select-source, publication move, and predecessor-edge helper fragments.
- Keep prepared publication-plan and consumer-classifier dependencies explicit in APIs.
- Preserve diagnostic distinctions for selected source producer fragments.

## Out Of Scope

- Generic scalar movement, local/global memory movement, `fragment_for_prepared_instruction` fanout relocation, fallback behavior changes, or publication fact repair.
- Testcase-specific select fixes, gcc_torture expectation changes, unsupported marker changes, target-side inference, or RV64 capability repair.

## Acceptance Criteria

- Select and edge-publication helper ownership is explicit and does not hide broad dispatch coupling.
- Prepared publication facts, fallback behavior, and diagnostics are unchanged.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_prepared_edge_publication|dump_riscv64_.*select|codegen_route_riscv64_.*select|rv64_runtime_riscv64_pointer_typed_select_publication|rv64_runtime_riscv64_short_circuit_select_false_lhs)'`

## Reviewer Reject Signals

- The slice changes prepared publication facts, fallback behavior, or testcase expectations.
- The diff hides `fragment_for_prepared_instruction` fanout behind another monolithic dispatch file.
- The implementation overfits a known select testcase rather than preserving generic publication behavior.
- Unsupported markers or expected results are weakened without explicit approval.
- Helper renames are claimed as cleanup while old select/publication coupling remains.
