# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Split `backend_x86_handoff_boundary_test.cpp` Into Focused Translation Units
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed Step 5 by extracting the bounded compare-against-zero prepared
branch family out of `tests/backend/backend_x86_handoff_boundary_test.cpp`
into the focused
`tests/backend/backend_x86_handoff_boundary_compare_branch_test.cpp`
translation unit, wiring that runner into the existing
`backend_x86_handoff_boundary_test` executable via
`tests/backend/CMakeLists.txt`, and leaving the oversized handoff file to
own the remaining join, short-circuit, loop, and non-branch families.

## Suggested Next

Extract the next coherent Step 5 family, preferably the adjacent joined
compare-branch coverage, into its own focused translation unit so the current
split grows by semantic ownership instead of scattering one-off testcase
shards across files.

## Watchouts

- Keep Step 5 focused on test translation-unit ownership only. Do not mix the
  next packet with emitter changes, expectation rewrites, or test weakening.
- The new compare-branch file carries a small duplicated harness subset
  (`check_route_outputs`, minimal ABI-register helpers, and simple fixture
  builders). Prefer centralizing shared handoff helpers only when another
  family needs the same seam, rather than turning this packet into a broad
  harness rewrite after the fact.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 5 packet after splitting the compare-against-zero branch family
into its own translation unit; the proof passed and `test_after.log` is the
canonical proof log for the packet.
