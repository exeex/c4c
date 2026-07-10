Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` repaired the new first focused failure after commit
`b7c665902`: `expected prepared pointer-value F64 local RV64 object module to
build`. The supported fixture had a positioned pointer-value
`PreparedMemoryAccess` for the named F64 `StoreLocalInst`, but the access did
not carry `stored_value_name`; the normal local-store access lookup dropped
that selected access before `prepared_local_memory_emit.cpp` could validate and
emit the pointer-value F64 path.

`fragment_for_prepared_store_local()` now preserves a positioned pointer-value
access with no `stored_value_name` for named stores when the caller did not pass
a selected access. Malformed selected pointer-value facts still fail closed in
the prepared local-memory emitter instead of falling back to BIR local-slot
offsets, while valid F64 pointer-value facts reach the existing `fsd`/`fld`
encoder. No tests, expectations, unsupported markers, allowlists, timeout or
runtime policy, baseline accounting, `plan.md`, or source idea files were
edited.

Evidence:
`build/agent_state/664_step4_pointer_value_f64_local/summary.md`,
`build/agent_state/664_step4_pointer_value_f64_local/before_after.diff`,
`build/agent_state/664_step4_pointer_value_f64_local/code.diff`,
`build/agent_state/664_step4_pointer_value_f64_local/regression_guard.txt`,
and `build/agent_state/664_step4_pointer_value_f64_local/test_after.log`.

## Suggested Next

Suggested next packet: address the new first visible focused failure,
`expected prepared sret stack-homed pointer store to build`. Start in the
prepared local-memory pointer-value/sret stack-homed store path and determine
why the supported pointer-store fixture no longer emits an object.

## Watchouts

- The pointer-value F64 failure was repaired by preserving the selected
  pointer-value access, not by relaxing local-slot fallback. Selected malformed
  prepared local-memory facts should still fail closed.
- Remaining focused failures visible in `test_after.log` are sret stack-homed
  pointer store, sret stack-homed I8 extent-6 store, and prepared local frame
  address publication before register/stack call-argument consumption.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, baseline accounting, `plan.md`, or the source idea in a routine
  executor packet.

## Proof

`cmake --build --preset default --target backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still fails on
`backend_riscv_object_emission`. `test_after.log` is the canonical proof log.

Aggregate-visible delta: `expected prepared pointer-value F64 local RV64
object module to build` was removed from `test_after.log`. The focused
aggregate still has one failing CTest row, and the first visible failure is now
`expected prepared sret stack-homed pointer store to build`.

Regression guard command:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: failed because passed count did not strictly increase
(`passed=0 failed=1 total=1` before and after); no new failing tests were
reported.
