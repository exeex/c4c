# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Split `backend_x86_handoff_boundary_test.cpp` Into Focused Translation Units
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed the next Step 5 split by extracting the local i32
prepared-branch guard-contract family out of
`tests/backend/backend_x86_handoff_boundary_test.cpp` into the focused
`tests/backend/backend_x86_handoff_boundary_local_i32_guard_test.cpp`
translation unit, wiring that runner into the existing
`backend_x86_handoff_boundary_test` executable via
`tests/backend/CMakeLists.txt`, and leaving the monolithic handoff file to
own the remaining non-local-i32 guard, multi-defined, and LIR families.

## Suggested Next

Extract the next coherent Step 5 family from the remaining monolithic
handoff file, preferably the residual guard-chain or local-i16 guard group,
so the translation unit keeps shrinking by semantic ownership instead of by
ad hoc testcase shards.

## Watchouts

- Keep Step 5 focused on test translation-unit ownership only. Do not mix the
  next packet with emitter changes, expectation rewrites, or test weakening.
- The extracted compare-branch, joined-branch, short-circuit, and loop files
  now each carry a small duplicated harness subset (`check_route_outputs`,
  targeted control-flow helpers, and local fixture builders). Prefer
  centralizing shared handoff helpers only when another family genuinely
  needs the seam, rather than turning Step 5 into a broad harness rewrite.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 5 packet after splitting the local i32 prepared-branch guard
family into its own translation unit; the proof passed and `test_after.log`
is the canonical proof log for the packet.
