Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Emission-Only Boundary

# Current Packet

## Just Finished

Step 5 validation found an AArch64 c_testsuite regression introduced during
Step 3 of the current plan.

The Step 4 centralized fallback slice is still accepted on focused and backend
proof. Broader Step 5 c_testsuite validation fails on
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c`, but the same two tests also fail at
`HEAD~1` (`b919e53a2`, Step 3) in a temporary worktree. Reviewer follow-up
showed the same pair passes at `d298bbbc8` after Step 2, so the failing
c_testsuite pair regressed in Step 3 (`b919e53a2`) and is inside this active
emission-only route.

## Suggested Next

Repair the Step 3 AArch64 source-selection regression for
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c` without expectation weakening or
named-case shortcuts.

## Watchouts

- Keep source-selection authority in prepared call facts when
  `source_selection` is present.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.
- Step 5 is blocked by Step 3 AArch64 c_testsuite regressions:
  `c_testsuite_aarch64_backend_src_00216_c` segfaults and
  `c_testsuite_aarch64_backend_src_00204_c` reports a runtime output mismatch.
- A failed speculative local fix in
  `calls_argument_sources.cpp` was discarded after proving the same failures
  reproduce at `HEAD~1`; do not resurrect it without a fresh source-path
  explanation.
- Remaining temporary fallback: absent-selection local aggregate pointer
  frame-address publication still needs prepared
  `LocalFrameAddressMaterialization`/`FrameSlotAddress` source selection with
  frame slot id, byte offset, size, align, and materialization site.
- Remaining temporary fallback: absent-selection indirect byval lane handling
  still needs prepared `ByvalRegisterLane` source selection with source slot,
  stack offset, source size, align, and `byval_lane_extent_bytes`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_memory_operand_records|backend_aarch64_operand_resolution)$'`

Passed. Proof log: `test_after.log`.

Supervisor follow-up validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 162/162.

Step 5 validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_'`
  failed 2/220 at `HEAD` (`529c57977`): `c_testsuite_aarch64_backend_src_00216_c`
  and `c_testsuite_aarch64_backend_src_00204_c`.
- Temporary-worktree comparison at `HEAD~1` (`b919e53a2`) with
  `ctest --test-dir /tmp/c4c-head1/build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$'`
  failed the same two tests, showing the blocker predates Step 4.
- Reviewer route check in
  `review/aarch64_emission_step5_blocker_route_review.md` found the same pair
  passes at `d298bbbc8` after Step 2, so the blocker is a Step 3 regression and
  should be repaired inside this active plan.
