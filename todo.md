Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` repaired the new first focused failure after commit
`a02cc6e25`: `expected prepared sret stack-homed pointer store to build`.
`fragment_for_prepared_store_local()` treated a selected scalar pointer-value
store as generic pointer-value memory before checking the specialized sret
stack-home authority. For valid sret stack-homed stores, the pointer base lives
in the hidden sret stack home, so the generic register/ordinary stack-home
materialization failed and the object build returned `std::nullopt`.

The selected pointer-value scalar store path now first recognizes
`prepared_sret_stack_slot_pointer_access()`. Valid sret stack-homed stores load
the hidden sret pointer home and store the named source through the return
pointee, while malformed selected sret facts still fail closed through the
existing unsupported local-memory diagnostic. No tests, expectations,
unsupported markers, allowlists, timeout or runtime policy, baseline
accounting, `plan.md`, source idea files, or `review/reviewA.md` were edited.

Evidence:
`build/agent_state/664_step4_sret_stack_pointer_store/summary.md`,
`build/agent_state/664_step4_sret_stack_pointer_store/before_after.diff`,
`build/agent_state/664_step4_sret_stack_pointer_store/code.diff`,
`build/agent_state/664_step4_sret_stack_pointer_store/regression_guard.txt`,
and `build/agent_state/664_step4_sret_stack_pointer_store/test_after.log`.

## Suggested Next

Suggested next packet: address the remaining first visible focused failure,
`expected prepared local frame addresses to be published before register and
stack call-argument consumption`. Start at the prepared local frame-address
publication ordering before register and stack call-argument consumption.

## Watchouts

- The sret repair is semantic: selected pointer-value scalar stores check
  valid sret stack-home authority before generic pointer-value materialization.
  Do not relax the malformed selected local-memory fail-closed checks.
- The focused proof also removed the visible sret stack-homed I8 extent-6
  store failure. The remaining aggregate-visible blocker is prepared local
  frame-address publication before register/stack call-argument consumption.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, baseline accounting, `plan.md`, or the source idea in a routine
  executor packet.

## Proof

`cmake --build --preset default --target backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still fails on
`backend_riscv_object_emission`. `test_after.log` is the canonical proof log.

Aggregate-visible delta: `expected prepared sret stack-homed pointer store to
build` and `expected prepared sret stack-homed I8 extent-6 store to build` were
removed from `test_after.log`. The focused aggregate still has one failing
CTest row, and the first visible failure is now `expected prepared local frame
addresses to be published before register and stack call-argument consumption`.

Regression guard command:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: failed because passed count did not strictly increase
(`passed=0 failed=1 total=1` before and after); no new failing tests were
reported.
