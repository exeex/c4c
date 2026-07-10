Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` repaired the new first focused failure after commit
`4c7e45784`: `expected prepared RV64 object path to reject`, mapped to
`rejects_prepared_scalar_local_subobject_fail_closed_shapes()`. The first
subcase clears the selected store access
`address.frame_slot_id`; `fragment_for_prepared_store_local()` rejected that
malformed prepared frame-slot access but then fell through to the broader BIR
slot-offset fallback and still published text.

The prepared local-memory store fragment path now only uses the BIR local-slot
fallback when no selected prepared memory access exists. If prepared
local-memory facts are selected for an instruction, malformed frame-slot or
pointer facts must fail closed instead of being rescued by the broader slot
path. No tests, expectations, unsupported markers, allowlists, timeout or
runtime policy, baseline accounting, `plan.md`, or source idea files were
edited.

Evidence:
`build/agent_state/664_step4_prepared_local_fail_closed/summary.md`,
`build/agent_state/664_step4_prepared_local_fail_closed/before_after.diff`,
and `build/agent_state/664_step4_prepared_local_fail_closed/regression_guard.txt`.

## Suggested Next

Suggested next packet: address the new first focused failure,
`expected prepared pointer-value F64 local RV64 object module to build`. Start
in the prepared local-memory F64 pointer-value path and determine why the
supported pointer-value F64 fixture no longer emits an object.

## Watchouts

- The current slice deliberately does not allow a selected but malformed
  prepared local-memory access to fall back to BIR local-slot offsets. That is
  the intended fail-closed behavior for malformed prepared facts.
- Remaining focused failures include pointer-value F64 local, sret stack-homed
  stores, call-argument publication ordering, string/direct-global diagnostics,
  and one remaining generic rejection.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, baseline accounting, `plan.md`, or the source idea in a routine
  executor packet.

## Proof

`cmake --build --preset default --target backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still fails on
`backend_riscv_object_emission`. `test_after.log` is the canonical proof log.

Aggregate-visible delta: `expected prepared RV64 object path to reject` was
removed from `test_after.log`. The focused aggregate still has one failing
CTest row, and the first visible failure is now
`expected prepared pointer-value F64 local RV64 object module to build`.

Regression guard command:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: failed because passed count did not strictly increase
(`passed=0 failed=1 total=1` before and after); no new failing tests were
reported.
