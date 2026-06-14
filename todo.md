Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Call Argument Source Inventory

# Current Packet

## Just Finished

Step 1 - Call Argument Source Inventory recorded the exact x86 scalar
named-`i32` Route 6/prepared agreement row and nearby proof surfaces.

Reader/helper row:

- `c4c::backend::x86::find_consumed_scalar_i32_call_argument_source_authority`
  in `src/backend/mir/x86/x86.hpp` is the concrete authority gate.
- It reads `find_consumed_call_argument_plan(...)` for the prepared
  `PreparedCallArgumentPlan` at `(block_index, instruction_index, arg_index)`
  and `ConsumedPlans::shared_route6_call_use_source_index()` for the Route 6
  index.
- It is already narrowed to a named `bir::TypeKind::I32` call argument and
  returns only after Route 6 `ArgumentValue` evidence and prepared
  `source_value_id` agree.

Route 6 fields available for agreement:

- `Route6CallArgumentSourceRecord::status`, `source_kind`, `argument_value`,
  `source_value`, `source_value_id`, and `source_value_name`.
- Lookup key: `block`, `call_instruction_index`, `call.callee`, and
  `arg_index` through `route6_find_call_argument_source(...)`.
- Record validation helper:
  `route6_call_argument_source_matches_argument_value_record(...)`.
- Publication path:
  `publish_route6_named_scalar_i32_call_argument_source(...)` creates or
  completes `CallInst::arg_sources` only for named scalar `i32` argument values
  whose prepared source id/name and storage encoding are compatible.

Prepared fields available for agreement:

- `PreparedCallArgumentPlan::source_value_id` is the prepared identity row
  compared against `Route6CallArgumentSourceRecord::source_value_id`.
- Public retained lookup/API surfaces remain
  `PreparedBirModule::call_plans`,
  `PreparedFunctionLookups::call_plans`,
  `find_prepared_call_plans(...)`,
  `find_indexed_prepared_call_plan(...)`, and x86
  `find_consumed_call_argument_plan(...)`.

Compatibility and proof surfaces identified:

- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
  checks `ConsumedPlans` threading, agreement becoming named Route 6 authority,
  missing Route 6 fallback, missing source-name fallback, prepared call-plan
  selector preservation, and unchanged fallback assembly.
- The same file has prepared handoff drift rows that reject reopening local
  x86 call-argument/result ABI fallbacks when authoritative prepared
  BeforeCall/AfterCall bundles are removed.
- `tests/backend/bir/backend_x86_route_debug_test.cpp` checks route-debug
  output for available/agreed, missing source relationship, missing source
  value, missing source name, duplicate relationship, prepared source mismatch,
  and source value/name mismatch gates.
- `tests/backend/bir/backend_prepared_printer_test.cpp` and
  `src/backend/prealloc/prepared_printer/*` retain public prepared call-plan
  wrapper, fallback, and callsite dump visibility.

Fail-closed rows needed before code changes:

- Prepared-only evidence must not become Route 6 authority.
- Route 6-only evidence must not bypass missing prepared `source_value_id`.
- Mismatched Route 6/prepared source ids or source names must keep the gate
  blocked.
- Unsupported/non-selected rows must remain blocked, including non-named,
  non-`i32`, immediate, computed/symbol address, direct-global dependency,
  source-selection, ABI-bound, aggregate, and target-policy-sensitive rows.
- Duplicate/conflict rows must remain blocked through duplicate
  `CallArgumentSourceRelationship` or conflicting source id/name evidence.
- Fallback rows must preserve prepared call-plan selection and emitted x86 asm
  when Route 6 evidence is absent or non-authoritative.
- Public helper/oracle status, route-debug strings, wrapper/fallback names, and
  `ConsumedPlans` expectations must remain stable.

## Suggested Next

Execute Step 2 by deciding whether the existing
`find_consumed_scalar_i32_call_argument_source_authority(...)` helper is already
the adapter boundary or should be wrapped by a smaller x86-local query name, and
record the accepted agreement row plus rejection matrix without changing public
prepared call-plan APIs.

## Watchouts

- The reader/helper row already exists, so Step 2 should avoid designing a
  parallel abstraction unless it removes real ambiguity.
- The current helper requires `source_value_name`; the design step should keep
  this named-authority gate unless a proof row explicitly justifies otherwise.
- Route-debug status rows already name `gate=agreed`,
  `gate=missing_source_value`, `gate=missing_source_name`,
  `gate=prepared_source_mismatch`, `gate=source_value_mismatch`, and
  `gate=blocked`; do not rewrite these strings as an implementation shortcut.
- Riscv, ABI/register/stack/result policy, helper selection, wrapper
  instruction text, and fallback names remain out of scope.

## Proof

Command: `git diff --check -- todo.md`

Result: passed.

The delegated proof command is todo-only and does not produce `test_after.log`;
no replacement root log was created.
