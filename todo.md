Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve Instantiated Trait and Base Recursion Semantics

# Current Packet

## Just Finished

Plan Step 2 repaired structured static-member const-value fallback semantics.
`find_struct_static_member_const_value(HirStructMemberLookupKey, ...)` still
returns direct `struct_static_member_const_values_by_owner_` hits first, but
on by-owner miss it now derives fallback tag/member inputs from the structured
owner key and member text when available, then reuses the legacy string path
that performs instantiated trait/static-member evaluation and inherited base
recursion. The template-global initializer route in `lower_global_init` also
folds qualified template static-member const expressions with the rendered
qualified member segment so stale structured member text does not block
`is_reference<T>::value` style reduction.

## Suggested Next

Next coherent packet: supervisor should decide whether to accept Step 2 as
the focused repair slice or move to Plan Step 3 broader idea-136
static-member/template coverage. The next validation packet should include the
five repaired regressions plus nearby static-member const lookup coverage that
exercises direct structured hits, inherited base fallback, and template trait
static-member folding.

## Watchouts

- The fix does not downgrade any expectations or add named-test shortcuts.
- `tests/frontend/frontend_hir_tests.cpp` now includes a direct guard that a
  structured by-owner miss uses owner-key base fallback before stale rendered
  spelling.
- The delegated proof is focused; Step 3 still owns broader nearby idea-136
  validation and any baseline/full-suite decision.

## Proof

Proof command run to `test_after.log`:

```sh
cmake --build build --target c4cll frontend_hir_tests frontend_hir_lookup_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp|cpp_positive_sema_template_variable_trait_runtime_cpp|cpp_hir_template_inherited_member_typedef_trait)$' >> test_after.log 2>&1
```

Result: passed. The build completed and the focused ctest subset passed 5/5:
tests 761, 762, 763, 764, and 1362. `test_after.log` is the proof log.

Additional local check: `./build/tests/frontend/frontend_hir_tests` passed,
covering the new internal structured-miss/base-fallback guard.
