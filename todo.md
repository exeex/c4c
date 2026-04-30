# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.5
Current Step Title: Preserve Parser Const-Int Boundary And HIR Blocker

## Just Finished

Step 2.4.4.6 deleted the remaining legacy rendered
`source_tag::member` publication from
`register_record_member_typedef_bindings()` and repaired
`parse_dependent_typename_specifier()` to consult structured
`QualifiedNameRef`-derived member typedef keys and preserve their `TypeSpec`
payload before using rendered `TextId` typedef probes. The structured route
covers direct qualified keys, namespace-relative record/member keys, reference
alias value-category handoff, and nested-owner fallback payloads, so the mirror
stays deleted while the dependent/template member typedef carrier remains the
authority.

## Suggested Next

Execute Step 2.5 by checking parser-owned const-int lookup callers and keeping
the HIR `NttpBindings` metadata blocker parked outside this parser/Sema plan.
Do not delete the rendered-name `eval_const_int` compatibility overload unless
the lifecycle switches to the HIR metadata idea first.

## Watchouts

- The carrier intentionally does not reconstruct or split rendered
  `owner::member` strings; it uses the existing record/member structured key
  produced at record finalization.
- `parse_dependent_typename_specifier()` first tries
  `find_typedef_type(qualified_name_key(qn))`, then uses
  `qualified_owner_name(qn)`, `resolve_namespace_context(owner_qn)`, and
  `record_member_typedef_key_in_context()` for namespace-relative member
  typedef carriers such as `eastl::add_lvalue_reference<...>::type`.
- The `TypeSpec` handoff intentionally narrows deferred owner/member payloads
  to reference aliases and concrete template-id owners; non-reference
  dependent alias chains continue to use the raw structured payload so HIR
  template member-owner resolution remains concrete.
- The repair intentionally did not restore rendered-key reconstruction,
  `owner::member` string splitting, or the deleted
  `register_typedef_binding(... source_tag::member ...)` mirror.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp|cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp|cpp_positive_sema_tuple_element_alias_mix_parse_cpp|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp)$' --output-on-failure) > test_after.log 2>&1`

Passed: build succeeded and CTest reported 5/5 matching tests passed.
Supervisor broader validation:
`(cmake --build build -j && ctest --test-dir build -R '^cpp_' --output-on-failure) > test_after.log 2>&1`

Passed: build succeeded and CTest reported 1105/1105 matching tests passed.
Canonical proof log: `test_after.log`.
