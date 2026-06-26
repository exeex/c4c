# RV64 Object Route Same-Module Sret Calls

Status: Open
Type: Target lowering follow-up
Parent: `ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md`
Exposed By: Step 1 evidence from idea 386.

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
