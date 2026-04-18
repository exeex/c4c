# LIR To BIR Memory Semantic Ownership Split

Status: Closed
Closed: 2026-04-18
Disposition: Completed; the semantic ownership split landed, and follow-on x86 route work continues under `ideas/open/56_x86_prepared_module_renderer_deheaderization.md` and `ideas/open/57_x86_backend_c_testsuite_capability_families.md`.

## Why This Was Closed

`src/backend/bir/lir_to_bir_memory.cpp` no longer needs to serve as a mixed
ownership monolith. The reopened route's goal was refactor-only: finish the
semantic split so the main memory TU becomes an opcode coordinator again,
without claiming new backend capability.

That goal is now satisfied.

## What Landed Before Closure

- the first ownership layer remained in place:
  - `src/backend/bir/lir_to_bir_memory_addressing.cpp`
  - `src/backend/bir/lir_to_bir_memory_provenance.cpp`
- the second ownership layer landed:
  - `src/backend/bir/lir_to_bir_memory_value_materialization.cpp`
  - `src/backend/bir/lir_to_bir_memory_local_slots.cpp`
- pointer-addressed load/store dispatch and global-provenance helpers moved out
  of the coordinator into provenance-owned helpers
- dynamic pointer/global value materialization moved into the dedicated
  value-materialization owner
- local-slot load/store and related mechanics moved into the dedicated
  local-slot owner
- `src/backend/bir/lir_to_bir_memory.cpp` is reduced to the main
  `lower_scalar_or_local_memory_inst` coordinator instead of acting as a broad
  fallback bucket

## Validation At Closure

Closure used a backend-scoped regression guard:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- regression guard passed
- before and after both reported `68` passed / `4` failed / `72` total
- the same four pre-existing backend failures remained:
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`
  - `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  - `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`

## Follow-On Context

- `ideas/open/56_x86_prepared_module_renderer_deheaderization.md` remains the
  separate refactor route for the x86 prepared-module renderer ownership split
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md` remains the
  umbrella capability-family route and should resume only through an explicit
  lifecycle activation, not by reopening this completed refactor idea
