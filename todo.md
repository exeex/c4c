Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` classified and repaired the remaining first diagnostic
exactness row after commit `f0cc218db`: `unsupported_local_memory_access` for
`byval_stack_param` rejected correctly but returned the canonical base
diagnostic with contextual suffix fields
`; function=byval_stack_param; block=entry; block_index=0;
instruction_index=0; access_base=pointer_value`.

Owner decision: this belongs to implementation diagnostic surface policy in
`src/backend/mir/riscv/codegen/object_emission.cpp`, not to plan-owner or
reviewer expectation churn. Both scalar and F64 local-memory address-shape
rejection paths now return the canonical base
`unsupported_local_memory_access` diagnostic while preserving the fail-closed
category. No tests, expectations, unsupported markers, allowlists, timeout or
runtime policy, baseline accounting, `plan.md`, or source idea files were
edited.

Evidence:
`build/agent_state/664_step4_local_memory_diagnostic/summary.md` and
`build/agent_state/664_step4_local_memory_diagnostic/gdb_first_postfix_fail.txt`.

## Suggested Next

Suggested next packet: address the new first focused failure after diagnostic
normalization, `expected prepared RV64 object path to reject`, mapped by gdb to
`rejects_prepared_scalar_local_subobject_fail_closed_shapes()`. Start with the
first subcase, where the prepared scalar local subobject fixture removes
`address.frame_slot_id` but the RV64 object route still builds instead of
failing closed with the local-memory diagnostic.

## Watchouts

- Optimized backtraces may fold identical helper wrappers; the post-fix gdb
  backtrace names the failing test function
  `rejects_prepared_scalar_local_subobject_fail_closed_shapes()`, and the
  source order points to the first mutation that clears `address.frame_slot_id`.
- Later focused failures remain, including local-memory diagnostic exactness,
  pointer-value F64 local, sret stack-homed stores, call-argument publication
  ordering, string/direct-global diagnostics, and one remaining generic
  rejection.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, baseline accounting, `plan.md`, or the source idea in a routine
  executor packet.

## Proof

`cmake --build --preset default --target backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still fails on
`backend_riscv_object_emission`. `test_after.log` is the canonical proof log.

Aggregate-visible delta: all
`unsupported_local_memory_access` diagnostic exactness mismatch lines in
`test_before.log` were removed from `test_after.log`. The focused aggregate
still has one failing CTest row, and the first visible failure is now
`expected prepared RV64 object path to reject`.

Regression guard command:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: failed because passed count did not strictly increase
(`passed=0 failed=1 total=1` before and after); no new failing tests were
reported.
