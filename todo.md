# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Prepared-Module Or Call-Bundle Seam
Plan Review Counter: 2 / 4
# Current Packet

## Just Finished

Plan Step 2.1 repaired the next idea-61 authoritative prepared short-circuit
consumption seam in `src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_param_zero_render.cpp`,
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`, and
`src/backend/mir/x86/codegen/x86_codegen.hpp`. The x86 prepared-module
consumer now accepts the generic looped short-circuit entry lane where the
authoritative branch condition is an `i8` named-vs-immediate compare and the
rhs continuation reaches the join through a plain branch block.

I added focused boundary coverage in
`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp` for a
minimal local-slot `i8` short-circuit fixture. That boundary test now proves
the old authoritative prepared short-circuit handoff rejection is gone and the
route graduates into the later scalar branch-family restriction instead of
dying at the idea-61 seam.

The reduced reproducer `/tmp/c4c_match_repro.c` now moves past the old
authoritative short-circuit handoff rejection and fails later with the same
downstream scalar restriction that full `00204` now reaches:
`x86 backend emitter only supports a minimal single-block i32 return
terminator, a bounded equality-against-immediate guard family with immediate
return leaves including fixed-offset same-module global i32 loads and
pointer-backed same-module global roots, or one bounded compare-against-zero
branch family through the canonical prepared-module handoff`.

## Suggested Next

Rehome the new full-`00204` blocker under the downstream scalar leaf and take
the next packet there. Idea 61 no longer owns the old authoritative
short-circuit handoff for this route; the remaining failure is the later
minimal-return / compare-against-zero scalar branch-family restriction.

## Watchouts

- The new `i8` short-circuit lane is only proven to graduate into the later
  scalar branch-family restriction; it is not yet a fully supported emitted
  route.
- `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp` now
  protects two things for this seam: the old authoritative short-circuit
  rejection is gone, and the route lands on the expected downstream scalar
  restriction instead of drifting somewhere else.
- The current full-`00204` failure no longer points at idea-61-owned
  short-circuit consumption. Do not reopen this packet unless the route falls
  back to the old authoritative handoff rejection again.

## Proof

Ran the delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'; } > test_after.log 2>&1`.
Current proof is mixed in the final workspace state:
`backend_x86_handoff_boundary` passes, while
`c_testsuite_x86_backend_src_00204_c` still fails. The old authoritative
prepared short-circuit handoff rejection is gone; the new failing diagnostic is
the later scalar restriction:
`x86 backend emitter only supports a minimal single-block i32 return
terminator, a bounded equality-against-immediate guard family with immediate
return leaves including fixed-offset same-module global i32 loads and
pointer-backed same-module global roots, or one bounded compare-against-zero
branch family through the canonical prepared-module handoff`.
The canonical proof log remains `test_after.log`. Additional reducer proof for
this packet:

- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu /tmp/c4c_match_repro.c`
  now reaches the same downstream scalar restriction instead of failing on the
  old authoritative short-circuit handoff.

Together, those results show this packet repaired one real idea-61
short-circuit consumption seam and moved full `00204` to a later blocker.
