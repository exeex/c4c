Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` repaired the remaining focused
`backend_riscv_object_emission` failure:
`expected prepared local frame addresses to be published before register and
stack call-argument consumption`.

Fixture/boundary:
`builds_prepared_local_frame_address_register_source_arg_call_object()` in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` expected prepared
local frame-address call arguments to publish into their prepared source
registers before those registers are consumed into ABI destinations. The
RISC-V object emitter already did this for stack-destination consumption, but
the register-destination path in `fragment_for_prepared_call()` emitted a
local frame address directly into the ABI destination register. That skipped
the prepared source-register publication and the later source-to-ABI move.

`fragment_for_prepared_call()` now publishes a GPR register-destination
`LocalFrameAddressMaterialization` source into the prepared source register
when one is present, then moves that source register into the ABI destination
if the registers differ. The existing prepared frame-slot address helper still
validates the route, so malformed selected local-memory facts remain
fail-closed. Valid pointer/sret stores from prior slices were not changed.

No tests, expectations, unsupported markers, allowlists, timeout/runtime
policy, baseline accounting, `plan.md`, source idea files, or
`review/reviewA.md` were edited.

Evidence:
`build/agent_state/664_step4_local_frame_address_publication/summary.md`,
`build/agent_state/664_step4_local_frame_address_publication/before_after.diff`,
`build/agent_state/664_step4_local_frame_address_publication/code.diff`,
`build/agent_state/664_step4_local_frame_address_publication/regression_guard.txt`,
and
`build/agent_state/664_step4_local_frame_address_publication/test_after.log`.

## Suggested Next

Suggested next packet: supervisor/plan-owner lifecycle review for idea 664.
The focused `backend_riscv_object_emission` row now passes against the
rolled-forward Step 4 baseline, so the next action is to decide whether idea
664 can close or needs broader supervisor-selected validation first.

## Watchouts

- Rows 139 and 176 were already failing in the temporary pre-sret worktree per
  supervisor status. This packet only touched RISC-V prepared object-emission
  local frame-address source-register publication; no evidence links those rows
  to this repair.
- Keep the prior malformed selected local-memory fail-closed behavior and valid
  pointer/sret stores intact when reviewing or broadening validation.
- Do not rewrite expectations, unsupported markers, allowlists,
  timeout/runtime policy, baseline accounting, `plan.md`, or the source idea in
  a routine executor packet.

## Proof

`cmake --build --preset default --target backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest passes. `test_after.log` is the canonical
proof log.

Aggregate-visible delta: focused row changed from
`backend_riscv_object_emission` failing with `expected prepared local frame
addresses to be published before register and stack call-argument consumption`
to `backend_riscv_object_emission` passing.

Regression guard command:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: passed (`passed=0 failed=1 total=1` before,
`passed=1 failed=0 total=1` after); resolved
`backend_riscv_object_emission`; no new failing tests were reported.
