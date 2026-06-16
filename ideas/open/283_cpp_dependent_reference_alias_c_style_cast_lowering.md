# 283 C++ dependent reference alias C-style cast lowering

## Goal

Repair lowering for C-style casts to dependent reference aliases so lvalue and
rvalue reference aliases produce valid scalar/reference IR instead of
aggregate-shaped stores.

## Why This Exists

Full `ctest` currently fails three related C++ positive runtime tests:

- `cpp_positive_sema_c_style_cast_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_typename_ref_alias_basic_cpp`

Each emits invalid IR where an `i32` value is stored as a dependent holder
aggregate type such as `%struct.Holder_T_int`:

```llvm
%t2 = add i32 %p.value, 4
store %struct.Holder_T_int %t2, ptr %t3
```

The source cases cast `x` through dependent `AliasL = U&` and `AliasR = U&&`
aliases and then use ref-overload selection plus reference assignment.

## In Scope

- Inspect dependent nested template alias substitution for lvalue/rvalue
  reference aliases.
- Repair C-style cast lowering so reference aliases keep scalar/reference
  semantics instead of aggregate holder semantics.
- Preserve overload selection between `int&` and `int&&`.
- Keep the fix general across direct, nested template-member, and nested
  typename alias forms.

## Out of Scope

- Do not weaken or skip the three positive runtime tests.
- Do not treat the holder template name as a special case.
- Do not fold in default-template return cast repair, C aggregate call ABI, or
  AArch64 fp128/vararg crash work.

## Acceptance Criteria

- The three failing `cpp_positive_sema_c_style_cast_*dependent*alias*` tests
  pass under CTest.
- Generated IR for the reference alias casts no longer attempts to store an
  `i32` as `%struct.Holder_T_int`.
- Ref-overload behavior remains correct: lvalue casts select the lvalue
  overload and rvalue casts select the rvalue overload.
- Existing reference, cast, and template positive tests remain green.

## Reviewer Reject Signals

- Reject testcase-shaped checks for the exact three filenames or
  `Holder<T>::Rebind`.
- Reject expectation rewrites, runtime skips, or harness weakening.
- Reject a route that changes overload resolution results instead of fixing
  alias/reference lowering.
- Reject broad template rewrites that do not directly address dependent
  reference alias substitution.
