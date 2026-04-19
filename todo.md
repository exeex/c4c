# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 5.2
Current Step Title: Split Residual Local-Slot Guard Lane
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed the next Step 5 split by extracting the minimal scalar smoke lane
covering the constant return, parameter passthrough, and immediate arithmetic
routes out of `tests/backend/backend_x86_handoff_boundary_test.cpp` into the
focused `tests/backend/backend_x86_handoff_boundary_scalar_smoke_test.cpp`
translation unit, wiring that runner into the existing
`backend_x86_handoff_boundary_test` executable via `tests/backend/CMakeLists.txt`,
and deleting the moved scalar-smoke route checks plus their lane-specific
fixture helpers from the monolithic handoff file.

## Suggested Next

Execute Step 5.2 by extracting the remaining local add/sub/address guard lane
out of `tests/backend/backend_x86_handoff_boundary_test.cpp` into a focused
translation unit, keeping the helper seam bounded and leaving the monolithic
file with only intentionally central ownership.

## Watchouts

- Keep Step 5 focused on test translation-unit ownership only. Do not mix the
  next packet with emitter changes, expectation rewrites, or test weakening.
- The extracted compare-branch, joined-branch, short-circuit, and loop files
  now each carry a small duplicated harness subset (`check_route_outputs`,
  targeted control-flow helpers, and local fixture builders); the new
  multi-defined call, rejection, and scalar-smoke files follow that same
  bounded pattern. Prefer centralizing shared handoff helpers only when
  another family genuinely needs the seam, rather than turning Step 5 into a
  broad harness rewrite.
- The monolithic handoff file now mostly carries the base helper harness plus
  the remaining local add/sub/address guard lane, so any further Step 5 work
  should keep those seams explicit instead of repacking previously split
  families.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 5 packet after splitting the minimal scalar smoke lane into its
own translation unit; the proof passed and `test_after.log` is the canonical
proof log for the packet.
