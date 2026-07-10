Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` repaired two prepared local-memory/object-emission
fail-open paths:

- selected prepared `PointerValue` local-memory facts now stay on the
  pointer/byval validation path instead of falling through to
  incoming-stack-formal load emission, and the stack-homed pointer-base
  fallback requires prepared pointer-value local-memory authority before
  loading a pointer home
- fused pointer branch operands that are same-block named pointer temporaries
  now require explicit prepared source materialization facts instead of falling
  back to an older stack home when the materialization is missing or malformed

Evidence in `build/agent_state/664_step4_byval_pointer_probe/summary.md` shows
the aggregate generic rejection count decreases from 2 to 1 and the first
pre-repair generic owner was
`rejects_prepared_fused_pointer_rhs_materialized_source_fail_closed_shapes()`.

## Suggested Next

Suggested next packet: address the remaining first failure, now a byval
diagnostic exactness mismatch where the object route rejects correctly but
returns contextual
`unsupported_local_memory_access...; function=byval_stack_param; ...;
access_base=pointer_value` instead of the exact base diagnostic expected by the
focused test. Keep the packet scoped to diagnostic surface policy unless the
supervisor routes this to plan-owner/reviewer because expectation edits are out
of executor scope.

## Watchouts

- Optimized backtraces fold helper names; precise untracked `-O0 -g` evidence
  lives under `build/agent_state/664_step4_precise_first_generic/`.
- The first remaining focused failure after this repair is not generic; it is
  the byval diagnostic exactness mismatch recorded in
  `build/agent_state/664_step4_precise_first_generic/gdb_first_fail_O0_after_object_patch.txt`.
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

Aggregate-visible delta: `expected prepared RV64 object path to reject`
decreased from 2 occurrences in `test_before.log` to 1 occurrence in
`test_after.log`; the first generic line from `test_before.log` was removed.
Precise debug evidence:
`build/agent_state/664_step4_precise_first_generic/gdb_first_fail_O0.txt` and
`build/agent_state/664_step4_precise_first_generic/gdb_first_fail_O0_after_object_patch.txt`.

Regression guard command:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: failed because passed count did not strictly increase
(`passed=0 failed=1 total=1` before and after); no new failing tests were
reported.
