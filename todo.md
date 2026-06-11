Status: Active
Source Idea Path: ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select One Cross-Target Boundary

# Current Packet

## Just Finished

Step 1 selected the x86 call-boundary `ConsumedPlans` wrapper as the first
cross-target reuse boundary. The concrete seam is
`c4c::backend::x86::ConsumedPlans` plus its call selectors in
`src/backend/mir/x86/x86.hpp`: `shared_call_plan_lookups()`,
`find_consumed_call_plan(...)`, `find_consumed_call_argument_plan(...)`, and
`find_consumed_call_result_plan(...)`. Existing x86 call emission already
consumes that seam through `src/backend/mir/x86/module/module.cpp` while
keeping x86 instruction text, ABI register spelling, frame adjustment, storage
homes, and move-bundle use target-owned.

The reused route-view interface should be Route 6 call-use source facts:
`bir::Route6CallUseSourceIndex` and the typed call-argument source-producer /
direct-global / publication-source lookups keyed by call instruction,
argument/result role, and value role. The AArch64 proof source is
`ideas/closed/187_phase_e_route6_call_use_source_view_consumer_migration.md`,
with the production route-first/fallback shape in
`src/backend/mir/aarch64/codegen/calls.cpp` around
`find_scalar_call_argument_source_producer_materialization(...)` and Route 6
oracle coverage in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

The x86 target-local fallback/policy path remains the prepared call-plan and
storage surface already exposed by `ConsumedPlans`: `PreparedCallPlan`,
`PreparedCallArgumentPlan`, `PreparedCallResultPlan`, value homes,
before/after-call move bundles, frame/dynamic-stack plans, wrapper kind,
direct/indirect callee identity, variadic FPR count, memory-return/sret shape,
clobber/preserve sets, and final x86 operand/instruction spelling. Those stay
out of BIR; Route 6 supplies only semantic source facts where available and
must fail closed to the existing prepared call-plan path when absent.

## Suggested Next

Delegate Step 2 to thread the existing Route 6 call-use source view through the
x86 `ConsumedPlans` call-boundary selectors for one scalar call-argument source
class. Keep the prepared call-plan and storage/move surfaces as fallback/oracle
inputs, and do not move x86 ABI placement, wrapper-kind decisions, frame
layout, or instruction spelling into BIR.

## Watchouts

- Do not invent an x86-only BIR adapter before reusing the Route 6 interface
  already proven by AArch64.
- Keep x86 instruction selection, formatting, frame/register allocation, ABI
  policy, wrapper classification, and emission records target-owned.
- Do not duplicate `PreparedFunctionLookups` under a BIR-owned name.
- Do not weaken x86 route-debug, handoff-boundary, or prepared call-wrapper
  coverage; this boundary has observable wrapper/debug behavior, so compile-only
  proof is not enough for the next code packet.
- Do not claim broad prepared aggregate contraction from one selected wrapper
  thread.

## Proof

Mapping-only packet. No build or tests were run, and no `test_after.log` was
written for this packet by instruction.

Initial narrow proof command for the next code-changing packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_x86_route_debug|backend_x86_handoff_boundary)$'
```
