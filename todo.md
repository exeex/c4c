Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement x86 Agreement Path

# Current Packet

## Just Finished

Step 3 - Implement x86 Agreement Path verified the selected x86 scalar
named-`i32` call argument source identity path is already routed through the
accepted authority helper. No code churn was needed.

Implementation evidence:

- `src/backend/mir/x86/x86.hpp` already implements
  `find_consumed_scalar_i32_call_argument_source_authority(...)` as the
  agreement gate.
- The helper first requires a prepared call argument plan from
  `find_consumed_call_argument_plan(...)`, a Route 6 call-use/source index from
  `ConsumedPlans::shared_route6_call_use_source_index()`, and a named
  `bir::TypeKind::I32` BIR argument.
- The helper then calls `route6_find_call_argument_source(...)` and accepts
  only rows that pass
  `route6_call_argument_source_matches_argument_value_record(...)`.
- It returns `std::nullopt` unless Route 6 and prepared `source_value_id` are
  both present and equal.
- It also returns `std::nullopt` when Route 6 `source_value_name` is absent,
  preserving the named-source authority requirement.
- The direct-extern prepared handoff path in
  `src/backend/mir/x86/module/module.cpp` routes each call argument through
  `find_consumed_scalar_i32_call_argument_source_authority(...)` before passing
  an optional source name to `append_prepared_direct_extern_call_argument(...)`.
- The broader prepared module lowering path also gates named `i32` arguments
  through the same helper before using an optional source name; non-`i32` and
  unsupported shapes keep the existing fallback argument path.
- `rg` found no alternate selected x86 scalar named-`i32` call argument source
  consumer bypassing the accepted authority helper.
- No public prepared call-plan APIs, route-debug strings, wrapper/fallback
  names, helper/oracle status, expected output, `ConsumedPlans` contracts,
  ABI/register/stack/result policy, or riscv behavior were changed.

## Suggested Next

Execute Step 4 by adding or updating focused proof for the agreed scalar
named-`i32` x86 path and the fail-closed rejection rows named by Steps 1-3.

## Watchouts

- Step 3 was already wired in code, so Step 4 should focus on proving behavior
  rather than reworking the helper boundary.
- Keep the `source_value_name` requirement as part of named authority; missing
  names are fallback rows, not partial successes.
- Route-debug status rows already name `gate=agreed`,
  `gate=missing_source_value`, `gate=missing_source_name`,
  `gate=prepared_source_mismatch`, `gate=source_value_mismatch`, and
  `gate=blocked`; do not rewrite these strings as an implementation shortcut.
- Riscv, ABI/register/stack/result policy, helper selection, wrapper
  instruction text, and fallback names remain out of scope.

## Proof

Command: `(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff)$' --output-on-failure) > test_after.log 2>&1`

Result: passed.

Proof log: `test_after.log`.
