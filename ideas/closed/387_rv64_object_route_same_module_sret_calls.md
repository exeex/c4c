# RV64 Object Route Same-Module Sret Calls

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md`
Exposed By: Step 1 evidence from idea 386.
Closed By: stack-homed sret parameter stores plus same-module frame-slot memory_return/sret call support

## Goal

Identify and lower, or precisely route, the RV64 object-route same-module
`bir::CallInst` family with ABI memory-return/sret state where the call plan
has `memory_return` and currently fails before normal argument lowering.

## Why This Exists

The active idea 386 originally grouped two representatives behind the same
generic `unsupported_instruction_fragment` diagnostic. Step 1 evidence showed
they are separate semantic call families.

`tests/c/external/gcc_torture/src/920908-1.c` reaches:

```text
bir.call void f(ptr sret(size=4, align=4) %t8, i32 2, i64 %t4, i64 %t7)
```

The same-module call has ABI memory-return/sret state, with the return object
home in a frame slot and the sret address represented through local frame
address materialization. The current object-route call lowering rejects the
call at the `call_plan->memory_return.has_value()` admission gate before
classifying ordinary argument lowering.

## In Scope

- Capture or refresh the prepared/BIR and call-plan evidence for the
  `920908-1.c` same-module sret call when this idea is activated.
- Classify the RV64 object-route responsibility for same-module memory-return
  calls, including sret address placement and post-call memory result handling.
- Add focused backend coverage for the smallest supportable same-module sret
  call fragment, or for a narrower fail-closed diagnostic if support is not
  justified.
- Preserve ordinary scalar call-argument support and diagnostics owned by idea
  386.
- Rerun `920908-1.c` and record any later boundary if the fragment advances.

## Out of Scope

- Frame-slot-address scalar GPR call arguments without memory-return state;
  those remain under idea 386.
- Reopening scalar GPR stack-slot formal-home support from idea 374.
- Byval aggregate parameter homes or aggregate `va_arg` helper lowering unless
  the refreshed sret evidence directly proves one is the existing owner.
- Broad RV64 call-lowering rewrites unrelated to same-module memory-return
  calls.
- Expectation downgrades or converting supported-path coverage to unsupported
  coverage without explicit approval.

## Acceptance Criteria

- The exact same-module sret call fragment is identified from prepared/BIR and
  call-plan evidence, not inferred from testcase names.
- Focused backend tests prove either semantic lowering for the selected sret
  call fragment or a narrower unsupported diagnostic.
- `920908-1.c` is rerun and documented against the new boundary.
- Any later boundary is routed to an existing or new owner instead of being
  folded silently into this idea.

## Closure Notes

Idea 387 is complete. Step 1 refreshed the representative evidence and
confirmed the same-module call carried `call_plan->memory_return` plus an sret
address represented through local frame address materialization. Step 2
classified the first support gap as stack-homed sret parameter pointer-value
stores. Commit `726c0025` added that support. Commit `1ab46b0b` then added
same-module frame-slot memory_return/sret call support.

Focused backend proof passed for the code slices, and `test_before.log` is the
current accepted backend baseline from Step 4 with 326/326 backend tests
passing. Step 5 reran the representative and proved the previous same-module
sret object-emission boundary is gone:

- `main` materializes the sret object address into `a0`.
- ordinary arguments are published into `a1`, `a2`, and `a3`.
- the object links and runs far enough to enter callee `f`.
- qemu evidence shows `f` entry receives `a2=10` and `a3=20`.

The remaining `920908-1.c` abort is a later owner, not an idea-387 sret owner.
Step 5 analysis shows c4c's callee-side variadic aggregate `va_arg` consumes
the first aggregate payload correctly, then advances the variadic cursor by 4
bytes instead of advancing to the next 8-byte variadic GPR save-area slot. The
second aggregate read therefore observes `0` instead of `20` and branches to
`abort()`. Evidence:

- `build/agent_state/387_step5_analysis.log`
- `build/agent_state/387_step5_920908-1.run.log`
- `test_after.log`

That later boundary is split to
`ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md`.

Close gate:

- Canonical `test_after.log` is the Step 5 representative log and no longer
  contains a CTest summary.
- Because Step 5 was todo-only and `test_before.log` is the current accepted
  backend baseline from Step 4, the plan owner used that current backend
  baseline as both sides of the non-regression close check:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.

## Reviewer Reject Signals

- Reject named-case-only handling for `920908-1.c` or callee name `f`.
- Reject bypassing `call_plan->memory_return` with a generic admission-gate
  deletion that does not prove correct sret address placement and memory-result
  behavior.
- Reject implementing frame-slot-address scalar GPR call arguments under this
  owner unless a shared abstraction is proven and idea 386 remains aligned.
- Reject expectation downgrades, allowlist filtering, or diagnostic-only
  renames claimed as same-module sret call progress.
- Reject broad RV64 call-lowering rewrites that do not directly prove the
  selected same-module memory-return call fragment and adjacent fail-closed
  cases.
