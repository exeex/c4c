# Ref-Overload Method Link-Name Binding

## Goal

Fix C++ ref-qualified method calls so once overload resolution selects the
concrete method body, the emitted direct-call carrier keeps that exact
lowered symbol identity instead of falling back to a generic per-method lookup
that can collapse `&` and `&&` overloads onto the same link name.

## Why This Is Separate

Several runtime-positive C++ regressions now compile successfully but execute
the wrong method body:

- `cpp_positive_sema_c_style_cast_base_ref_qualified_method_cpp`
- `cpp_positive_sema_c_style_cast_ref_qualified_method_cpp`
- `cpp_positive_sema_forwarding_ref_qualified_member_dispatch_cpp`
- `cpp_positive_sema_function_return_temporary_member_call_runtime_cpp`
- `cpp_positive_sema_ref_overload_method_basic_cpp`
- `cpp_positive_sema_ref_overload_method_reads_arg_cpp`

The current shape is not a parser failure. HIR chooses the correct overload in
multiple cases, but later direct-call metadata rewrites the chosen callee to
the most recently registered overload symbol, commonly the `__rref` variant.

That is a bounded lowering/binding bug and should be fixed without silently
expanding into broader parser or template work.

## Scope

- direct-call carrier construction for resolved member calls
- ref-overload method dispatch where the concrete mangled callee name is
  already known before the call expression is emitted
- focused runtime validation for the affected ref-qualified method regressions

## Non-Goals

- parser fixes for function-pointer declarators
- template struct specialization field-owner/type fixes
- redesigning the full struct-method registry shape unless a narrow follow-up
  proves it is required
- downgrading tests or routing around the issue with testcase-specific hacks

## Acceptance Criteria

- [ ] lvalue and rvalue ref-qualified method calls keep distinct direct-call
      link identities
- [ ] member calls that resolve to `&` overloads no longer emit the `__rref`
      callee by mistake
- [ ] the focused ref-qualified runtime regression subset passes

## Validation

- build the compiler
- run the focused C++ runtime subset covering the ref-qualified method cases
