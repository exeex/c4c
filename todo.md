Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Source Plan: plan.md

# Current Packet

## Just Finished

Plan Step 2: extracted the immediate local `memset`/`memcpy` helper family from
`src/backend/bir/lir_to_bir_memory.cpp` into
`src/backend/bir/lir_to_bir_memory_local_slots.cpp` as
`try_lower_immediate_local_memset` and `try_lower_immediate_local_memcpy`, so
the local-slot owner now handles array or aggregate view resolution, leaf-slot
collection, and scalar-slot copy or fill mechanics while the coordinator keeps
only the call-family admission and aliasing glue.

## Suggested Next

Continue `plan.md` Step 2 by re-reading the remaining coordinator-side
local-slot wrappers in `src/backend/bir/lir_to_bir_memory.cpp` and choosing one
coherent seam that still belongs with `lir_to_bir_memory_local_slots.cpp`
without pulling direct-call family admission or other coordinator-only glue out
of the main TU.

## Watchouts

- This plan is refactor-only; do not claim x86 backend capability progress from
  it.
- Keep `lower_scalar_or_local_memory_inst` in the main coordinator TU.
- `try_lower_immediate_local_memset` and
  `try_lower_immediate_local_memcpy` now sit alongside the other local-slot
  owners; the next packet should preserve that ownership boundary instead of
  re-embedding view resolution or leaf-copy logic back into the coordinator.
- The regression guard script currently exits non-zero on this subset because
  pass count stayed flat at `4/5` even though it reported `new failing tests:
  0`; treat the canonical log pair as unchanged-behavior evidence, not as a
  newly introduced regression.
- Keep future packets focused on the next honest local-slot-owned wrapper or
  helper family instead of pulling shared addressing helpers across ownership
  buckets or emptying `lir_to_bir_memory.cpp` for its own sake.
- Treat renderer de-headerization as separate idea `56`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$' | tee test_after.log`.
The build passed, and the narrow backend proof reproduced the same `4/5`
result recorded in `test_before.log`: the four backend notes/handoff/dynamic
member-array tests passed, and the pre-existing failure
`c_testsuite_x86_backend_src_00205_c` remained unchanged apart from timing and
ordering noise in the log. `test_after.log` remains the canonical proof log
for this packet.
