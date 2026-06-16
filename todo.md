Status: Active
Source Idea Path: ideas/open/283_cpp_dependent_reference_alias_c_style_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Dependent Reference Alias Cast Semantics

# Current Packet

## Just Finished

Step 2: Repair Dependent Reference Alias Cast Semantics completed.

Repaired the HIR type boundary shared by C-style cast targets and local
reference-alias declarations:
- `Lowerer::lower_cast_expr()` now sends cast target types through the same
  structured/TextId-aware callable type preparation chain before materializing
  the `CastExpr`.
- Local declarations now use the same structured binding inputs before
  reference storage materialization, so a declared `AliasL` does not keep the
  dependent owner aggregate as its storage value type.
- `resolve_struct_member_typedef_if_ready()` now prefers the last qualified
  owner segment for nested deferred member typedefs before falling back to the
  already-realized outer holder tag, preserving reference flags while resolving
  `Rebind<T>::AliasL` / nested alias owners.

Evidence from
`c_style_cast_dependent_template_member_ref_alias_basic.cpp`:
- HIR no longer materializes the lvalue alias as `struct Holder_T_int*&` or
  casts through `struct Holder_T_int&`; it lowers as reference storage
  `decl l: ?*& = (&((?&)x#L0))`.
- LLVM now emits scalar/reference operations:
  `store ptr %lv.x, ptr %lv.l`, `store i32 %t2, ptr %t3`, and
  `store i32 %t4, ptr %t5`.
- The rvalue alias still selects `pick__rref_overload`, preserving overload
  behavior.

## Suggested Next

Execute Step 3 by recording focused evidence across all three source-idea
alias forms, including direct template-member, nested template-member, and
nested dependent-typename aliases. Confirm the lvalue path is scalar/reference
shaped and the rvalue path still selects `pick__rref_overload`.

## Watchouts

- HIR prints the resolved nested template-member alias value type as `?` even
  though LLVM lowering is now scalar/reference shaped. Treat that as a printer
  or final spelling gap unless a later proof requires changing semantics.
- No test expectations, runtime harnesses, backend store lowering, or
  filename-specific paths were changed.
- This packet ran only the supervisor-selected focused regex; do not claim
  Step 4 broader validation complete.

## Proof

Ran delegated proof:

`( set -o pipefail; cmake --build build_debug && ctest --test-dir build_debug -R '^cpp_positive_sema_c_style_cast_.*dependent.*alias.*_cpp$' --output-on-failure ) 2>&1 | tee test_after.log`

Result: passed, 7/7 tests.

The three source-idea targets now pass:
- `cpp_positive_sema_c_style_cast_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_template_member_ref_alias_basic_cpp`
- `cpp_positive_sema_c_style_cast_nested_dependent_typename_ref_alias_basic_cpp`

The four related passing regex matches also remained green. Proof log path:
`test_after.log`.

Diagnostic evidence for this packet was written under `/tmp`:
`/tmp/c4c_283_fixed_direct_member.hir` and
`/tmp/c4c_283_fixed_direct_member.ll`.

Supervisor regression guard compared matching focused logs:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`.
Result: PASS, before 4/7 passed and after 7/7 passed, resolving the three
source-idea target failures with no new failures.

Supervisor also ran nearby reference/cast/template validation:
`ctest --test-dir build_debug -R '^(cpp_hir_member_typedef_.*|cpp_hir_template_member_owner_.*|cpp_hir_template_(function|method|function_deduction|function_pack|function_recursive).*|cpp_positive_sema_c_style_cast_.*|cpp_positive_sema_.*ref.*|cpp_positive_sema_template_alias_member_typedef_.*|cpp_positive_sema_template_variable_.*member_typedef.*)$' --output-on-failure`.
Result: passed, 284/284 tests.
