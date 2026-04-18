Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: extracted the remaining coordinator-side dynamic local-aggregate
GEP projection lookup from `src/backend/bir/lir_to_bir_memory.cpp` into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` as
`try_lower_dynamic_local_aggregate_gep_projection`, so the coordinator now only
dispatches that family while local-slot ownership handles the dynamic local
aggregate map lookup and pointer-array projection result.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining local dynamic
pointer-array and local-slot helper branches in
`src/backend/bir/lir_to_bir_memory.cpp`, then choose the next coherent
coordinator-owned local-slot mechanics wrapper rather than widening into
addressing or provenance helpers.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `build_dynamic_local_aggregate_array_access` now owns repeated-extent
  packaging, and `resolve_dynamic_local_aggregate_gep_projection` now owns the
  per-element slot projection, while
  `try_lower_dynamic_local_aggregate_store` and
  `try_lower_dynamic_local_aggregate_load` now own the dynamic local aggregate
  store/load dispatch mechanics, and
  `try_lower_dynamic_local_aggregate_gep_projection` now owns the remaining GEP
  map lookup and dispatch for that family; keep future packets focused on the
  next honest local-slot-owned wrapper instead of pulling shared addressing
  helpers across ownership buckets.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- The next packet should still be chosen by the next coherent
  coordinator-owned local dynamic aggregate helper family, not by line counts
  or by emptying `lir_to_bir_memory.cpp` for its own sake.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof reproduced the pre-existing
single failure `c_testsuite_x86_backend_src_00205_c` with no new failing tests
versus `test_before.log`. The canonical regression check
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
reported `delta : passed=0 failed=0` and `new failing tests: 0`, but still
returned failure because it currently requires the passed count to increase
strictly. `test_after.log` remains the canonical proof log for this packet.
