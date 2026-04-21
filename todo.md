# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Scalar Prepared/Emitter Seam
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 2.1 ran a direct prepared-module probe for the reduced `match` route and
corrected the previous ownership theory. The shared prepared pipeline already
publishes the key addressing and home facts that this packet was chasing:
`match` reaches x86 with pointer-backed prepared accesses for `%p.s`, `%p.f`,
and `%t1`, plus concrete prepared value homes for `%t22`, `%t24`, and `%t34`.
That means the current idea-60 blocker is no longer a producer-side
`stack_layout.cpp` gap. The remaining failure sits in x86 structural dispatch,
which still rejects this multi-block two-parameter pointer-backed loop/return
route even though the prepared handoff already carries the needed pointer
addressing metadata.

## Suggested Next

Send the next idea-60 packet back to the x86 consumer surface, centered on
`prepared_module_emit.cpp` and the adjacent local-slot/structural dispatch
helpers. Use the confirmed prepared facts from `match` as the contract: the
route already has pointer-backed accesses and prepared homes, so the next work
should inspect why x86 still rejects the multi-block two-parameter loop/return
shape instead of consuming the existing prepared addressing and value-home
data.

## Watchouts

- Do not reopen the `stack_layout.cpp` publication theory without new evidence.
  The probe showed `match` already has prepared addressing records at the
  relevant sites, including a pointer-backed `%p.s` store and prepared homes
  for `%t22`, `%t24`, and `%t34`.
- Do not revive x86-local pointer-binary matcher growth. The current failure is
  not "missing pointer address metadata"; it is that the existing structural
  dispatch families still do not admit this multi-block two-parameter loop.
- Keep the blocker in idea 60 unless the next x86 dispatch inspection shows the
  route actually falls back before scalar emission into a different leaf.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the idea-60 scalar
restriction. Supporting investigation used a local probe that lowered
`00204.c` through prepare and inspected the prepared `match` function directly:
the route already publishes pointer-backed prepared accesses plus prepared
homes for `%t22`, `%t24`, and `%t34`. Canonical proof log: `test_after.log`.
