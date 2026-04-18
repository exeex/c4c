Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 3 ("Implement One Honest Capability Lane") for a
broader scalar-array dynamic-`gep` packet by extending the backend-owned lane
from local struct-member arrays to global aggregate bases as well. The backend
notes witnesses for both dynamic global scalar-member access and dynamic global
member-array indexing now lower through semantic BIR without `gep local-memory`
failure notes, and `00205` no longer fails in the `gep` family.

## Suggested Next

Do not keep widening this `gep` packet. Re-baseline the first external-family
selection now that `00205` has moved past semantic `gep` lowering and instead
stops in the x86 emitter's minimal control-flow / prepared-module boundary.
Either switch the first externally coherent family to that scalar-control-flow
or minimal-handoff blocker, or choose a different external family whose narrow
packet is already visible and bounded.

## Watchouts

- Do not reopen idea `55`; the memory ownership split is closed.
- Treat idea `56` as a separate initiative; the current Step 1 baseline still
  does not show renderer de-headerization as the immediate blocker.
- Do not weaken `x86_backend` expectations or pretend this packet already
  proves the first external family; `00205` still fails, but now outside the
  `gep` family.
- `00205` currently stops in the x86 emitter's minimal route contract: the
  remaining blocker is looped control flow plus runtime-heavy prepared-module
  support, not scalar-array dynamic-`gep` lowering.
- The new backend-owned witnesses live in `backend_lir_to_bir_notes_test.cpp`;
  they prove both dynamic global scalar-member access and dynamic global
  member-array indexing through semantic BIR without needing testcase-shaped
  recognizers.
- `backend_x86_handoff_boundary_test.cpp` still passes unchanged. That is a
  non-regression check only; do not claim new emitter-family support from this
  packet.
- Because the external blocker moved to a different family, this slice is
  honest backend capability progress but not yet the first accepted external
  capability-family packet.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir|backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir|c_testsuite_x86_backend_src_00205_c)$" |& tee test_after.log'`

Result: `backend_lir_to_bir_notes`, `backend_x86_handoff_boundary`, and the
local dynamic-member-array backend route tests pass. `00205` still fails, but
its failure moved from `gep local-memory semantic family` to the x86 emitter's
minimal control-flow / prepared-module boundary. `test_after.log` should
capture that exact subset and remaining blocker.
