Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Resolve Type-Binding Text Mirror Plumbing

# Current Packet

## Just Finished

Step 7 - Resolve Type-Binding Text Mirror Plumbing is complete.

`bind_consteval_call_env` now routes parser-owned
`template_param_name_text_ids[i]` into consteval template type binding mirror
population. `record_type_binding_mirrors` fills `TypeBindingTextMap` and
`TypeBindingNameTextMap` when valid parameter `TextId` metadata exists, beside
the existing rendered-name binding map and structured-key mirror path.

## Suggested Next

Step 8 final cleanup/demotion or lifecycle review packet: decide whether the
remaining type-binding text mirror scaffolding is sufficient to demote/remove
dead advisory code, or whether the active runbook should go through formal
lifecycle review before further parser/sema cleanup.

## Watchouts

- Keep this plan limited to parser/sema cleanup; HIR module lookup migration
  belongs to idea 99.
- Preserve rendered-string bridges required by AST, HIR, consteval, diagnostics,
  codegen, and link-name output.
- Do not touch parser struct/tag maps, template rendered names, `TypeSpec::tag`
  outputs, or HIR/type/codegen identity surfaces.
- Do not downgrade expectations or add testcase-shaped exceptions.
- Do not treat the parser `struct_tag_def_map` argument to `eval_const_int` as a
  removable string leftover; it is still the rendered tag bridge used by
  `sizeof`, `alignof`, and `offsetof`.
- Step 5 added sema-only template type-parameter `TextId` mirrors in
  `validate.cpp`; it did not alter parser metadata, HIR/type/codegen identity,
  template rendering, diagnostics, expectations, or consteval NTTP call
  binding maps.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.
- Step 6 added no parser metadata, HIR/type/codegen identity migration,
  template rendering changes, diagnostics changes, expectation rewrites, or
  broad compile-time engine call-site migration outside the owned sema path.
- Step 7 added no parser metadata, HIR/type/codegen identity migration,
  template rendering changes, diagnostics changes, expectation rewrites, or
  testcase-shaped exceptions.
- Preserve the unrelated untracked `review/parser_step8_demotion_route_review.md`
  artifact.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(frontend_parser_tests|cpp_positive_sema_consteval_template_cpp|cpp_positive_sema_consteval_template_sizeof_cpp|cpp_positive_sema_template_type_subst_cpp|cpp_positive_sema_template_type_traits_builtin_cpp|cpp_positive_sema_template_typename_typed_nttp_parse_cpp|cpp_positive_sema_template_type_context_nttp_parse_cpp|cpp_positive_sema_template_visible_typedef_type_arg_parse_cpp|cpp_positive_sema_dependent_template_typename_member_parse_cpp|cpp_positive_sema_template_alias_type_pack_runtime_cpp)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 10/10 tests.
