Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Compatibility Sweep
Lifecycle Decision: Active plan remains blocked on x86-enabled proof; do not close from default-build proof alone.

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

Closure readiness in this default build context:

- Blocked for lifecycle close. The source idea acceptance criteria require
  proof for the focused x86 adapter path plus unchanged prepared lookup,
  route-debug, wrapper/fallback, helper/oracle, and `ConsumedPlans` behavior.
- The default build has `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`, and
  `ctest --test-dir build -N` does not register `backend_x86_route_debug` or
  `backend_x86_handoff_boundary`.
- Existing direct x86 rows in the gated tests cover the needed adapter and
  compatibility behavior, including agreed authority, nameless fallback,
  missing Route 6 facts, prepared selection preservation, and prepared/Route 6
  mismatch. Because those CTests are disabled here, the current proof cannot
  satisfy the direct x86 proof portion of the source acceptance criteria.

## Suggested Next

Supervisor should run or delegate an x86-enabled proof configuration for
`backend_x86_route_debug` and `backend_x86_handoff_boundary`, then decide
whether the plan can close or needs a plan-owner blocker record.

## Watchouts

- Step 3 was already wired in code; this packet does not add a new adapter or
  tests.
- Do not close this source idea from the current default-build proof alone; it
  lacks direct x86 route-debug / handoff-boundary CTest execution.
- Keep the `source_value_name` requirement as part of named authority; missing
  names are fallback rows, not partial successes.
- Do not treat prepared-only evidence or Route 6-only evidence as agreement.
- Riscv, ABI/register/stack/result policy, helper selection, wrapper
  instruction text, and fallback names remain out of scope.

## Proof

Command: `(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff)$' --output-on-failure) > test_after.log 2>&1`

Result: passed; the delegated enabled subset passed 3/3, but it does not remove
the closure blocker for disabled direct x86 proof rows.

Proof log: `test_after.log`.
