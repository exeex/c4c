Status: Active
Source Idea Path: ideas/open/283_cpp_dependent_reference_alias_c_style_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Reference Alias Cast Boundary

# Current Packet

## Just Finished

Step 1: Locate The Reference Alias Cast Boundary completed.

First bad boundary: HIR cast target materialization in
`Lowerer::lower_cast_expr()` after dependent substitution. The C-style cast
target is resolved far enough for overload selection, but the lvalue reference
alias target keeps the dependent owner aggregate as its value type before LIR
emission.

Concrete failing evidence:
- Direct template-member alias
  `c_style_cast_dependent_template_member_ref_alias_basic.cpp`:
  HIR lowers `AliasL` as
  `decl l: struct Holder_T_int*& = (&((struct Holder_T_int&)x#L0))`.
  LLVM then emits `store %struct.Holder_T_int %t2, ptr %t3` where `%t2` is
  produced by `add i32 %p.value, 4`.
- Nested template-member alias
  `c_style_cast_nested_dependent_template_member_ref_alias_basic.cpp`:
  HIR lowers the lvalue alias the same way,
  `decl l: struct Holder_T_int*& = (&((struct Holder_T_int&)x#L0))`, and LLVM
  emits the same invalid `%struct.Holder_T_int` store from an `i32`.
- Nested dependent-typename alias
  `c_style_cast_nested_dependent_typename_ref_alias_basic.cpp`:
  HIR lowers the lvalue alias as
  `decl l: struct Holder_T_int*& = (&((struct Holder_T_int&)x#L0))`; LLVM
  emits `store %struct.Holder_T_int %t2, ptr %t3` with
  `%struct.Holder_T_int = type { %struct.Inner }`.

Comparison notes:
- In all three failing forms, the rvalue alias path still lowers through
  `(&x#L0)` and selects `pick__rref_overload`, so the failure is concentrated
  on lvalue reference alias target/value-type resolution.
- A passing direct dependent-typename comparison lowers as
  `decl l: int*& = (&((int&)x#L0))`, which is the expected scalar/reference
  shape.
- `--dump-bir` cannot provide a BIR boundary in this build because it reports
  `backend support is disabled in this build`; the LLVM-path invalid IR is a
  downstream symptom of the bad HIR type.

## Suggested Next

Execute Step 2 by repairing `Lowerer::lower_cast_expr()` in
`src/frontend/hir/impl/expr/scalar_control.cpp` to run the same structured
member-typedef/reference resolution used by callable return/parameter
preparation after `substitute_signature_template_type()`.

Narrow target: after seeding a dependent cast target, call the
`resolve_signature_template_type_if_needed()` /
`resolve_struct_member_typedef_if_ready()` chain, preserving structured
TextId/owner-key metadata and reference flags, so `AliasL` materializes as
`int&` storage (`int*&`) rather than `struct Holder_T_int&` storage.

## Watchouts

- Do not weaken or skip the three positive runtime tests.
- Do not special-case the filenames or `Holder<T>::Rebind`.
- Preserve lvalue/rvalue reference overload behavior.
- Avoid a late LIR/LLVM store-type workaround; the first bad fact already
  exists in HIR before LIR emission.
- Do not collapse all dependent owner structs to their `type` member blindly;
  the repair should apply to C-style cast target typedef/member-alias
  resolution and preserve aggregate casts that are genuinely aggregate casts.

## Proof

Ran delegated proof:

`cmake --build --preset default && ctest --test-dir build_debug -R '^cpp_positive_sema_c_style_cast_.*dependent.*alias.*_cpp$' --output-on-failure 2>&1 | tee test_after.log`

Build passed with no work to do. CTest reproduced the current failures:
- `cpp_positive_sema_c_style_cast_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_typename_ref_alias_basic_cpp`

Four related regex matches passed. The shell pipeline exited successfully
because output was piped through `tee` without `pipefail`, but CTest reported
`3 tests failed out of 7`. Proof log path: `test_after.log`.

Diagnostic dumps used for investigation only were written under `/tmp`:
`/tmp/c4c_283_direct.hir`, `/tmp/c4c_283_nested_member.hir`,
`/tmp/c4c_283_nested_typename.hir`, and matching `.ll` files.
