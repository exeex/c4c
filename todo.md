Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and Localize Structured Const Lookup Failure

# Current Packet

## Just Finished

Plan Step 1 reproduced the structured static-member const lookup regression
and localized the missing semantic branch. `lower_var_expr` builds
`ScopedStaticMemberLookup` in
`src/frontend/hir/impl/expr/scalar_control.cpp`, then the local
`find_static_member_const_value` lambda tries
`find_struct_static_member_const_value(HirStructMemberLookupKey, ...)` before
falling back to `find_struct_static_member_const_value(std::string,
std::string)`. The structured overload in `src/frontend/hir/hir_types.cpp`
returns `std::nullopt` immediately when
`struct_static_member_const_values_by_owner_` has no direct keyed value, so it
bypasses the legacy string overload behavior that calls
`try_eval_instantiated_struct_static_member_const(tag, member)` and then
recurses through `module_->struct_defs[tag].base_tags`.

## Suggested Next

Next coherent packet: execute Plan Step 2 by changing only the structured
static-member const-value lookup path in `src/frontend/hir/hir_types.cpp` so a
complete structured key still wins for direct
`struct_static_member_const_values_by_owner_` hits, but misses preserve the
legacy string-path semantics: instantiated trait/static-member const
evaluation via `try_eval_instantiated_struct_static_member_const` and inherited
base recursion via `find_struct_static_member_const_value(base_tag, member)`.
Keep rendered tag/member strings as compatibility inputs for fallback only;
do not add named-test or trait-specific shortcuts outside the existing
semantic helper.

## Watchouts

- The exact early-return branch is the structured overload at
  `src/frontend/hir/hir_types.cpp:1416`: after metadata validation, it looks
  up `struct_static_member_const_values_by_owner_.find(key)` and returns
  `std::nullopt` on miss without any trait evaluation or base walk.
- The legacy behavior to preserve is the string overload at
  `src/frontend/hir/hir_types.cpp:1377`: keyed direct hit, rendered-map direct
  hit, `try_eval_instantiated_struct_static_member_const(tag, member)`, then
  recursive base lookup.
- The focused HIR failure shows the consequence directly:
  `ns::is_reference_v_T_Tns::add_lvalue_reference_T_int` remains
  `const = is_reference::value` instead of reducing to `const = 1`.
- Keep Step 2 semantic and general: do not weaken expected results and do not
  special-case the five failing test names.

## Proof

Repro command run to `test_after.log`:

```sh
cmake --build build --target c4cll frontend_hir_tests frontend_hir_lookup_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp|cpp_positive_sema_template_variable_trait_runtime_cpp|cpp_hir_template_inherited_member_typedef_trait)$' >> test_after.log 2>&1
```

Result: build had no work to do; focused repro failed 5/5 as expected. Failed
tests were 761, 762, 763, 764, and 1362. `test_after.log` is the proof log.
