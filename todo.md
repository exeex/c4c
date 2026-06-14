Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Compatibility Sweep
Lifecycle Decision: x86-enabled proof is satisfied; supervisor should request plan-owner closure decision.

# Current Packet

## Just Finished

Step 5 - Compatibility Sweep re-checked the public compatibility surfaces named
by Steps 1-4 for the selected x86 scalar named-`i32` Route 6 / prepared call
argument source identity adapter. No implementation code, tests, expectations,
`plan.md`, or source idea files were changed.

Compatibility surfaces swept:

- Public prepared call-plan APIs remain present and observable:
  `PreparedFunctionLookups::call_plans`, `PreparedBirModule::call_plans`,
  `find_prepared_call_plans(...)`, `find_consumed_call_plan(...)`, and
  `find_consumed_call_argument_plan(...)`.
- `ConsumedPlans` still carries prepared call plans plus optional
  `route6_call_use_sources`; the x86 adapter remains local to
  `find_consumed_scalar_i32_call_argument_source_authority(...)`.
- The adapter still requires a prepared call argument plan, Route 6 call-use
  source index, named `i32` argument, `ArgumentValue` match, matching prepared
  and Route 6 `source_value_id`, and a present Route 6 `source_value_name`
  before it returns named authority.
- Prepared-only, Route 6-only, missing source-name, mismatched source-id, and
  fallback rows remain fail-closed by helper contract while preserving prepared
  call argument selection.
- Route-debug status strings remain the existing rows:
  `gate=agreed`, `gate=missing_source_value`, `gate=missing_source_name`,
  `gate=prepared_source_mismatch`, `gate=source_value_mismatch`, and
  `gate=blocked`.
- Wrapper/fallback behavior and fallback names/output are not edited; the
  enabled stdarg semantic/prepared handoff tests are the available default-build
  compatibility proof.
- Riscv remains non-applicable: no riscv call-plan consumer for this x86-local
  `ConsumedPlans` adapter was found, and this packet makes no riscv parity
  claim.

Closure readiness:

- Ready for plan-owner closure decision. The supervisor provided an
  x86-enabled proof configuration with `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=ON`;
  `ctest --test-dir build-x86 -N` registers both direct x86 rows:
  `backend_x86_route_debug` and `backend_x86_handoff_boundary`.
- The x86-enabled proof command completed successfully:
  `(cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_before.log 2>&1 && cp test_before.log test_after.log`.
- Result: `backend_x86_route_debug` passed,
  `backend_x86_handoff_boundary` passed, 2/2 tests passed.
- Regression guard accepted the matching before/after x86 proof with
  `--allow-non-decreasing-passed`: before 2/2, after 2/2.
- The default build still has `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`, and
  `ctest --test-dir build -N` does not register `backend_x86_route_debug` or
  `backend_x86_handoff_boundary`; that is now historical context only, not a
  current lifecycle blocker.
- Existing direct x86 rows in the gated tests cover the needed adapter and
  compatibility behavior, including agreed authority, nameless fallback,
  missing Route 6 facts, prepared selection preservation, and prepared/Route 6
  mismatch.

## Suggested Next

Supervisor should request plan-owner lifecycle closure review for idea 257.

## Watchouts

- Step 3 was already wired in code; this packet does not add a new adapter or
  tests.
- Do not treat the default-build x86 CTest registration gap as a current
  blocker; the required direct x86 rows were proved in `build-x86`.
- Keep the `source_value_name` requirement as part of named authority; missing
  names are fallback rows, not partial successes.
- Do not treat prepared-only evidence or Route 6-only evidence as agreement.
- Riscv, ABI/register/stack/result policy, helper selection, wrapper
  instruction text, and fallback names remain out of scope.

## Proof

Command: `(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff)$' --output-on-failure) > test_after.log 2>&1`

Result: passed; the delegated enabled subset passed 3/3, but it does not remove
the need for direct x86 proof rows.

X86-enabled command: `(cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_before.log 2>&1 && cp test_before.log test_after.log`

X86-enabled result: passed; `backend_x86_route_debug` and
`backend_x86_handoff_boundary` both passed, 2/2.

Regression guard: `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Regression guard result: passed; before 2/2 and after 2/2.

Proof log: `test_after.log`.
