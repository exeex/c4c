Status: Active
Source Idea Path: ideas/open/283_cpp_dependent_reference_alias_c_style_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: 3-4
Current Step Title: Prove The Three Alias Forms And Broaden Reference, Cast, And Template Validation

# Current Packet

## Just Finished

Steps 3-4: Prove The Three Alias Forms And Broaden Reference, Cast, And
Template Validation completed.

Focused proof passed for all three source-idea alias forms:
- direct dependent template-member reference alias:
  `cpp_positive_sema_c_style_cast_dependent_template_member_ref_alias_basic_cpp`
- nested dependent template-member reference alias:
  `cpp_positive_sema_c_style_cast_nested_dependent_template_member_ref_alias_basic_cpp`
- nested dependent-typename reference alias:
  `cpp_positive_sema_c_style_cast_nested_dependent_typename_ref_alias_basic_cpp`

Refreshed HIR/LLVM evidence for the three forms shows scalar/reference-shaped
lvalue casts and preserved rvalue overload selection:
- HIR lowers lvalue aliases as reference storage, e.g.
  `decl l: ?*& = (&((?&)x#L0))`, and rvalue aliases as `decl r: ?*&& = (&x#L0)`.
- HIR lvalue overload calls stay on `pick((&((?&)x#L0)))`; rvalue overload
  calls stay on `pick__rref_overload((&x#L0))`.
- LLVM stores the reference as a pointer (`store ptr %lv.x, ptr %lv.l`) and
  updates through scalar `i32` stores (`store i32 %t2, ptr %t3`,
  `store i32 %t4, ptr %t5`).
- No aggregate `%struct.Holder_T_int` stores remain in the refreshed LLVM for
  the three repaired source paths; remaining `%struct.Holder_T_int` mentions
  are type declarations only.

Step 4 broader validation evidence from the supervisor remains green:
nearby reference, cast, member-typedef, template function/method/deduction,
template alias member typedef, and template variable member-typedef tests
passed 284/284.

## Suggested Next

Ask the plan owner to evaluate lifecycle closure for idea 283 using the
completed Step 3 focused proof and the recorded Step 4 broader validation.

## Watchouts

- HIR prints the resolved nested template-member alias value type as `?` even
  though LLVM lowering is now scalar/reference shaped. Treat that as a printer
  or final spelling gap unless a later proof requires changing semantics.
- No test expectations, runtime harnesses, backend store lowering, or
  filename-specific paths were changed.
- This packet did not rerun the 284/284 nearby validation; it recorded the
  already-run supervisor result as requested.

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

Refreshed diagnostic evidence for this packet was written under `/tmp`:
- `/tmp/c4c_283_step3_direct_member.hir`
- `/tmp/c4c_283_step3_direct_member.ll`
- `/tmp/c4c_283_step3_nested_member.hir`
- `/tmp/c4c_283_step3_nested_member.ll`
- `/tmp/c4c_283_step3_nested_typename.hir`
- `/tmp/c4c_283_step3_nested_typename.ll`

Supervisor regression guard compared matching focused logs:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`.
Result: PASS, before 4/7 passed and after 7/7 passed, resolving the three
source-idea target failures with no new failures.

Supervisor also ran nearby reference/cast/template validation:
`ctest --test-dir build_debug -R '^(cpp_hir_member_typedef_.*|cpp_hir_template_member_owner_.*|cpp_hir_template_(function|method|function_deduction|function_pack|function_recursive).*|cpp_positive_sema_c_style_cast_.*|cpp_positive_sema_.*ref.*|cpp_positive_sema_template_alias_member_typedef_.*|cpp_positive_sema_template_variable_.*member_typedef.*)$' --output-on-failure`.
Result: passed, 284/284 tests.
